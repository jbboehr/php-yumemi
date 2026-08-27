--TEST--
yumemi registers an abstract internal quantity base class
--EXTENSIONS--
yumemi
--FILE--
<?php
$className = 'jbboehr\\Yumemi\\InternalQuantity';

var_dump(class_exists($className, false));

if (!class_exists($className, false)) {
    return;
}

$reflection = new ReflectionClass($className);

var_dump($reflection->isInternal());
var_dump($reflection->isAbstract());
var_dump($reflection->isInstantiable());

class QuantityProbe extends \jbboehr\Yumemi\InternalQuantity
{
}

echo get_parent_class(new QuantityProbe()), PHP_EOL;
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(false)
jbboehr\Yumemi\InternalQuantity
