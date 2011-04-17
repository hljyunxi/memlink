import com.googlecode.memlink.*;
import java.util.*;
import java.lang.*;

public class Test
{
	/*static {
        try{
			System.out.println("try load library: libmemlink.so");
            System.loadLibrary("memlink");
        }catch (UnsatisfiedLinkError e) {
            System.err.println("library libmemlink.so load error! " + e); 
            System.exit(1);
        }   
		System.out.println("load ok!");
    }*/

	public static void main(String [] args)
	{
        //String str = System.getProperty("java.library.path");
        //System.out.println(str);
		MemLinkClient m = new MemLinkClient("127.0.0.1", 11001, 11002, 10);
        
        int ret;
        int num = 100;
        int valuelen = 12;
        String maskformat = "4:3:1";
		String key = "haha";

	    ret = m.create_list(key, valuelen, maskformat);
        if (memlink.MEMLINK_OK != ret) {
            System.out.println("create error:" + ret);
            return;
        }else{
            System.out.println("create ok:" + key); 
        }
        
        int i = 0;
        //String value = "012345678912";
        for (i = 0; i < num; i++) {
            String value = String.format("%012d", i);
            ret = m.insert(key, value, "8:3:1", 0);
            if (memlink.MEMLINK_OK != ret) {
                System.out.println("errr insert!");
                return;
            }
        }
        
        System.out.println("insert 200 ok!"); 
        
        MemLinkStat stat;
        stat = m.stat(key);
        if (null == stat) {
            System.out.println("errr stat!");
            return;
        }
        //System.out.println("data: %d, used: %d", stat.getData(), stat.getData_used());

        if (stat.getData() != num || stat.getData_used() != num) {
            System.out.println("errr stat result!");
            return;
        }
        
        MemLinkResult rs;
        rs = m.range(key, memlink.MEMLINK_VALUE_VISIBLE, "", 0, 100);
        if (null == rs) {
            System.out.println("err range!");
            return;
        }
        if (rs.getCount() != num) {
            System.out.println("err count:" + rs.getCount());
            return;
        }
        i = num; 
        MemLinkItem item = rs.getRoot();
        while(i > 0) {
            i--;
            String value = String.format("%012d", i);
            System.out.println(item.getValue());
            if (!value.equals(item.getValue())){
                System.out.println("item.value: " + item.getValue());
                return;
            }
            item = item.getNext();
        }
		m.range_result_destroy(rs);

        MemLinkCount count;
        count = m.count(key, "8:3:1");
        if (null == count) {
            System.out.println("err count!");
            return;
        }
        
        System.out.println("count.visible:" + count.getVisible_count() + "\ncount.tagdel:" + count.getTagdel_count());
        m.destroy();
	}
}
