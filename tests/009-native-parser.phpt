--TEST--
yumemi native parser exposes a neutral span-preserving AST for parity testing
--EXTENSIONS--
yumemi
--SKIPIF--
<?php
$lexerClass = 'jbboehr\\Yumemi\\Parser\\NativeLexer';

if (class_exists($lexerClass, false) && !$lexerClass::isCompatible()) {
    echo 'skip native lexer Unicode tables do not match runtime PCRE';
}
?>
--FILE--
<?php
$className = 'jbboehr\\Yumemi\\Parser\\NativeParser';

var_dump(class_exists($className, false));

if (!class_exists($className, false)) {
    return;
}

$reflection = new ReflectionClass($className);
var_dump($reflection->isInternal());
var_dump($reflection->isFinal());
var_dump($className::ABI_VERSION);
var_dump($className::isCompatible());

function describeAst(array $node): string
{
    $span = $node['start'] === null ? '-' : $node['start'] . '..' . $node['end'];

    if (array_key_exists('text', $node)) {
        return $node['kind'] . '(' . $node['text'] . ')@' . $span;
    }

    return $node['kind'] . '(' . describeAst($node['left']) . ',' . describeAst($node['right']) . ')@' . $span;
}

function describeLeafBytes(array $node): string
{
    if (array_key_exists('text', $node)) {
        return $node['kind'] . ':' . bin2hex($node['text']) . '@' . $node['start'] . '..' . $node['end'];
    }

    return describeLeafBytes($node['left']) . ',' . describeLeafBytes($node['right']);
}

$cases = [
    'precedence' => 'm/s^2',
    'complete' => '2 m·s² + °C @ -273.15',
    'negation' => '-m',
    'double-negation' => '--2',
    'unary-power' => '-m^2',
    'grouping' => '(m/s)',
    'right-power' => 'm^s^2',
    'left-product' => 'm/s kg',
    'left-additive' => 'm + s - kg',
    'exact-lexemes' => '0001.2300e+04 foo_02',
    'superscript-digits' => 'm⁻⁰¹²³⁴⁵⁶⁷⁸⁹',
    'uppercase-invalid-number' => '1.2.3E4',
    'truncated-exponent' => '12e',
];

foreach ($cases as $label => $input) {
    echo $label, ':', describeAst($className::parse($input)), PHP_EOL;
}

echo 'nul-bytes:', describeLeafBytes($className::parse("a\0b")), PHP_EOL;
echo 'invalid-utf8-bytes:', describeLeafBytes($className::parse("a\xffb")), PHP_EOL;
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
int(1)
bool(true)
precedence:div(identifier(m)@0..1,pow(identifier(s)@2..3,integer(2)@4..5)@2..5)@0..5
complete:add(mul(mul(integer(2)@0..1,identifier(m)@2..3)@0..3,pow(identifier(s)@5..6,integer(2)@6..8)@5..8)@0..8,at(identifier(°C)@11..14,decimal-number(-273.15)@17..24)@11..24)@0..24
negation:mul(integer(-1)@-,identifier(m)@1..2)@0..2
double-negation:integer(2)@0..3
unary-power:mul(integer(-1)@-,pow(identifier(m)@1..2,integer(2)@3..4)@1..4)@0..4
grouping:div(identifier(m)@1..2,identifier(s)@3..4)@1..4
right-power:pow(identifier(m)@0..1,pow(identifier(s)@2..3,integer(2)@4..5)@2..5)@0..5
left-product:mul(div(identifier(m)@0..1,identifier(s)@2..3)@0..3,identifier(kg)@4..6)@0..6
left-additive:sub(add(identifier(m)@0..1,identifier(s)@4..5)@0..5,identifier(kg)@8..10)@0..10
exact-lexemes:mul(decimal-number(0001.2300e+04)@0..13,identifier(foo_02)@14..20)@0..20
superscript-digits:pow(identifier(m)@0..1,integer(-0123456789)@1..31)@0..31
uppercase-invalid-number:identifier(1.2.3E4)@0..7
truncated-exponent:mul(integer(12)@0..2,identifier(e)@2..3)@0..3
nul-bytes:identifier:61@0..1,identifier:00@1..2,identifier:62@2..3
invalid-utf8-bytes:identifier:61ff62@0..3
