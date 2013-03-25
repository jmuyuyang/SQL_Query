--TEST--
Check for query presence
--SKIPIF--
<?php if (!extension_loaded("query")) print "skip"; ?>
--FILE--
<?php
	$query = new SqlQuery();
	$query->setTable("center");
	echo $query->update(array("test" => "test"),array("id" => "1"));
?>
--EXCEPT--
update center set test = 'test' where id = 1

