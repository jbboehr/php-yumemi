--TEST--
yumemi preserves compound assignment, cloning, properties, and garbage collection
--EXTENSIONS--
yumemi
--FILE--
<?php
final class StatefulProbe extends \jbboehr\Yumemi\InternalQuantity
{
    public ?self $peer = null;

    public function __construct(
        public int $value,
        public readonly string $unit,
    ) {
    }

    public function add(self $right): self
    {
        return new self($this->value + $right->value, $this->unit);
    }

    public function mul(int $right): self
    {
        return new self($this->value * $right, $this->unit);
    }
}

$left = new StatefulProbe(2, 'meter');
$original = $left;
$right = new StatefulProbe(3, 'meter');
$left += $right;

var_dump($left !== $original);
var_dump($original->value, $left->value, $left->unit);

$clone = clone $left;
$product = $clone * 2;

var_dump($clone !== $left);
var_dump($clone->value, $clone->unit, $product->value);

$cycle = new StatefulProbe(1, 'second');
$cycle->peer = $cycle;
$weakReference = WeakReference::create($cycle);
unset($cycle);
gc_collect_cycles();

var_dump($weakReference->get());
?>
--EXPECT--
bool(true)
int(2)
int(5)
string(5) "meter"
bool(true)
int(5)
string(5) "meter"
int(10)
NULL
