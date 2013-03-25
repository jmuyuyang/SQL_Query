--TEST--
Check for query presence
--SKIPIF--
<?php if (!extension_loaded("query")) print "skip"; ?>
--FILE--
<?php
	$query = new SqlQuery();
	$query->setTable("center");
	echo $query->delete(array("test" => "test"));
?>
--EXPECT--
delete from center where test = 'test'

