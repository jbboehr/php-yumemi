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
        return 'mul:' . get_debug_type($right) . ':' . $right;
    }

    public function div(mixed $right): string
    {
        return 'div:' . $right;
    }

    public function pow(mixed $right): string
    {
        return 'pow:' . $right;
    }

    public function rdiv(mixed $left): string
    {
        return 'rdiv:' . $left;
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

final class ComparisonProbe extends \jbboehr\Yumemi\InternalQuantity
{
    public static int $compareCalls = 0;

    public function __construct(private int $rank)
    {
    }

    public function compareTo(self $right): int
    {
        self::$compareCalls++;

        return 0;
    }
}

$probe = new OperatorProbe();

echo $probe + 2, PHP_EOL;
echo $probe - 3, PHP_EOL;
echo $probe * 4, PHP_EOL;
echo $probe / 5, PHP_EOL;
echo $probe ** 6, PHP_EOL;
echo 7 / $probe, PHP_EOL;
echo +$probe, PHP_EOL;
echo -$probe, PHP_EOL;

$left = new MultiplicationProbe('left');
$right = new MultiplicationProbe('right');

echo $left * $right, PHP_EOL;
echo $right * $left, PHP_EOL;

$lower = new ComparisonProbe(1);
$higher = new ComparisonProbe(2);

echo 'object-state-compare:', $lower <=> $higher, PHP_EOL;
echo 'named-compare-calls:', ComparisonProbe::$compareCalls, PHP_EOL;
?>
--EXPECT--
add:2
sub:3
mul:int:4
div:5
pow:6
rdiv:7
mul:int:1
mul:int:-1
left:right
right:left
object-state-compare:-1
named-compare-calls:0
