<?php
include_once("memlinkclient.php");
/*
$class = new ReflectionClass("MemLink");
$methods = $class->getMethods();

foreach ($methods as $method) {
    echo "method:".$method->getName()."\n";
}
*/

$m = new MemLinkClient("127.0.0.1", 11001, 11002, 10);
//echo "host:".$m->readport;
$key = "haha4";


$ret = $m->create($key, 12, "4:3:1");
if ($ret != MEMLINK_OK) {
	echo "create error: $key\n";
	//exit(-1);
}

for ($i = 0; $i < 100; $i++) {
	$val = sprintf("%012d", $i);
	$ret = $m->insert($key, $val, strlen($val), "8:1:1", 0);
	if ($ret != MEMLINK_OK) {
		echo "insert error!", ret;
	}
}


$ret = $m->stat($key);
if (is_null($ret)) {
	echo "stat error: $key\n";
	exit(-1);
}
echo "stat, data_used: $ret->data_used\n";

$ret = $m->range($key, "::", 3, 10);
if (is_null($ret)) {
	echo "range error!";
	exit(-1);
}

echo "range, count: $ret->count\n";

$item = $ret->root;
while ($item) {
	echo "value: $item->value\n";
	$item = $item->next;
}

for ($i = 0; $i < 100; $i++) {
	$val = sprintf("%012d", $i);
	$ret = $m->delete($key, $val, strlen($val));
	if ($ret != MEMLINK_OK) {
		echo "delete error!", ret;
	}
}

$m->destroy();

?>

