<?
include_once("memlink.php");

$conn = memlink_create("127.0.0.1", 11001, 11002, 10);

memlink_destroy($conn);


?>

