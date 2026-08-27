--TEST--
yumemi preserves normal failures and delegated exceptions
--EXTENSIONS--
yumemi
--FILE--
<?php
final class MissingMethodsProbe extends \jbboehr\Yumemi\InternalQuantity
{
}

final class TypedProbe extends \jbboehr\Yumemi\InternalQuantity
{
    public function add(self $right): self
    {
        return $right;
    }

    public function mul(mixed $right): mixed
    {
        return $right;
    }

    public function rdiv(int $left): int
    {
        return $left;
    }
}

final class ThrowingProbe extends \jbboehr\Yumemi\InternalQuantity
{
    public function add(mixed $right): never
    {
        throw new LogicException('delegated method failure');
    }
}

function reportFailure(string $label, Closure $operation): void
{
    try {
        $operation();
        echo $label, ':no failure', PHP_EOL;
    } catch (Throwable $throwable) {
        echo $label, ':', $throwable::class, PHP_EOL;
    }
}

reportFailure('missing', fn () => new MissingMethodsProbe() + new MissingMethodsProbe());
reportFailure('missing-pow', fn () => new MissingMethodsProbe() ** 2);
reportFailure('missing-rdiv', fn () => 2 / new MissingMethodsProbe());
reportFailure('unsupported', fn () => new TypedProbe() % 2);
reportFailure('invalid-right', fn () => new TypedProbe() + 2);
reportFailure('scalar-left-add', fn () => 2 + new TypedProbe());
reportFailure('scalar-left-sub', fn () => 2 - new TypedProbe());
reportFailure('invalid-rdiv-argument', fn () => [] / new TypedProbe());
reportFailure('scalar-left-pow', fn () => 2 ** new TypedProbe());
reportFailure('exception', fn () => new ThrowingProbe() + 2);

$scalar = 2;
$probe = new TypedProbe();

echo 'quantity-left-mul:', $probe * $scalar, PHP_EOL;
echo 'literal-left-mul:', 2 * $probe, PHP_EOL;
echo 'variable-left-mul:', $scalar * $probe, PHP_EOL;
echo 'literal-left-div:', 2 / $probe, PHP_EOL;
echo 'variable-left-div:', $scalar / $probe, PHP_EOL;

try {
    new ThrowingProbe() + 2;
} catch (LogicException $exception) {
    echo $exception->getMessage(), PHP_EOL;
}
?>
--EXPECT--
missing:Error
missing-pow:Error
missing-rdiv:Error
unsupported:TypeError
invalid-right:TypeError
scalar-left-add:TypeError
scalar-left-sub:TypeError
invalid-rdiv-argument:TypeError
scalar-left-pow:TypeError
exception:LogicException
quantity-left-mul:2
literal-left-mul:2
variable-left-mul:2
literal-left-div:2
variable-left-div:2
delegated method failure
