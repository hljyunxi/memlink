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

		String			key = "haha1";
		/*MemLinkClient	client = new MemLinkClient("127.0.0.1", 11001, 11002, 10);


		MemLinkStat ms = client.stat(key);
		System.out.println("blocks: " + ms.getBlocks());


		client.destroy();
		*/

		MemLink	m = memlink.memlink_create("127.0.0.1", 11001, 11002, 10);
		memlink.memlink_destroy(m);
	}
}
