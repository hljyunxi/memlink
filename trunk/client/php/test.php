<?
include_once("memlinkclient.php");

$class = new ReflectionClass("MemLink");
$methods = $class->getMethods();

foreach ($methods as $method) {
    echo "method:".$method->getName()."\n";
}


$m = new MemLinkClient("127.0.0.1", 11001, 11002, 10);
echo "host:".$m->readport;
$key = "haha1";

$ret = $m->stat($key);

$ret = $m->range($key, "::", 3, 10);

$m->destroy();

?>

