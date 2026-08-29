--TEST--
yumemi native parser errors expose machine-readable syntax and resource metadata
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
$parserClass = 'jbboehr\\Yumemi\\Parser\\NativeParser';
$parseExceptionClass = 'jbboehr\\Yumemi\\Parser\\NativeParseException';
$limitExceptionClass = 'jbboehr\\Yumemi\\Parser\\NativeLimitException';

$limitClassExists = class_exists($limitExceptionClass, false);
var_dump($limitClassExists);
var_dump($limitClassExists && (new ReflectionClass($limitExceptionClass))->isInternal());
var_dump($limitClassExists && (new ReflectionClass($limitExceptionClass))->isFinal());
var_dump($limitClassExists && is_subclass_of($limitExceptionClass, LengthException::class));

$metadataContracts = [
    'syntax' => [
        $parseExceptionClass,
        [
            'input' => 'string',
            'start' => 'int',
            'end' => 'int',
            'unexpected' => '?string',
            'expected' => 'array',
        ],
    ],
    'limit' => [
        $limitExceptionClass,
        [
            'limit' => 'string',
            'maximum' => 'int',
            'observed' => 'int',
            'start' => 'int',
            'end' => 'int',
        ],
    ],
];

foreach ($metadataContracts as $label => [$class, $properties]) {
    $reflection = new ReflectionClass($class);
    $matches = true;
    foreach ($properties as $name => $type) {
        $property = $reflection->getProperty($name);
        $matches = $matches
            && $property->isPublic()
            && $property->isReadOnly()
            && (string) $property->getType() === $type;
    }
    echo $label, '-metadata-contract:', $matches ? 'typed-readonly' : 'wrong', PHP_EOL;
}

$syntaxFailures = [
    'initial-token' => [
        '+',
        0,
        1,
        '+',
        ['integer', 'decimal number', '-', 'identifier', '('],
        'syntax error, unexpected + at bytes 0..1',
    ],
    'unexpected-token' => [
        'meter @ )',
        8,
        9,
        ')',
        ['integer', 'decimal number', '-'],
        'syntax error, unexpected ), expecting integer or decimal number or - at bytes 8..9',
    ],
    'end-of-input' => [
        'meter /',
        7,
        7,
        'end of file',
        ['integer', 'decimal number', '-', 'identifier', '('],
        'syntax error, unexpected end of file at bytes 7..7',
    ],
    'group-end-of-input' => [
        '(meter',
        6,
        6,
        'end of file',
        [')'],
        'syntax error, unexpected end of file, expecting ) at bytes 6..6',
    ],
    'invalid-token-name' => [
        'meter ⁺',
        6,
        9,
        'superscript sign without digits',
        ['end of file'],
        'syntax error, unexpected superscript sign without digits, expecting end of file at bytes 6..9',
    ],
];

foreach ($syntaxFailures as $label => [$input, $start, $end, $unexpected, $expected, $message]) {
    try {
        $parserClass::parse($input);
        echo $label, ":no failure\n";
    } catch (Throwable $exception) {
        $actualUnexpected = property_exists($exception, 'unexpected') ? $exception->unexpected : null;
        $actualExpected = property_exists($exception, 'expected') ? $exception->expected : null;
        $structured = $exception::class === $parseExceptionClass
            && $exception->input === $input
            && $exception->start === $start
            && $exception->end === $end
            && $actualUnexpected === $unexpected
            && $actualExpected === $expected;
        echo $label,
            ':',
            $structured ? 'structured' : 'wrong',
            ':',
            $exception->getMessage() === $message ? 'message-compatible' : 'message-changed',
            PHP_EOL;
    }
}

try {
    $parserClass::parse('meter @ )');
} catch (Throwable $first) {
    try {
        $first->unexpected = 'mutated';
        echo "syntax-metadata:mutable\n";
    } catch (Error) {
        echo "syntax-metadata:readonly\n";
    }

    try {
        $first->expected[] = 'mutated';
        echo "syntax-expected-metadata:mutable\n";
    } catch (Error) {
        echo "syntax-expected-metadata:readonly\n";
    }
}

try {
    $parserClass::parse('meter @ )');
} catch (Throwable $second) {
    $isolated = $second->unexpected === ')'
        && $second->expected === ['integer', 'decimal number', '-'];
    echo 'syntax-object-isolation:', $isolated ? 'isolated' : 'shared', PHP_EOL;
}

$limitFailures = [
    'input-bytes' => [str_repeat('m', 4097), 4096, 4097, 0, 4097],
    'token-count' => [implode(' ', array_fill(0, 257, 'm')), 256, 257, 512, 513],
    'nesting-depth' => [str_repeat('(', 65), 64, 65, 64, 65],
    'token-bytes' => [str_repeat('m', 1025), 1024, 1025, 0, 1025],
];

foreach ($limitFailures as $limit => [$input, $maximum, $observed, $start, $end]) {
    try {
        $parserClass::parse($input);
        echo $limit, ":no failure\n";
    } catch (Throwable $exception) {
        $structured = $exception::class === $limitExceptionClass
            && property_exists($exception, 'limit')
            && $exception->limit === $limit
            && $exception->maximum === $maximum
            && $exception->observed === $observed
            && $exception->start === $start
            && $exception->end === $end;
        echo $limit, ':', $structured ? 'structured' : 'wrong', PHP_EOL;
    }
}

for ($attempt = 0; $attempt < 100; ++$attempt) {
    try {
        $parserClass::parse($attempt % 2 === 0 ? 'meter @ )' : str_repeat('(', 65));
    } catch (Throwable) {
    }
}
echo 'repeated-failure-reset:', $parserClass::parse('meter')['text'], PHP_EOL;
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
syntax-metadata-contract:typed-readonly
limit-metadata-contract:typed-readonly
initial-token:structured:message-compatible
unexpected-token:structured:message-compatible
end-of-input:structured:message-compatible
group-end-of-input:structured:message-compatible
invalid-token-name:structured:message-compatible
syntax-metadata:readonly
syntax-expected-metadata:readonly
syntax-object-isolation:isolated
input-bytes:structured
token-count:structured
nesting-depth:structured
token-bytes:structured
repeated-failure-reset:meter
