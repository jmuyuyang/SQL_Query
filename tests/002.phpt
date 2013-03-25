--TEST--
Check for query presence
--SKIPIF--
<?php if (!extension_loaded("query")) print "skip"; ?>
--FILE--
<?php
$sqlQuery = new SqlQuery();
$sqlQuery->setTable("sss");
echo $sqlQuery->table;
--EXPECT--
sss

