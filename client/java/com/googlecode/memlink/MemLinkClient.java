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
    
    public int create_list(String key, int valuesize, String maskstr)
	{
		return memlink.memlink_cmd_create_list(client, key, valuesize, maskstr);
	}

    public int create_queue(String key, int valuesize, String maskstr)
	{
		return memlink.memlink_cmd_create_queue(client, key, valuesize, maskstr);
	}

    public int create_sortlist(String key, int valuesize, String maskstr, short valuetype)
	{
		return memlink.memlink_cmd_create_sortlist(client, key, valuesize, maskstr, valuetype);
	}

	public int dump()
	{
		return memlink.memlink_cmd_dump(client);
	}

	public int clean(String key)
	{
		return memlink.memlink_cmd_clean(client, key);
	}

	public MemLinkStat stat(String key)
	{
		MemLinkStat ms = new MemLinkStat();
		int ret = memlink.memlink_cmd_stat(client, key, ms);
		if (ret == memlink.MEMLINK_OK) {
			return ms;
		}

		return null;
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

	public MemLinkResult range(String key, int kind, String maskstr, int frompos, int len)
	{
		MemLinkResult result = new MemLinkResult();
        
		int ret = memlink.memlink_cmd_range(client, key, kind, maskstr, frompos, len, result);
		if (ret == memlink.MEMLINK_OK) {
			return result;
		}
		return null;
	}


	public MemLinkResult range_visible(String key, String maskstr, int frompos, int len)
    {
        return range(key, memlink.MEMLINK_VALUE_VISIBLE, maskstr, frompos, len);
    }
	
    public MemLinkResult range_tagdel(String key, String maskstr, int frompos, int len)
    {
        return range(key, memlink.MEMLINK_VALUE_TAGDEL, maskstr, frompos, len);
    }

	public MemLinkResult range_all(String key, String maskstr, int frompos, int len)
    {
        return range(key, memlink.MEMLINK_VALUE_ALL, maskstr, frompos, len);
    }

	public int rmkey(String key)
	{
		return memlink.memlink_cmd_rmkey(client, key);
	}

	public MemLinkCount count(String key, String maskstr) 
	{
		MemLinkCount count = new MemLinkCount();

		int ret = memlink.memlink_cmd_count(client, key, maskstr, count);
		if (ret == memlink.MEMLINK_OK) {
			return count;
		}
		return null;
	}	
    public MemLinkRcInfo read_conn_info()
    {
        MemLinkRcInfo rcinfo = new MemLinkRcInfo();
        
        int ret = memlink.memlink_cmd_read_conn_info(client, rcinfo);
        if (ret == memlink.MEMLINK_OK) {
            return rcinfo;
        }
        return null;
    }
    public MemLinkWcInfo write_conn_info()
    {
        MemLinkWcInfo wcinfo = new MemLinkWcInfo();

        int ret = memlink.memlink_cmd_write_conn_info(client, wcinfo);
        if (ret == memlink.MEMLINK_OK) {
            return wcinfo;
        }
        return null;
    }
    public MemLinkScInfo sync_conn_info()
    {
        MemLinkScInfo scinfo = new MemLinkScInfo();

        int ret = memlink.memlink_cmd_sync_conn_info(client, scinfo);
        if (ret == memlink.MEMLINK_OK) {
            return scinfo;
        }
        return null;
    }
    public MemLinkInsertMkv create_mkv()
    {
        return memlink.memlink_imkv_create();
    }
    public MemLinkInsertKey create_key(String key)
    {
        return memlink.memlink_ikey_create(key, key.length());
    }
    public MemLinkInsertVal create_value(String val, String mask, int pos)
    {
        return memlink.memlink_ival_create(val, val.length(), mask, pos);
    }
    public int mkv_add_key(MemLinkInsertMkv mkv, MemLinkInsertKey keyobj)
    {
        return memlink.memlink_mkv_add_key(mkv, keyobj);
    }
    public int key_add_value(MemLinkInsertKey keyobj, MemLinkInsertVal valobj)
    {
        return memlink.memlink_ikey_add_value(keyobj, valobj);
    }
    public int insert_mkv(MemLinkInsertMkv mkv)
    {
        int ret = memlink.memlink_cmd_insert_mkv(client, mkv);
        
        return ret;
    }
    public int mkv_destroy(MemLinkInsertMkv mkv)
    {
        return memlink.memlink_mkv_destroy(mkv);
    }
    public void range_result_destroy(MemLinkResult result)
    {
        memlink.memlink_result_free(result);
    }
}
