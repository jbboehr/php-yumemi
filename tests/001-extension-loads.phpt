--TEST--
yumemi extension loads and reports its version
--EXTENSIONS--
yumemi
--FILE--
<?php
var_dump(extension_loaded('yumemi'));
var_dump(phpversion('yumemi'));
?>
--EXPECT--
bool(true)
string(5) "0.1.0"
