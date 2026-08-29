--TEST--
yumemi accepts integer compareTo results returned by reference
--EXTENSIONS--
yumemi
--FILE--
<?php
final class ReferenceReturningComparisonProbe extends \jbboehr\Yumemi\InternalQuantity
{
    private int $comparison = 0;

    public function __construct(private int $rank)
    {
    }

    public function &compareTo(self $right): int
    {
        $this->comparison = 100 * ($this->rank <=> $right->rank);

        return $this->comparison;
    }
}

$low = new ReferenceReturningComparisonProbe(1);
$equal = new ReferenceReturningComparisonProbe(1);
$high = new ReferenceReturningComparisonProbe(2);

var_dump($low->compareTo($high));
var_dump($low <=> $high);
var_dump($high <=> $low);
var_dump($low <=> $equal);
?>
--EXPECT--
int(-100)
int(-1)
int(1)
int(0)
