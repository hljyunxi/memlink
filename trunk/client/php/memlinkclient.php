<?php
include_once("cmemlink.php");

class MemLinkClient
{
    private $client;

    function __construct($host, $readport, $writeport, $timeout)
    {
        $this->client = memlink_create($host, $readport, $writeport, $timeout);
        //$r = memlink_create($host,$readport,$writeport,$timeout);
        //$this->client = is_resource($r) ? new MemLink($r) : $r;
    }

    function close()
    {
        if ($this->client) {
            memlink_close($this->client);
        }
    }

    function destroy()
    {
        if ($this->client) {
            memlink_destroy($this->client);
        }
        $this->client = null;
    }

	function create_table($key, $valuesize, $maskformat, $listtype, $valuetype)
	{
    	if( False == is_int($valuesize) or False == is_string($key) or False == is_string($maskformat) )
    	{
    		return -1;
    	}
    	
		return memlink_cmd_create($this->client, $key, $valuesize, $maskformat, $listtype, $valuetype);
	}

	function create_table_list($key, $valuesize, $maskformat)
	{
    	if( False == is_int($valuesize) or False == is_string($key) or False == is_string($maskformat) )
    	{
    		return -1;
    	}
    	
		return memlink_cmd_create_list($this->client, $key, $valuesize, $maskformat);
	}

	function create_table_queue($key, $valuesize, $maskformat)
	{
    	if( False == is_int($valuesize) or False == is_string($key) or False == is_string($maskformat) )
    	{
    		return -1;
    	}
    	
		return memlink_cmd_create_queue($this->client, $key, $valuesize, $maskformat);
	}

    function create_table_sortlist($key, $valuesize, $maskformat, $valuetype)
	{
    	if( False == is_int($valuesize) or False == is_string($key) or False == is_string($maskformat) )
    	{
    		return -1;
    	}
    	
		return memlink_cmd_create_sortlist($this->client, $key, $valuesize, $maskformat, $valuetype);
	}
	
	function remove_table($name)
	{
		if (false == is_string($name)) {
			return -1;
		}
		return memlink_cmd_remove_table($this->client, $name);
	}

	
	function create_node($name, $key)
	{
		if (false == is_string($name) or false == is_string($key)) {
			return -1;
		}
		return memlink_cmd_create_node($this->client, $name, $key);
	}

	function ping()
    {
        return memlink_cmd_ping($this->client);
    }

    function dump()
    {
        return memlink_cmd_dump($this->client);
    }

    function clean($key)
    {
    	if( False == is_string($key))
    	{
    		return -1;
    	}
        return memlink_cmd_clean($this->client, $key);
    }

    function stat($key)
    {
    	if( False == is_string($key))
    	{
    		return NULL;
    	}
    
        $mstat = new MemLinkStat();

        $ret = memlink_cmd_stat($this->client, $key, $mstat);
		if ($ret == MEMLINK_OK) {
			return $mstat;
		}
		return NULL;
    }

	function stat2($key, $mstat)
	{
    	if( False == is_string($key))
    	{
    		return -1;
    	}
	
        return memlink_cmd_stat($this->client, $key, $mstat);
	}

    function stat_sys()
    {
        $stat = new MemLinkStatSys();
        $ret = memlink_cmd_stat_sys($this->client, $stat);
        if ($ret == MEMLINK_OK) {
            return $stat;
        }
        return NULL;
    }

    function stat_sys2($stat)
    {
        return memlink_cmd_stat_sys($this->client, $stat);
    }

    function delete($key, $value, $valuelen)
    {
    	if( False == is_string($key) or False == is_int($valuelen) )
    	{
    		return -1;
    	}
    	
        return memlink_cmd_del($this->client, $key, $value, $valuelen);
    }

    function insert($key, $value, $valuelen, $maskstr, $pos)
    {
    	if( False == is_string($key) or False == is_int($valuelen) or
    		False == is_int($pos) or False == is_string($maskstr) )
    	{
    		return -1;
    	}
		
        return memlink_cmd_insert($this->client, $key, $value, $valuelen, $maskstr, $pos);
    }

    function move($key, $value, $valuelen, $pos)
    {
    	if( False == is_string($key) or False == is_int($valuelen) or False == is_int($pos) )
    	{
    		return -1;
    	}
			
        return memlink_cmd_move($this->client, $key, $value, $valuelen, $pos);
    }

    function mask($key, $value, $valuelen, $maskstr)
    {
    	if( False == is_string($key) or False == is_int($valuelen) or False == is_string($maskstr) )
    	{
    		return -1;
    	}
    
        return memlink_cmd_mask($this->client, $key, $value, $valuelen, $maskstr);
    }

