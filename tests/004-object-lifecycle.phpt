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

    public function div(int $right): self
    {
        return new self(intdiv($this->value, $right), $this->unit);
    }

    public function rdiv(int $left): self
    {
        return new self(intdiv($left, $this->value), '1/' . $this->unit);
    }

    public function pow(int $right): self
    {
        return new self($this->value ** $right, $this->unit . '^' . $right);
    }
}

final class LifecycleProbe extends \jbboehr\Yumemi\InternalQuantity
{
    public static ?WeakReference $lastClone = null;
    public int $value = 1;
    public bool $throwOnClone = false;

    public function __clone(): void
    {
        self::$lastClone = WeakReference::create($this);
        ++$this->value;

        if ($this->throwOnClone) {
            throw new LogicException('clone failure');
        }
    }

    public function add(mixed $right): never
    {
        throw new LogicException('assignment failure');
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

$power = new StatefulProbe(2, 'meter');
$power **= 3;

$quantityLeftQuotient = new StatefulProbe(12, 'meter');
$quantityLeftQuotient /= 3;

$scalarLeftQuotient = 12;
$scalarLeftQuotient /= new StatefulProbe(3, 'meter');

$refcountedLeftQuotient = '15';
$refcountedLeftQuotient /= new StatefulProbe(3, 'meter');

var_dump($power->value, $power->unit);
var_dump($quantityLeftQuotient->value, $quantityLeftQuotient->unit);
var_dump($scalarLeftQuotient->value, $scalarLeftQuotient->unit);
var_dump($refcountedLeftQuotient->value, $refcountedLeftQuotient->unit);

$cycle = new StatefulProbe(1, 'second');
$cycle->peer = $cycle;
$weakReference = WeakReference::create($cycle);
unset($cycle);
gc_collect_cycles();

var_dump($weakReference->get());

$hookOriginal = new LifecycleProbe();
$hookClone = clone $hookOriginal;

echo 'clone-hook:', PHP_EOL;
var_dump($hookClone !== $hookOriginal, $hookOriginal->value, $hookClone->value);

$hookOriginal->throwOnClone = true;
try {
    clone $hookOriginal;
    echo 'clone:no failure', PHP_EOL;
} catch (LogicException $exception) {
    echo 'clone:', $exception->getMessage(), PHP_EOL;
}

var_dump($hookOriginal->value, LifecycleProbe::$lastClone->get());

$quantity = new LifecycleProbe();
$weakQuantity = WeakReference::create($quantity);
try {
    $quantity += 1;
    echo 'assignment:no failure', PHP_EOL;
} catch (LogicException $exception) {
    echo 'assignment:', $exception->getMessage(), PHP_EOL;
}

var_dump($quantity === $weakQuantity->get(), $quantity->value);
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
int(8)
string(7) "meter^3"
int(4)
string(5) "meter"
int(4)
string(7) "1/meter"
int(5)
string(7) "1/meter"
NULL
clone-hook:
bool(true)
int(1)
int(2)
clone:clone failure
int(1)
NULL
assignment:assignment failure
bool(true)
int(1)
