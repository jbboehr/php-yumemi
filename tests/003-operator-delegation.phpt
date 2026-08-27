--TEST--
yumemi delegates supported operators to userland methods
--EXTENSIONS--
yumemi
--FILE--
<?php
final class OperatorProbe extends \jbboehr\Yumemi\InternalQuantity
{
    public function add(mixed $right): string
    {
        return 'add:' . $right;
    }

    public function sub(mixed $right): string
    {
        return 'sub:' . $right;
    }

    public function mul(mixed $right): string
    {
        return 'mul:' . $right;
    }

    public function div(mixed $right): string
    {
        return 'div:' . $right;
    }
}

final class MultiplicationProbe extends \jbboehr\Yumemi\InternalQuantity
{
    public function __construct(private string $label)
    {
    }

    public function mul(self $right): string
    {
        return $this->label . ':' . $right->label;
    }
}

$probe = new OperatorProbe();

echo $probe + 2, PHP_EOL;
echo $probe - 3, PHP_EOL;
echo $probe * 4, PHP_EOL;
echo $probe / 5, PHP_EOL;

$left = new MultiplicationProbe('left');
$right = new MultiplicationProbe('right');

echo $left * $right, PHP_EOL;
echo $right * $left, PHP_EOL;
?>
--EXPECT--
add:2
sub:3
mul:4
div:5
left:right
right:left