    function tag($key, $value, $valuelen, $tag)
    {
    	if( False == is_string($key) or False == is_int($valuelen) or False == is_int($tag) )
    	{
    		return -1;
    	}
    	
        return memlink_cmd_tag($this->client, $key, $value, $valuelen, $tag);
    }

    function range($key, $kind, $maskstr, $frompos, $len)
    {
    	if( False == is_string($key) or False == is_int($frompos) or
    		False == is_int($len) or False == is_string($maskstr) )
    	{
    		return NULL;
    	}
    	
        $result = new MemLinkResult();
        
        $ret = memlink_cmd_range($this->client, $key, $kind, $maskstr, $frompos, $len, $result);
		if ($ret == MEMLINK_OK) {
			return $result;
		}
		return NULL;
    }

	function range_visible($key, $maskstr, $frompos, $len) 
	{
		return $this->range($key, MEMLINK_VALUE_VISIBLE, $maskstr, $frompos, $len);
	}
	
	function range_tagdel($key, $maskstr, $frompos, $len) 
	{
		return $this->range($key, MEMLINK_VALUE_TAGDEL, $maskstr, $frompos, $len);
	}

	function range_all($key, $maskstr, $frompos, $len) 
	{
		return $this->range($key, MEMLINK_VALUE_ALL, $maskstr, $frompos, $len);
	}

	function range2($key, $kind, $maskstr, $frompos, $len, $result)
	{
    	if( False == is_string($key) or False == is_int($frompos) or False == is_int($len) or False == is_string($maskstr) )
    	{
    		return -1;
    	}
	
		return memlink_cmd_range($this->client, $key, $kind, $maskstr, $frompos, $len, $result);
	}

	function range2_visible($key, $maskstr, $frompos, $len, $result)
	{
		return $this->range2($key, MEMLINK_VALUIE_VISIBLE, $maskstr, $frompos, $len, $result);
	}

	function range2_tagdel($key, $maskstr, $frompos, $len, $result)
	{
		return $this->range2($key, MEMLINK_VALUIE_TAGDEL, $maskstr, $frompos, $len, $result);
	}

	function range2_all($key, $maskstr, $frompos, $len, $result)
	{
		return $this->range2($key, MEMLINK_VALUIE_ALL, $maskstr, $frompos, $len, $result);
	}

	function rmkey($key)
	{
    	if( False == is_string($key) ) {
    		return -1;
    	}
	
		return memlink_cmd_rmkey($this->client, $key);
	}

	function count($key, $maskstr)
	{
    	if( False == is_string($key) or False == is_string($maskstr) ) {
    		return NULL;
    	}
	
		$count = new MemLinkCount();
		$ret = memlink_cmd_count($this->client, $key, $maskstr, $count);
		if ($ret == MEMLINK_OK) {
			return $count;
		}
		return NULL;
	}

	function count2($key, $maskstr, $count)
	{
    	if( False == is_string($key) or False == is_string($maskstr) ) {
    		return -1;
    	}
	
		return memlink_cmd_count($this->client, $key, $maskstr, $count);
	}

    function lpush($key, $value, $valuelen, $maskstr="")
    {
    	if( False == is_string($key) or False == is_string($maskstr) ) {
    		return -1;
    	}
	
        return memlink_cmd_lpush($this->client, $key, $value, $valuelen, $maskstr);
    }

    function rpush($key, $value, $valuelen, $maskstr="")
    {
    	if( False == is_string($key) or False == is_string($maskstr) ) {
    		return -1;
    	}
	
        return memlink_cmd_rpush($this->client, $key, $value, $valuelen, $maskstr);
    }

    function lpop($key, $num=1)
    {
        $result = new MemLinkResult();
        $ret = memlink_cmd_lpop($this->client, $key, $num, $result);
		if ($ret == MEMLINK_OK) {
			return $result;
		}
		return NULL;
    }

    function lpop2($key, $num, $result)
    {
        return memlink_cmd_lpop($this->client, $key, $num, $result);
    }


    function rpop($key, $num=1)
    {
        $result = new MemLinkResult();
        $ret = memlink_cmd_lpop($this->client, $key, $num, $result);
		if ($ret == MEMLINK_OK) {
			return $result;
		}
		return NULL;
    }
    
    function rpop2($key, $num, $result)
    {
        return memlink_cmd_rpop($this->client, $key, $num, $result);
    }
    
    function sortlist_insert($key, $value, $valuelen, $maskstr="")
    {
        return memlink_cmd_sortlist_insert($this->client, $key, $value, $valuelen, $maskstr);
    }

    function sortlist_range($key, $kind, $vmin, $vminlen, $vmax, $vmaxlen, $maskstr='')
    {
        $result = new MemLinkResult();
        $ret = memlink_cmd_sortlist_range($this->client, $key, $kind, $maskstr, $vmin, $vminlen, $vmax, $vmaxlen, $result);
		if ($ret == MEMLINK_OK) {
			return $result;
		}
		return NULL;
    }

