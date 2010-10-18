<?
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
if ($ret != MEMLNK_OK) {
	echo "create error: $key\n";
	exit(-1);
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

$m->destroy();

?>

