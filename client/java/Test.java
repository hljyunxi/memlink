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

    public static void testQueue()
    {
		MemLinkClient m = new MemLinkClient("127.0.0.1", 11001, 11002, 10);
        int i;
        String key = "haha";
        
        m.rmkey(key);

        int ret;

        ret = m.createQueue(key, 10, "4:3:1");
        if (ret != memlink.MEMLINK_OK) {
            System.out.println("create queue error!" + ret);
            return;
        }
        
        for (i = 0; i < 100; i++) {
            String value = String.format("%010d", i);
            ret = m.lpush(key, value, "8:1:1");
            if (ret != memlink.MEMLINK_OK) {
                System.out.println("lpush error:" + ret);
                return;
            }
        }
        for (i = 100; i < 200; i++) {
            String value = String.format("%010d", i);
            ret = m.rpush(key, value, "8:1:0");
            if (ret != memlink.MEMLINK_OK) {
                System.out.println("rpush error:" + ret);
                return;
            }
        }
        
        MemLinkCount count = new MemLinkCount();
        ret = m.count(key, "", count);
        if (ret == memlink.MEMLINK_OK) {
            System.out.println("count:" + count.getVisible_count());
            if (count.getVisible_count() + count.getTagdel_count() != 200) {
                System.out.println("count error!");
            }
        }

        for (i = 0; i < 20; i++) {
            MemLinkResult result = new MemLinkResult();
            ret = m.lpop(key, 2, result);
            if (ret != memlink.MEMLINK_OK) {
                System.out.println("lpop error:" + ret);
                return;
            }
        }
        for (i = 0; i < 20; i++) {
            MemLinkResult result = new MemLinkResult();
            ret = m.rpop(key, 2, result);
            if (ret != memlink.MEMLINK_OK) {
                System.out.println("rpop error:" + ret);
                return;
            }
        }

        MemLinkCount count2 = new MemLinkCount();
        ret = m.count(key, "", count2);
        if (ret == memlink.MEMLINK_OK) {
            System.out.println("count:" + count2.getVisible_count());
            if (count2.getVisible_count() + count2.getTagdel_count() != 120) {
                System.out.println("count error!");
            }
        }

        m.destroy();
    }
    public static void testSortList()
    {
        MemLinkClient m = new MemLinkClient("127.0.0.1", 11001, 11002, 10);
        int i;
        String key = "haha";
        
        m.rmkey(key);

        int ret;

        ret = m.createSortList(key, 10, "4:3:1", memlink.MEMLINK_VALUE_STRING);
        if (ret != memlink.MEMLINK_OK) {
            System.out.println("create queue error!" + ret);
            return;
        }
       
        List<String> values = new ArrayList<String>();

        for (i = 0; i < 100; i++) {
            String value = String.format("%010d", i);
            values.add(value);
        }

        Collections.shuffle(values);

        for (i = 0; i < 100; i++) {
            System.out.println(values.get(i));
        }

        for (i = 0; i < 100; i++) {
            ret = m.sortListInsert(key, values.get(i), "7:2:1");
            if (ret != memlink.MEMLINK_OK) {
                System.out.println("insert error: " + ret);
                return;
            }
        }
        
        String first = String.format("%010d", 0);
        String last = String.format("%010d", 100);

        String start = String.format("%010d", 15);
        String end   = String.format("%010d", 33);

        MemLinkCount count = new MemLinkCount();
        ret = m.sortListCount(key, first, last, "", count);
        if (ret != memlink.MEMLINK_OK) {
            System.out.println("count error: " + ret);
            return; 
        }
        System.out.println(count.getVisible_count());


        ret = m.sortListCount(key, start, end, "", count);
        if (ret != memlink.MEMLINK_OK) {
            System.out.println("count error: " + ret);
            return; 
        }
        if (count.getVisible_count() != 18) {
            System.out.println("count error: " + ret);
            return;
        }

        ret = m.sortListDelete(key, memlink.MEMLINK_VALUE_ALL, start, end, "");
        if (ret != memlink.MEMLINK_OK) {
            System.out.println("delete error: " + ret);
            return; 
        }

        ret = m.sortListCount(key, first, last, "", count);
        if (ret != memlink.MEMLINK_OK) {
            System.out.println("count error: " + ret);
            return; 
        }
        if (count.getVisible_count() != 100 - 18) {
            System.out.println("count error: " + ret);
        }

        m.destroy();
    }

    public static void test()
    {
        MemLinkClient m = new MemLinkClient("127.0.0.1", 11001, 11002, 10);
        
        int ret;
        int num = 100;
        int valuelen = 12;
        String maskformat = "4:3:1";
		String key = "haha";

	    ret = m.createList(key, valuelen, maskformat);
        if (memlink.MEMLINK_OK != ret) {
            System.out.println("create error:" + ret);
            return;
        }else{
            System.out.println("create ok:" + key); 
        }
        
        int i = 0;
        //String value = "012345678912";
        for (i = 0; i < num; i++) {
            String value = String.format("%010d", i);
            ret = m.insert(key, value, "8:3:1", 0);
            if (memlink.MEMLINK_OK != ret) {
                System.out.println("insert error!");
                return;
            }
        }
        
        System.out.println("insert 200 ok!"); 
        
        MemLinkStat stat = new MemLinkStat();
        ret = m.stat(key, stat);
        if (memlink.MEMLINK_OK != ret) {
            System.out.println("stat error!");
            return;
        }
        //System.out.println("data: %d, used: %d", stat.getData(), stat.getData_used());

        if (stat.getData() != num || stat.getData_used() != num) {
            System.out.println("stat result error!");
            return;
        }
        stat.delete();
        
        MemLinkResult rs = new MemLinkResult();
        ret = m.range(key, memlink.MEMLINK_VALUE_VISIBLE, "", 0, 100, rs);
        if (memlink.MEMLINK_OK != ret) {
            System.out.println("range error!");
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
            String value = String.format("%010d", i);
            System.out.println(item.getValue());
            if (!value.equals(item.getValue())){
                System.out.println("item.value: " + item.getValue());
                return;
            }
            item = item.getNext();
        }
        rs.delete();

        MemLinkCount count = new MemLinkCount();
        ret = m.count(key, "8:3:1", count);
        if (memlink.MEMLINK_OK != ret) {
            System.out.println("count error!");
            return;
        }
        
        System.out.println("count.visible:" + count.getVisible_count() + "\ncount.tagdel:" + count.getTagdel_count());
        count.delete(); 

        m.destroy();

    }

	public static void main(String [] args)
	{
        //testQueue();			
        testSortList();
    }
}
