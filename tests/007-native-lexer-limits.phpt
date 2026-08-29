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
        [4096, 4097, 0, 4097],
    ],
    'token-count' => [
        implode(' ', array_fill(0, 257, 'm')),
        'Yumemi parser token-count limit exceeded: limit 256, observed 257 at bytes 512..513',
        [256, 257, 512, 513],
    ],
    'nesting-depth' => [
        str_repeat('(', 65),
        'Yumemi parser nesting-depth limit exceeded: limit 64, observed 65 at bytes 64..65',
        [64, 65, 64, 65],
    ],
    'token-bytes' => [
        str_repeat('m', 1025),
        'Yumemi parser token-bytes limit exceeded: limit 1024, observed 1025 at bytes 0..1025',
        [1024, 1025, 0, 1025],
    ],
];

foreach ($cases as $category => [$input, $expectedMessage, $metadata]) {
    try {
        $className::tokenize($input);
        echo $category, ":no failure\n";
    } catch (LengthException $exception) {
        [$maximum, $observed, $start, $end] = $metadata;
        $details = $exception->getMessage() === $expectedMessage
            && $exception->limit === $category
            && $exception->maximum === $maximum
            && $exception->observed === $observed
            && $exception->start === $start
            && $exception->end === $end;
        echo $category, ':', $exception::class, ':', $details ? 'details' : 'wrong', PHP_EOL;
    }
}

try {
    $className::tokenize(str_repeat('(', 65));
} catch (LengthException $first) {
    try {
        $first->limit = 'mutated';
        echo "limit-metadata:mutable\n";
    } catch (Error) {
        echo "limit-metadata:readonly\n";
    }
}

try {
    $className::tokenize(str_repeat('(', 65));
} catch (LengthException $second) {
    $isolated = $second->limit === 'nesting-depth'
        && $second->maximum === 64
        && $second->observed === 65
        && $second->start === 64
        && $second->end === 65;
    echo 'limit-object-isolation:', $isolated ? 'isolated' : 'shared', PHP_EOL;
}

$boundaries = [
    'input-bytes-boundary' => str_repeat(' ', 4095) . 'm',
    'token-count-boundary' => implode(' ', array_fill(0, 256, 'm')),
    'nesting-depth-boundary' => str_repeat('(', 64) . 'm' . str_repeat(')', 64),
    'token-bytes-boundary' => str_repeat('m', 1024),
];

foreach ($boundaries as $category => $input) {
    try {
        $className::tokenize($input);
        echo $category, ":accepted\n";
    } catch (Throwable $exception) {
        echo $category, ':wrong:', $exception::class, PHP_EOL;
    }
}

echo 'post-limit-reset:', $className::tokenize('m')[0]['text'], PHP_EOL;
?>
--EXPECT--
input-bytes:jbboehr\Yumemi\Parser\NativeLimitException:details
token-count:jbboehr\Yumemi\Parser\NativeLimitException:details
nesting-depth:jbboehr\Yumemi\Parser\NativeLimitException:details
token-bytes:jbboehr\Yumemi\Parser\NativeLimitException:details
limit-metadata:readonly
limit-object-isolation:isolated
input-bytes-boundary:accepted
token-count-boundary:accepted
nesting-depth-boundary:accepted
token-bytes-boundary:accepted
post-limit-reset:m
