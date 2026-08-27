--TEST--
yumemi native lexer enforces parser resource limits before returning tokens
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

if (!class_exists($className, false)) {
    echo "missing\n";
    return;
}

$cases = [
    'input-bytes' => [
        str_repeat('m', 4097),
        'Yumemi parser input-bytes limit exceeded: limit 4096, observed 4097 at bytes 0..4097',
    ],
    'token-count' => [
        implode(' ', array_fill(0, 257, 'm')),
        'Yumemi parser token-count limit exceeded: limit 256, observed 257 at bytes 512..513',
    ],
    'nesting-depth' => [
        str_repeat('(', 65),
        'Yumemi parser nesting-depth limit exceeded: limit 64, observed 65 at bytes 64..65',
    ],
    'token-bytes' => [
        str_repeat('m', 1025),
        'Yumemi parser token-bytes limit exceeded: limit 1024, observed 1025 at bytes 0..1025',
    ],
];

foreach ($cases as $category => [$input, $expectedMessage]) {
    try {
        $className::tokenize($input);
        echo $category, ":no failure\n";
    } catch (LengthException $exception) {
        echo $category, ':', $exception::class, ':', $exception->getMessage() === $expectedMessage ? 'details' : 'wrong', PHP_EOL;
    }
}
?>
--EXPECT--
input-bytes:LengthException:details
token-count:LengthException:details
nesting-depth:LengthException:details
token-bytes:LengthException:details
