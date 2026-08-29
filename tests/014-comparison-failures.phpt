--TEST--
yumemi preserves comparison fallbacks and delegated failures
--EXTENSIONS--
yumemi
--FILE--
<?php
final class MissingComparisonProbe extends \jbboehr\Yumemi\InternalQuantity
{
}

final class PrivateComparisonProbe extends \jbboehr\Yumemi\InternalQuantity
{
    private function compareTo(self $right): int
    {
        return 0;
    }
}

final class StaticComparisonProbe extends \jbboehr\Yumemi\InternalQuantity
{
    public static function compareTo(self $right): int
    {
        return 0;
    }
}

final class InvalidComparisonResultProbe extends \jbboehr\Yumemi\InternalQuantity
{
    public function compareTo(self $right): string
    {
        return 'equal';
    }
}

final class TypedComparisonProbe extends \jbboehr\Yumemi\InternalQuantity
{
    public function compareTo(self $right): int
    {
        return 0;
    }
}

final class ThrowingComparisonProbe extends \jbboehr\Yumemi\InternalQuantity
{
    public function compareTo(self $right): never
    {
        throw new LogicException('delegated comparison failure');
    }
}

final class UnrelatedComparisonProbe
{
}

function reportComparisonFailure(string $label, Closure $comparison): void
{
    try {
        $comparison();
        echo $label, ':no failure', PHP_EOL;
    } catch (Throwable $throwable) {
        echo $label, ':', $throwable::class, PHP_EOL;
    }
}

reportComparisonFailure('missing', fn () => new MissingComparisonProbe() <=> new MissingComparisonProbe());
reportComparisonFailure('private', fn () => new PrivateComparisonProbe() <=> new PrivateComparisonProbe());
reportComparisonFailure('static', fn () => new StaticComparisonProbe() <=> new StaticComparisonProbe());
reportComparisonFailure('invalid-result', fn () => new InvalidComparisonResultProbe() <=> new InvalidComparisonResultProbe());
reportComparisonFailure('invalid-argument', fn () => new TypedComparisonProbe() <=> new MissingComparisonProbe());
reportComparisonFailure('exception', fn () => new ThrowingComparisonProbe() <=> new ThrowingComparisonProbe());
reportComparisonFailure('sort-exception', function (): void {
    $values = [new ThrowingComparisonProbe(), new ThrowingComparisonProbe()];
    sort($values, SORT_REGULAR);
});

var_dump(new MissingComparisonProbe() == new UnrelatedComparisonProbe());
var_dump(new UnrelatedComparisonProbe() == new MissingComparisonProbe());
var_dump(new MissingComparisonProbe() == null);
var_dump(null != new MissingComparisonProbe());
var_dump(@(new MissingComparisonProbe() <=> 0));

try {
    new ThrowingComparisonProbe() <=> new ThrowingComparisonProbe();
} catch (LogicException $exception) {
    echo $exception->getMessage(), PHP_EOL;
}
?>
--EXPECT--
missing:Error
private:Error
static:Error
invalid-result:TypeError
invalid-argument:TypeError
exception:LogicException
sort-exception:LogicException
bool(false)
bool(false)
bool(false)
bool(true)
int(1)
delegated comparison failure
