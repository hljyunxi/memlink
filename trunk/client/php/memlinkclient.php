<?php
include_once("memlink.php");

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

	function create($key, $valuesize, $maskformat)
	{
		return memlink_cmd_create($this->client, $key, $valuesize, $maskformat);
	}

    function dump()
    {
        return memlink_cmd_dump($this->client);
    }

    function clean($key)
    {
        return memlink_cmd_clean($this->client, $key);
    }

    function stat($key)
    {
        $mstat = new MemLinkStat();

        $ret = memlink_cmd_stat($this->client, $key, $mstat);
		if ($ret == MEMLINK_OK) {
			return $mstat;
		}
		return NULL;
    }

	function stat2($key, $mstat)
	{
        return memlink_cmd_stat($this->client, $key, $mstat);
	}

    function delete($key, $value, $valuelen)
    {
        return memlink_cmd_del($this->client, $key, $value, $valuelen);
    }

    function insert($key, $value, $valuelen, $maskstr, $pos)
    {
        return memlink_cmd_insert($this->client, $key, $value, $valuelen, $maskstr, $pos);
    }

    function update($key, $value, $valuelen, $pos)
    {
        return memlink_cmd_update($this->client, $key, $value, $valuelen, $pos);
    }


    function mask($key, $value, $valulen, $maskstr)
    {
        return memlink_cmd_mask($this->client, $key, $value, $valuelen, $maskstr);
    }

    function tag($key, $value, $valuelen, $tag)
    {
        return memlink_cmd_tag($this->client, $key, $value, $valuelen, $tag);
    }

    function range($key, $maskstr, $frompos, $len)
    {
        $result = new MemLinkResult();
        
        $ret = memlink_cmd_range($this->client, $key, $maskstr, $frompos, $len, $result);
		if ($ret == MEMLINK_OK) {
			return $result;
		}
		return NULL;
    }

	function range2($key, $maskstr, $frompos, $len, $result)
	{
		return memlink_cmd_range($this->client, $key, $maskstr, $frompos, $len, $result);
	}

	function rmkey($key)
	{
		return memlink_cmd_rmkey($this->client, $key);
	}

	function count($key, $maskstr)
	{
		$count = new MemLinkCount();
		$ret = memlink_cmd_count($this->client, $key, $maskstr, $count);
		if ($ret == MEMLINK_OK) {
			return $count;
		}
		return NULL;
	}

	function count2($key, $maskstr, $count)
	{
		return memlink_cmd_range($this->client, $key, $maskstr, $result);
	}
}

?>
