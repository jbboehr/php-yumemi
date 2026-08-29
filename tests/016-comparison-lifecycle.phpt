--TEST--
yumemi releases comparison values and recovers after repeated failures
--EXTENSIONS--
yumemi
--FILE--
<?php
final class InvalidComparisonReturnValue
{
    public static int $destructionCount = 0;

    public function __destruct()
    {
        self::$destructionCount++;
    }
}

final class InvalidResultLifecycleProbe extends \jbboehr\Yumemi\InternalQuantity
{
    public static int $destructionCount = 0;

    public function compareTo(self $right): mixed
    {
        return new InvalidComparisonReturnValue();
    }

    public function __destruct()
    {
        self::$destructionCount++;
    }
}

final class ThrowingLifecycleProbe extends \jbboehr\Yumemi\InternalQuantity
{
    public static int $destructionCount = 0;

    public function compareTo(self $right): never
    {
        throw new LogicException('expected comparison failure');
    }

    public function __destruct()
    {
        self::$destructionCount++;
    }
}

final class RecoveryComparisonProbe extends \jbboehr\Yumemi\InternalQuantity
{
    public static int $comparisonCount = 0;

    public function compareTo(self $right): int
    {
        self::$comparisonCount++;

        return PHP_INT_MAX;
    }
}

$recoveryLeft = new RecoveryComparisonProbe();
$recoveryRight = new RecoveryComparisonProbe();
$recoverySum = 0;

for ($iteration = 0; $iteration < 64; $iteration++) {
    try {
        new InvalidResultLifecycleProbe() <=> new InvalidResultLifecycleProbe();
    } catch (TypeError $throwable) {
        unset($throwable);
    }

    $recoverySum += $recoveryLeft <=> $recoveryRight;

    try {
        new ThrowingLifecycleProbe() <=> new ThrowingLifecycleProbe();
    } catch (LogicException $throwable) {
        unset($throwable);
    }
}

gc_collect_cycles();

echo 'return-values:', InvalidComparisonReturnValue::$destructionCount, PHP_EOL;
echo 'invalid-operands:', InvalidResultLifecycleProbe::$destructionCount, PHP_EOL;
echo 'throwing-operands:', ThrowingLifecycleProbe::$destructionCount, PHP_EOL;
echo 'recoveries:', RecoveryComparisonProbe::$comparisonCount, PHP_EOL;
echo 'recovery-sum:', $recoverySum, PHP_EOL;
?>
--EXPECT--
return-values:64
invalid-operands:128
throwing-operands:128
recoveries:64
recovery-sum:64
