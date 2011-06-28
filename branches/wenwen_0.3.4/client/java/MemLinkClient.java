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

	public int create(String key, int valuesize, String maskstr, short listtype, short valuetype)
	{
		return cmemlink.memlink_cmd_create(client, key, valuesize, maskstr, listtype, valuetype);
	}
    
    public int createList(String key, int valuesize, String maskstr)
	{
		return cmemlink.memlink_cmd_create_list(client, key, valuesize, maskstr);
	}

    public int createQueue(String key, int valuesize, String maskstr)
	{
		return cmemlink.memlink_cmd_create_queue(client, key, valuesize, maskstr);
	}

    public int createSortList(String key, int valuesize, String maskstr, int valuetype)
	{
		return cmemlink.memlink_cmd_create_sortlist(client, key, valuesize, maskstr, (short)valuetype);
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

	public int delete(String key, byte[] value)
	{
		return cmemlink.memlink_cmd_del(client, key, value);
	}

	public int insert(String key, byte[] value, String maskstr, int pos)
	{
		return cmemlink.memlink_cmd_insert(client, key, value, maskstr, pos);
	}

	public int move(String key, byte[] value, int pos)
	{
		return cmemlink.memlink_cmd_move(client, key, value, pos);
	}

	public int mask(String key, byte[] value, String maskstr)
	{
		return cmemlink.memlink_cmd_mask(client, key, value, maskstr);
	}

	public int tag(String key, byte[] value, int tag)
	{
		return cmemlink.memlink_cmd_tag(client, key, value, tag);
	}

	public int range(String key, int kind, String maskstr, int frompos, int len, MemLinkResult result)
	{
		/*MemLinkResult result = new MemLinkResult();
		int ret = cmemlink.memlink_cmd_range(client, key, kind, maskstr, frompos, len, result);
		if (ret == memlink.MEMLINK_OK) {
			return result;
		}
		return null;*/

		return cmemlink.memlink_cmd_range(client, key, kind, maskstr, frompos, len, result);
	}


	public int rangeVisible(String key, String maskstr, int frompos, int len, MemLinkResult result)
    {
        return range(key, cmemlink.MEMLINK_VALUE_VISIBLE, maskstr, frompos, len, result);
    }
	
    public int rangeTagdel(String key, String maskstr, int frompos, int len, MemLinkResult result)
    {
        return range(key, cmemlink.MEMLINK_VALUE_TAGDEL, maskstr, frompos, len, result);
    }

	public int rangeAll(String key, String maskstr, int frompos, int len, MemLinkResult result)
    {
        return range(key, cmemlink.MEMLINK_VALUE_ALL, maskstr, frompos, len, result);
    }

	public int rmkey(String key)
	{
		return cmemlink.memlink_cmd_rmkey(client, key);
	}

	public int count(String key, String maskstr, MemLinkCount count) 
	{
		return cmemlink.memlink_cmd_count(client, key, maskstr, count);
	}	

    public int lpush(String key, byte[] value, String maskstr) 
    {
        return cmemlink.memlink_cmd_lpush(client, key, value, maskstr);
    }

    public int rpush(String key, byte[] value, String maskstr) 
    {
        return cmemlink.memlink_cmd_rpush(client, key, value, maskstr);
    }

    public int lpop(String key, int num, MemLinkResult result)
    {
        return cmemlink.memlink_cmd_lpop(client, key, num, result);
    }

    public int rpop(String key, int num, MemLinkResult result)
    {
        return cmemlink.memlink_cmd_rpop(client, key, num, result);
    }
    
    public int sortListInsert(String key, byte[] value, String maskstr)
    {
        return cmemlink.memlink_cmd_sortlist_insert(client, key, value, maskstr);
    }

    public int sortListRange(String key, int kind, byte[] vmin, byte[] vmax, String maskstr, MemLinkResult result)
    {
        return cmemlink.memlink_cmd_sortlist_range(client, key, kind, maskstr, vmin, vmax, result);
    }

    public int sortListRangeAll(String key, byte[] vmin, byte[] vmax, String maskstr, MemLinkResult result)
    {
        return sortListRange(key,cmemlink.MEMLINK_VALUE_ALL, vmin, vmax, maskstr, result);
    }

    public int sortListRangeVisible(String key, byte[] vmin, byte[] vmax, String maskstr, MemLinkResult result)
    {
        return sortListRange(key,cmemlink.MEMLINK_VALUE_VISIBLE, vmin, vmax, maskstr, result);
    }

    public int sortListRangeTagdel(String key, byte[] vmin, byte[] vmax, String maskstr, MemLinkResult result)
    {
        return sortListRange(key,cmemlink.MEMLINK_VALUE_TAGDEL, vmin, vmax, maskstr, result);
    }


    public int sortListDelete(String key, int kind, byte[] vmin, byte[] vmax, String maskstr)
    {
        return cmemlink.memlink_cmd_sortlist_del(client, key, kind, maskstr, vmin, vmax);
    }

    public int sortListCount(String key, byte[] vmin, byte[] vmax, String maskstr, MemLinkCount count)
    {
        return cmemlink.memlink_cmd_sortlist_count(client, key, maskstr, vmin, vmax, count);
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
