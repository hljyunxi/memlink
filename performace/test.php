<?
include "memlinkclient.php";

function timediff($starttm, $endtm)
{
	return ($endtm['sec'] - $starttm['sec']) * 1000000 + ($endtm['usec'] - $starttm['usec']);
}

function test_insert($count)
{
	echo "====== test_insert ======\n";
	$m = MemLinkClient('127.0.0.1', 11001, 11002, 30);

	$key = "testmyhaha";
	$ret = $m->create($key, 12, "4:3:1");
	if ($ret != MEMLINK_OK) {
		echo "create error $ret\n";
		return -1;
	}
	
	$maskstr = "8:3:1";
	$starttm = gettimeofday();
	
	for ($i = 0; $i < $count; $i++) {
		$val = sprintf("%012d", $i);
		$ret = $m->insert($key, $val, strlen($val), $maskstr, 0);
		if (ret != MEMLINK_OK) {
			echo "insert error $ret\n";
			return -2;
		}
	}

	$endtm = gettimeofday();
	echo "use time: ".timediff($starttm, $endtm);
	echo "speed: ". $count / (timediff($starttm, $endtm) / 1000000);

	$m->destroy();
}


function test_range($frompos, $dlen, $count)
{
	echo "====== test_range ======\n";
	$key = "testmyhaha";
	$m = MemLinkClient('127.0.0.1', 11001, 11002, 30);
	
	$starttm = gettimeofday();
	for ($i = 0; $i < $count; $i++) {
		$ret = $m->range($key, "", $frompos, $dlen);
		if (is_null($ret)) {
			echo "range error!\n";
			return -1;
		}
	}
	$endtm = gettimeofday();
	echo "use time: ".timediff($starttm, $endtm);
	echo "speed: ". $count / (timediff($starttm, $endtm) / 1000000);

	$m->destroy();
}

$count = 100000;
test_insert($count);
test_range(0, 100, 1000);

?>
