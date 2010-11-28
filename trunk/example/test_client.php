<?
include_once("memlinkclient.php");

function test_create($m, $key)
{
    $maskformat = "4:3:1";
    $valuesize  = 12;
    $ret = $m->create($key, $valuesize, $maskformat);
    if ($ret != MEMLINK_OK) {
        echo "create error: $ret";
        return -1;
    }
    return 0;
}

function test_insert($m, $key)
{
    for ($i = 0; $i < 100; $i++) {
        $val = sprintf("%012d", $i);
        $ret = $m->insert($key, $val, strlen($val), "8:1:1", 0);
        if ($ret != MEMLINK_OK) {
            echo "insert error! $ret";
            return -1;
        }
    }  
    return 0;
}

function test_stat($m, $key)
{
    $ret = $m->stat($key);
    if (is_null($ret)) {
        echo "stat error!";
        return -1;
    }

    echo "stat blocks: $ret->blocks";
    return 0;
}

function test_delete($m, $key)
{
    $value = sprintf("%012d", 1);
    $valuelen = strlen($value);
    $ret = $m->delete($m, $key, $value, $valuelen) 
    if ($ret != MEMLINK_OK) {
        echo "delete error: $ret";
        return -1;
    }
    return 0;
}

function test_tag($m, $key)
{
}


function test_range($m, $key)
{
    $ret = $m->range($key, "", 0, 10);
    if (is_null($ret)) {
        echo "range error!";
        return -1;
    }

    echo "range count: $ret->count";

    $item = $ret->root;

    while ($item) {
        echo "item: $item \n";
        $item = $item->next;
    }
    return 0;
}

function test_count($m, $key)
{
    $ret = $m->count($key, "");
    if (is_null($ret)) {
        echo "range error!";
        return -1;
    }

    return 0;
}

$m = new MemLinkClient("127.0.0.1", 11001, 11002, 10);

$key = "haha";


test_create($m, $key);


?>
