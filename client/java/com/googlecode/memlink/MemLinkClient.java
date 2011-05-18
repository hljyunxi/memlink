package com.googlecode.memlink;

import java.util.*;


public class MemLinkClient
{
	private MemLink client = null;
	
	static {
		try{
			System.loadLibrary("memlink");
		}catch (UnsatisfiedLinkError e) {
			System.err.println("library libmemlink.so load error! " + e);
			System.exit(1);
		}
	}

	public MemLinkClient (String host, int readport, int writeport, int timeout)
	{
		//client = new MemLink(host, readport, writeport, timeout);
		client = memlink.memlink_create(host, readport, writeport, timeout);
	}

	public void finalize()
	{
		destroy();
	}

	public void close()
	{
		if (client != null) {
			memlink.memlink_close(client);
		}
	}

	public void destroy()
	{
		if (client != null) {
			memlink.memlink_destroy(client);
			client = null;
		}
	}

	public int create(String key, int valuesize, String maskstr, short listtype, short valuetype)
	{
		return memlink.memlink_cmd_create(client, key, valuesize, maskstr, listtype, valuetype);
	}
    
    public int createList(String key, int valuesize, String maskstr)
	{
		return memlink.memlink_cmd_create_list(client, key, valuesize, maskstr);
	}

    public int createQueue(String key, int valuesize, String maskstr)
	{
		return memlink.memlink_cmd_create_queue(client, key, valuesize, maskstr);
	}

    public int createSortList(String key, int valuesize, String maskstr, int valuetype)
	{
		return memlink.memlink_cmd_create_sortlist(client, key, valuesize, maskstr, (short)valuetype);
	}

	public int dump()
	{
		return memlink.memlink_cmd_dump(client);
	}

	public int clean(String key)
	{
		return memlink.memlink_cmd_clean(client, key);
	}

	public int stat(String key, MemLinkStat ms)
	{
		return memlink.memlink_cmd_stat(client, key, ms);
	}

	public int delete(String key, String value)
	{
		return memlink.memlink_cmd_del(client, key, value, value.length());
	}

	public int insert(String key, String value, String maskstr, int pos)
	{
		return memlink.memlink_cmd_insert(client, key, value, value.length(), maskstr, pos);
	}

	public int move(String key, String value, int pos)
	{
		return memlink.memlink_cmd_move(client, key, value, value.length(), pos);
	}

	public int mask(String key, String value, String maskstr)
	{
		return memlink.memlink_cmd_mask(client, key, value, value.length(), maskstr);
	}

	public int tag(String key, String value, int tag)
	{
		return memlink.memlink_cmd_tag(client, key, value, value.length(), tag);
	}

	public int range(String key, int kind, String maskstr, int frompos, int len, MemLinkResult result)
	{
		/*MemLinkResult result = new MemLinkResult();
		int ret = memlink.memlink_cmd_range(client, key, kind, maskstr, frompos, len, result);
		if (ret == memlink.MEMLINK_OK) {
			return result;
		}
		return null;*/

		return memlink.memlink_cmd_range(client, key, kind, maskstr, frompos, len, result);
	}


	public int rangeVisible(String key, String maskstr, int frompos, int len, MemLinkResult result)
    {
        return range(key, memlink.MEMLINK_VALUE_VISIBLE, maskstr, frompos, len, result);
    }
	
    public int rangeTagdel(String key, String maskstr, int frompos, int len, MemLinkResult result)
    {
        return range(key, memlink.MEMLINK_VALUE_TAGDEL, maskstr, frompos, len, result);
    }

	public int rangeAll(String key, String maskstr, int frompos, int len, MemLinkResult result)
    {
        return range(key, memlink.MEMLINK_VALUE_ALL, maskstr, frompos, len, result);
    }

	public int rmkey(String key)
	{
		return memlink.memlink_cmd_rmkey(client, key);
	}

	public int count(String key, String maskstr, MemLinkCount count) 
	{
		return memlink.memlink_cmd_count(client, key, maskstr, count);
	}	

    public int lpush(String key, String value, String maskstr) 
    {
        return memlink.memlink_cmd_lpush(client, key, value, value.length(), maskstr);
    }

    public int rpush(String key, String value, String maskstr) 
    {
        return memlink.memlink_cmd_rpush(client, key, value, value.length(), maskstr);
    }

    public int lpop(String key, int num, MemLinkResult result)
    {
        return memlink.memlink_cmd_lpop(client, key, num, result);
    }

    public int rpop(String key, int num, MemLinkResult result)
    {
        return memlink.memlink_cmd_rpop(client, key, num, result);
    }
    
    public int sortListInsert(String key, String value, String maskstr)
    {
        return memlink.memlink_cmd_sortlist_insert(client, key, value, value.length(), maskstr);
    }

    public int sortListRange(String key, int kind, String vmin, String vmax, String maskstr, MemLinkResult result)
    {
        return memlink.memlink_cmd_sortlist_range(client, key, kind, maskstr, vmin, vmin.length(), vmax, vmax.length(), result);
    }

    public int sortListRangeAll(String key, String vmin, String vmax, String maskstr, MemLinkResult result)
    {
        return sortListRange(key, memlink.MEMLINK_VALUE_ALL, vmin, vmax, maskstr, result);
    }

    public int sortListRangeVisible(String key, String vmin, String vmax, String maskstr, MemLinkResult result)
    {
        return sortListRange(key, memlink.MEMLINK_VALUE_VISIBLE, vmin, vmax, maskstr, result);
    }

    public int sortListRangeTagdel(String key, String vmin, String vmax, String maskstr, MemLinkResult result)
    {
        return sortListRange(key, memlink.MEMLINK_VALUE_TAGDEL, vmin, vmax, maskstr, result);
    }


    public int sortListDelete(String key, int kind, String vmin, String vmax, String maskstr)
    {
        return memlink.memlink_cmd_sortlist_del(client, key, kind, maskstr, vmin, vmin.length(), vmax, vmax.length());
    }

    public int sortListCount(String key, String vmin, String vmax, String maskstr, MemLinkCount count)
    {
        return memlink.memlink_cmd_sortlist_count(client, key, maskstr, vmin, vmin.length(), vmax, vmax.length(), count);
    }

    public int readConnInfo(MemLinkRcInfo info)
    {
        return memlink.memlink_cmd_read_conn_info(client, info);
    }

    public int writeConnInfo(MemLinkWcInfo info)
    {
        return memlink.memlink_cmd_write_conn_info(client, info);
    }

    public int syncConnInfo(MemLinkScInfo info)
    {
        return memlink.memlink_cmd_sync_conn_info(client, info);
    }

    public int insertMKV(MemLinkInsertMkv mkv)
    {
        return memlink.memlink_cmd_insert_mkv(client, mkv);
    }
}
