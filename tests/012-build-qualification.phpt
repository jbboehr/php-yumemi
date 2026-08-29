--TEST--
yumemi qualification builds expose the requested PHP runtime mode
--EXTENSIONS--
yumemi
--SKIPIF--
<?php
if (getenv('YUMEMI_EXPECT_ZTS') === false || getenv('YUMEMI_EXPECT_DEBUG') === false) {
    echo 'skip only used by build qualification checks';
}
?>
--FILE--
<?php
var_dump(getenv('YUMEMI_EXPECT_ZTS') === '1');
var_dump(getenv('YUMEMI_EXPECT_DEBUG') === '1');
var_dump((bool) PHP_ZTS);
var_dump((bool) PHP_DEBUG);
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
