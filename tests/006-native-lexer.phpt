--TEST--
yumemi native lexer preserves the Yumemi token contract
--EXTENSIONS--
yumemi
--SKIPIF--
<?php
$className = 'jbboehr\\Yumemi\\Parser\\NativeLexer';

if (
    class_exists($className, false)
    && method_exists($className, 'isCompatible')
    && !$className::isCompatible()
) {
    echo 'skip native lexer Unicode tables do not match runtime PCRE';
}
?>
--FILE--
<?php
$className = 'jbboehr\\Yumemi\\Parser\\NativeLexer';

var_dump(class_exists($className, false));

if (!class_exists($className, false)) {
    return;
}

$reflection = new ReflectionClass($className);
var_dump($reflection->isInternal());
var_dump($reflection->isFinal());

$cases = [
    'operators' => '12.3e-4 meter·second²/(kg @ -2) + foo.bar^⁻³',
    'mixed' => 'ab𐐀cd',
    'fallback' => 'ab💩cd',
    'digit-prefix' => '123abc',
    'unicode-digits' => '١.٢',
    'invalid' => '1.2.3 ⁺',
    'uppercase-exponents' => '1E5 2.5E-3 1.2.3E4 1.2.3E+4',
    'exponents-before-unicode' => '1.2.3E4µ 1.2.3e4µ',
    'empty' => '',
    'whitespace-only' => " \t\n\u{00a0}",
    'number-boundaries' => '.5 1. 1..2 12e 12e+',
    'mixed-script-digits' => '1٢3 ١e٢',
    'operator-before-unicode' => '-⁻ .² +µ',
    'unicode-whitespace' => "a\u{00a0}b\u{2003}c",
];

foreach ($cases as $label => $input) {
    echo '[', $label, ']', PHP_EOL;

    foreach ($className::tokenize($input) as $token) {
        echo $token['type'], '|', $token['text'], '|', $token['start'], '|', $token['end'], PHP_EOL;
    }
}

echo '[invalid-utf8]', PHP_EOL;
foreach ($className::tokenize(hex2bin('61ff62')) as $token) {
    echo $token['type'], '|', bin2hex($token['text']), '|', $token['start'], '|', $token['end'], PHP_EOL;
}
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
[operators]
decimal-number|12.3e-4|0|7
identifier|meter|8|13
mul|·|13|15
identifier|second|15|21
superscript-integer|²|21|23
div|/|23|24
left-paren|(|24|25
identifier|kg|25|27
at|@|28|29
sub|-|30|31
integer|2|31|32
right-paren|)|32|33
add|+|34|35
identifier|foo|36|39
dot|.|39|40
identifier|bar|40|43
pow|^|43|44
superscript-integer|⁻³|44|49
[mixed]
identifier|ab𐐀cd|0|8
[fallback]
identifier|ab|0|2
identifier|💩|2|6
identifier|cd|6|8
[digit-prefix]
integer|123|0|3
identifier|abc|3|6
[unicode-digits]
identifier|١.٢|0|5
[invalid]
invalid-number|1.2.3|0|5
invalid-superscript|⁺|6|9
[uppercase-exponents]
decimal-number|1E5|0|3
decimal-number|2.5E-3|4|10
identifier|1.2.3E4|11|18
identifier|1.2.3E+4|19|27
[exponents-before-unicode]
identifier|1.2.3E4|0|7
identifier|µ|7|9
invalid-number|1.2.3e4|10|17
identifier|µ|17|19
[empty]
[whitespace-only]
[number-boundaries]
dot|.|0|1
integer|5|1|2
integer|1|3|4
dot|.|4|5
integer|1|6|7
dot|.|7|8
dot|.|8|9
integer|2|9|10
integer|12|11|13
identifier|e|13|14
integer|12|15|17
identifier|e|17|18
add|+|18|19
[mixed-script-digits]
identifier|1٢3|0|4
identifier|١e٢|5|10
[operator-before-unicode]
sub|-|0|1
invalid-superscript|⁻|1|4
dot|.|5|6
superscript-integer|²|6|8
add|+|9|10
identifier|µ|10|12
[unicode-whitespace]
identifier|a|0|1
identifier|b|3|4
identifier|c|7|8
[invalid-utf8]
identifier|61ff62|0|3
