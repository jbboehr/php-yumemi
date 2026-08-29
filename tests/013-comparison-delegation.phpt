--TEST--
yumemi delegates natural comparison to userland compareTo
--EXTENSIONS--
yumemi
--FILE--
<?php
final class ComparisonProbe extends \jbboehr\Yumemi\InternalQuantity
{
    public static int $comparisonCount = 0;

    public function __construct(
        public string $storageKey,
        public int $rank,
    ) {
    }

    public function compareTo(self $right): int
    {
        self::$comparisonCount++;

        return 100 * ($this->rank <=> $right->rank);
    }
}

function comparisonProbe(string $storageKey, int $rank): ComparisonProbe
{
    return new ComparisonProbe($storageKey, $rank);
}

$low = comparisonProbe('z-low', 1);
$equal = comparisonProbe('a-equal', 1);
$middle = comparisonProbe('m-middle', 2);
$high = comparisonProbe('a-high', 3);

var_dump($low == $equal);
var_dump($low != $equal);
var_dump($low < $high);
var_dump($low <= $equal);
var_dump($high > $low);
var_dump($equal >= $low);
var_dump($low <=> $high);
var_dump($high <=> $low);
var_dump($low <=> $equal);

var_dump($low === $low);
var_dump($low === $equal);
var_dump($low !== $equal);

var_dump(comparisonProbe('z-temporary-low', 1) < comparisonProbe('a-temporary-high', 3));
var_dump(comparisonProbe('a-temporary-high', 3) >= comparisonProbe('m-temporary-middle', 2));

$leftReference =& $low;
$rightReference =& $high;
var_dump($leftReference <=> $rightReference);

$beforeSameObject = ComparisonProbe::$comparisonCount;
var_dump($low == $low);
var_dump(ComparisonProbe::$comparisonCount - $beforeSameObject);

var_dump($low == null);
var_dump(null == $low);
var_dump($low != null);
var_dump(null != $low);

$sortable = [
    comparisonProbe('high', 3),
    comparisonProbe('low', 1),
    comparisonProbe('middle', 2),
];
sort($sortable, SORT_REGULAR);
echo implode(',', array_map(fn (ComparisonProbe $probe): string => $probe->storageKey, $sortable)), PHP_EOL;
rsort($sortable, SORT_REGULAR);
echo implode(',', array_map(fn (ComparisonProbe $probe): string => $probe->storageKey, $sortable)), PHP_EOL;

$associative = [
    'high' => comparisonProbe('associative-high', 3),
    'low' => comparisonProbe('associative-low', 1),
    'middle' => comparisonProbe('associative-middle', 2),
];
asort($associative, SORT_REGULAR);
echo implode(',', array_keys($associative)), PHP_EOL;
?>
--EXPECT--
bool(true)
bool(false)
bool(true)
bool(true)
bool(true)
bool(true)
int(-1)
int(1)
int(0)
bool(true)
bool(false)
bool(true)
bool(true)
bool(true)
int(-1)
bool(true)
int(0)
bool(false)
bool(false)
bool(true)
bool(true)
low,middle,high
high,middle,low
low,middle,high