    function sortlist_range2($key, $kind, $vmin, $vminlen, $vmax, $vmaxlen, $maskstr, $result)
    {
        return memlink_cmd_sortlist_range($this->client, $key, $kind, $maskstr, $vmin, $vminlen, $vmax, $vmaxlen, $result);
    }

    function sortlist_del($key, $kind, $vmin, $vminlen, $vmax, $vmaxlen, $maskstr='')
    {
        return memlink_cmd_sortlist_del($this->client, $key, $kind, $maskstr, $vmin, $vminlen, $vmax, $vmaxlen);
    }

    function sortlist_count($key, $vmin, $vminlen, $vmax, $vmaxlen, $maskstr='')
    {
        $result = new MemLinkCount();
        $ret = memlink_cmd_sortlist_count($this->client, $key, $maskstr, $vmin, $vminlen, $vmax, $vmaxlen, $result);
        if ($ret == MEMLINK_OK) {
            return $result;
        }
        return NULL;
    }

    function sortlist_count2($key, $vmin, $vminlen, $vmax, $vmaxlen, $maskstr, $result)
    {
        return memlink_cmd_sortlist_count($this->client, $key, $maskstr, $vmin, $vminlen, $vmax, $vmaxlen, $result);
    }

    function read_conn_info()
    {
        $rcinfo = new MemLinkRcInfo();
        $ret = memlink_cmd_read_conn_info($this->client, $rcinfo);
        if ($ret == MEMLINK_OK) {
            return $rcinfo;
        }
        return NULL;
    }
    function write_conn_info()
    {
        $wcinfo = new MemLinkWcInfo();
        $ret = memlink_cmd_write_conn_info($this->client, $wcinfo);
        if ($ret == MEMLINK_OK) {
            return $wcinfo;
        }
        return NULL;
    }
    function sync_conn_info()
    {
        $scinfo = new MemLinkScInfo();
        $ret = memlink_cmd_sync_conn_info($this->client, $scinfo);
        if ($ret == MEMLINK_OK) {
            return $scinfo;
        }
        return NULL;
    }
    static function memlink_imkv_create() {
        $r=memlink_imkv_create();
        if (is_resource($r)) {
            $c=substr(get_resource_type($r), (strpos(get_resource_type($r), '__') ? strpos(get_resource_type($r), '__') + 2 : 3));
            if (!class_exists($c)) {
                return new MemLinkInsertMkv($r);
            }
            return new $c($r);
        }
        return $r;
    }
    static function memlink_ikey_create($key,$keylen) {
        $r=memlink_ikey_create($key,$keylen);
        if (is_resource($r)) {
            $c=substr(get_resource_type($r), (strpos(get_resource_type($r), '__') ? strpos(get_resource_type($r), '__') + 2 : 3));
            if (!class_exists($c)) {
                return new MemLinkInsertKey($r);
            }
            return new $c($r);
        }
        return $r;
    }
    static function memlink_ival_create($value,$valuelen,$maskstr,$pos) {
        $r=memlink_ival_create($value,$valuelen,$maskstr,$pos);
        if (is_resource($r)) {
            $c=substr(get_resource_type($r), (strpos(get_resource_type($r), '__') ? strpos(get_resource_type($r), '__') + 2 : 3));
            if (!class_exists($c)) {
                return new MemLinkInsertVal($r);
            }
            return new $c($r);
        }
        return $r;
    }

    function insert_mkv($array)
    {
        $mkv = $this->memlink_imkv_create();
        foreach ($array as $item) {
            if (is_array($item) && count($item) == 4) {
                if (is_null($item["key"]) || is_null($item["value"]) || 
                    is_null($item["mask"]) || is_null($item["pos"])) {
                    return $mkv;
                }
                $keyobj = $this->memlink_ikey_create($item["key"], strlen($item["key"]));
                $valobj = $this->memlink_ival_create($item["value"], strlen($item["value"]), $item["mask"], $item["pos"]);
                $ret = memlink_ikey_add_value($keyobj, $valobj); 
                if ($ret != MEMLINK_OK) {
                    return $mkv;
                }
                $ret = memlink_imkv_add_key($mkv, $keyobj);
                if ($ret != MEMLINK_OK) {
                    return $mkv;
                }
            }
        }
        $ret = memlink_cmd_insert_mkv($this->client, $mkv);
        return $mkv;
    }

    function rcinfo_free($info) {
        return memlink_rcinfo_free($info);
    }

    function wcinfo_free($info) {
        return memlink_wcinfo_free($info);
    }

    function scinfo_free($info) {
        return memlink_scinfo_free($info);
    }

    function mkv_destroy($mkv) {
        return memlink_imkv_destroy($mkv);
    }
}

?>
