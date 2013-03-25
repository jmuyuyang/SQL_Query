--TEST--
Check for query presence
--SKIPIF--
<?php if (!extension_loaded("query")) print "skip"; ?>
--FILE--
<?php
	$query = new SqlQuery();
	$query->setTable("center");
	echo $query->where(array("user" => "test"))->select();
?>
--EXPECT--
select * from center where user = 'test'

