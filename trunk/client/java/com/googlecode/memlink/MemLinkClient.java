package com.googlecode.memlink;

import java.util.*;


public class MemLinkClient
{
	private MemLink client = null;
	
	static {
		try{
			System.loadLibrary("cmemlink");
		}catch (UnsatisfiedLinkError e) {
			System.err.println("library libcmemlink.so load error! " + e);
			System.exit(1);
		}
	}

	public MemLinkClient (String host, int readport, int writeport, int timeout)
	{
		//client = new MemLink(host, readport, writeport, timeout);
		client = cmemlink.memlink_create(host, readport, writeport, timeout);
	}

	public void finalize()
	{
		destroy();
	}

	public void close()
	{
		if (client != null) {
			cmemlink.memlink_close(client);
		}
	}

	public void destroy()
	{
		if (client != null) {
			cmemlink.memlink_destroy(client);
			client = null;
		}
	}

	public int createTable(String name, int valuesize, String attrstr, short listtype, short valuetype)
	{
		return cmemlink.memlink_cmd_create_table(client, name, valuesize, attrstr, listtype, valuetype);
	}
    
    public int createTableList(String name, int valuesize, String attrstr)
	{
		return cmemlink.memlink_cmd_create_table_list(client, name, valuesize, attrstr);
	}

    public int createTableQueue(String name, int valuesize, String attrstr)
	{
		return cmemlink.memlink_cmd_create_table_queue(client, name, valuesize, attrstr);
	}

    public int createTableSortList(String name, int valuesize, String attrstr, int valuetype)
	{
		return cmemlink.memlink_cmd_create_table_sortlist(client, name, valuesize, attrstr, (short)valuetype);
	}

	public int createNode(String name, String key) 
	{
		return cmemlink.memlink_cmd_create_node(client, name, key);
	}

	public int dump()
	{
		return cmemlink.memlink_cmd_dump(client);
	}

	public int clean(String key)
	{
		return cmemlink.memlink_cmd_clean(client, key);
	}

	public int stat(String key, MemLinkStat ms)
	{
		return cmemlink.memlink_cmd_stat(client, key, ms);
	}

	public int delete(String key, byte []value)
	{
		return cmemlink.memlink_cmd_del(client, key, value);
	}

	public int insert(String key, byte []value, String attrstr, int pos)
	{
		return cmemlink.memlink_cmd_insert(client, key, value, attrstr, pos);
	}

	public int move(String key, byte []value, int pos)
	{
		return cmemlink.memlink_cmd_move(client, key, value, pos);
	}

	public int attr(String key, byte []value, String attrstr)
	{
		return cmemlink.memlink_cmd_attr(client, key, value, attrstr);
	}

	public int tag(String key, byte []value, int tag)
	{
		return cmemlink.memlink_cmd_tag(client, key, value, tag);
	}

	public int range(String key, int kind, String attrstr, int frompos, int len, MemLinkResult result)
	{
		/*MemLinkResult result = new MemLinkResult();
		int ret = cmemlink.memlink_cmd_range(client, key, kind, attrstr, frompos, len, result);
		if (ret == cmemlink.MEMLINK_OK) {
			return result;
		}
		return null;*/

		return cmemlink.memlink_cmd_range(client, key, kind, attrstr, frompos, len, result);
	}


	public int rangeVisible(String key, String attrstr, int frompos, int len, MemLinkResult result)
    {
        return range(key, cmemlink.MEMLINK_VALUE_VISIBLE, attrstr, frompos, len, result);
    }
	
    public int rangeTagdel(String key, String attrstr, int frompos, int len, MemLinkResult result)
    {
        return range(key, cmemlink.MEMLINK_VALUE_TAGDEL, attrstr, frompos, len, result);
    }

	public int rangeAll(String key, String attrstr, int frompos, int len, MemLinkResult result)
    {
        return range(key, cmemlink.MEMLINK_VALUE_ALL, attrstr, frompos, len, result);
    }

	public int rmkey(String key)
	{
		return cmemlink.memlink_cmd_rmkey(client, key);
	}

	public int count(String key, String attrstr, MemLinkCount count) 
	{
		return cmemlink.memlink_cmd_count(client, key, attrstr, count);
	}	

    public int lpush(String key, byte []value, String attrstr) 
    {
        return cmemlink.memlink_cmd_lpush(client, key, value, attrstr);
    }

    public int rpush(String key, byte []value, String attrstr) 
    {
        return cmemlink.memlink_cmd_rpush(client, key, value, attrstr);
    }

    public int lpop(String key, int num, MemLinkResult result)
    {
        return cmemlink.memlink_cmd_lpop(client, key, num, result);
    }

    public int rpop(String key, int num, MemLinkResult result)
    {
        return cmemlink.memlink_cmd_rpop(client, key, num, result);
    }
    
    public int sortListInsert(String key, byte []value, String attrstr)
    {
        return cmemlink.memlink_cmd_sortlist_insert(client, key, value, attrstr);
    }

    public int sortListRange(String key, int kind, byte []vmin, byte []vmax, 
							String attrstr, MemLinkResult result)
    {
        return cmemlink.memlink_cmd_sortlist_range(client, key, kind, attrstr, vmin, vmax, result);
    }

    public int sortListRangeAll(String key, byte []vmin, byte []vmax, String attrstr, MemLinkResult result)
    {
        return sortListRange(key, cmemlink.MEMLINK_VALUE_ALL, vmin, vmax, attrstr, result);
    }

    public int sortListRangeVisible(String key, byte []vmin, byte []vmax, String attrstr, MemLinkResult result)
    {
        return sortListRange(key, cmemlink.MEMLINK_VALUE_VISIBLE, vmin, vmax, attrstr, result);
    }

    public int sortListRangeTagdel(String key, byte []vmin, byte []vmax, String attrstr, MemLinkResult result)
    {
        return sortListRange(key, cmemlink.MEMLINK_VALUE_TAGDEL, vmin, vmax, attrstr, result);
    }


    public int sortListDelete(String key, int kind, byte []vmin, byte []vmax, String attrstr)
    {
        return cmemlink.memlink_cmd_sortlist_del(client, key, kind, attrstr, vmin, vmax);
    }

    public int sortListCount(String key, byte []vmin, byte []vmax, String attrstr, MemLinkCount count)
    {
        return cmemlink.memlink_cmd_sortlist_count(client, key, attrstr, vmin, vmax, count);
    }

    public int readConnInfo(MemLinkRcInfo info)
    {
        return cmemlink.memlink_cmd_read_conn_info(client, info);
    }

    public int writeConnInfo(MemLinkWcInfo info)
    {
        return cmemlink.memlink_cmd_write_conn_info(client, info);
    }

    public int syncConnInfo(MemLinkScInfo info)
    {
        return cmemlink.memlink_cmd_sync_conn_info(client, info);
    }

    public int insertMKV(MemLinkInsertMkv mkv)
    {
        return cmemlink.memlink_cmd_insert_mkv(client, mkv);
    }
}
