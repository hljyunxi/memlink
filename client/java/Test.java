import com.googlecode.memlink.*;
import java.util.*;
import java.lang.*;

public class Test
{
	static {
        try{
			System.out.println("try load library: libmemlink.so");
            System.loadLibrary("memlink");
        }catch (UnsatisfiedLinkError e) {
            System.err.println("library libmemlink.so load error! " + e); 
            System.exit(1);
        }   
		System.out.println("load ok!");
    }

	public static void main(String [] args)
	{
        //String str = System.getProperty("java.library.path");
        //System.out.println(str);

		MemLink m = memlink.memlink_create("127.0.0.1", 11011, 11012, 10);
        
        int ret;
        int valuelen = 12;
        String maskformat = "4:3:1";
		String key = "haha";
	    ret = memlink.memlink_cmd_create(m, key, valuelen, maskformat);
        if (0 != ret)
        {
            System.out.println("errr create key!");
            return;
        }
        
        int i = 0;
        //String value = "012345678912";
        for (i = 0; i < 200; i++)
        {
            String value = String.format("%012d", i);
            ret = memlink.memlink_cmd_insert(m, key, value, 12, "8:3:1", 0);
            if (0 != ret)
            {
                System.out.println("errr insert!");
                return;
            }
        }
        
        MemLinkStat stat = new MemLinkStat();
        ret = memlink.memlink_cmd_stat(m, key, stat);
        if (0 != ret)
        {
            System.out.println("errr create key!");
            return;
        }
        if (stat.getData() != 200 || stat.getData_used() != 200)
        {    
            System.out.println("errr stat key!");
            return;
        }
        
        MemLinkResult rs = new MemLinkResult();
        ret = memlink.memlink_cmd_range(m, key, memlinkConstants.MEMLINK_VALUE_VISIBLE, "", 0, 200, rs);
        if (0 != ret)
        {
            System.out.println("err range!");
            return;
        }
        if (rs.getCount() != 200)
        {
            System.out.println("err count:" + rs.getCount());
            return;
        }
        i = 200;
        MemLinkItem item = rs.getRoot();
        while(i > 0)
        {
            i--;
            String value = String.format("%012d", i);
            if ( !value.equals( item.getValue() ) )
            {
                System.out.println("item.value: " + item.getValue());
                return;
            }
            item = item.getNext();
        }

        MemLinkCount count = new MemLinkCount();
        ret = memlink.memlink_cmd_count(m, key, "8:3:1", count);
        if (0 != ret)
        {
            System.out.println("err count!");
            return;
        }
        
        System.out.println("count.visible:" + count.getVisible_count() + "\ncount.tagdel:" + count.getTagdel_count());
        memlink.memlink_destroy(m);
	}
}
