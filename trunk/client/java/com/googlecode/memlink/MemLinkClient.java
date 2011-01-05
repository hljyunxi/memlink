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

	public int create(String key, int valuesize, String maskstr)
	{
		return memlink.memlink_cmd_create(client, key, valuesize, maskstr);
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

	public int update(String key, String value, int pos)
	{
		return memlink.memlink_cmd_update(client, key, value, value.length(), pos);
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

}
