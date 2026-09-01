--TEST--
yumemi native parser exposes structured syntax failures and preserves lexer limits
--EXTENSIONS--
yumemi
--FILE--
<?php
$parserClass = 'jbboehr\\Yumemi\\Parser\\NativeParser';
$exceptionClass = 'jbboehr\\Yumemi\\Parser\\NativeParseException';

var_dump(class_exists($exceptionClass, false));
$reflection = new ReflectionClass($exceptionClass);
var_dump($reflection->isInternal());
var_dump($reflection->isFinal());
var_dump($reflection->isSubclassOf(RuntimeException::class));

$invalid = [
    'empty' => ['', 0, 0],
    'whitespace-only' => [' ', 1, 1],
    'trailing-operator' => ['m +', 3, 3],
    'unclosed-group' => ['(m', 2, 2],
    'invalid-number' => ['1.2.3', 0, 5],
    'invalid-superscript' => ['⁺', 0, 3],
    'invalid-offset' => ['m @ s', 4, 5],
    'extra-close' => ['m )', 2, 3],
    'multibyte-prefix' => ['° * / second', 5, 6],
];

foreach ($invalid as $label => [$input, $start, $end]) {
    try {
        $parserClass::parse($input);
        echo $label, ":no failure\n";
    } catch (Throwable $exception) {
        $structured = $exception::class === $exceptionClass
            && $exception->input === $input
            && $exception->start === $start
            && $exception->end === $end
            && str_contains(strtolower($exception->getMessage()), 'syntax error');
        echo $label, ':', $structured ? 'structured' : 'wrong', PHP_EOL;
    }
}

$limits = [
    'input-bytes' => str_repeat('m', 4097),
    'token-count' => implode(' ', array_fill(0, 257, 'm')),
    'nesting-depth' => str_repeat('(', 65),
    'token-bytes' => str_repeat('m', 1025),
];

foreach ($limits as $category => $input) {
    try {
        $parserClass::parse($input);
        echo $category, ":no failure\n";
    } catch (LengthException $exception) {
        echo $category, ':', str_contains($exception->getMessage(), $category) ? 'limited' : 'wrong', PHP_EOL;
    }
}

$boundaries = [
    'input-bytes-boundary' => str_repeat(' ', 4095) . 'm',
    'token-count-boundary' => str_repeat('-', 255) . 'm',
    'nesting-depth-boundary' => str_repeat('(', 64) . 'm' . str_repeat(')', 64),
    'token-bytes-boundary' => str_repeat('m', 1024),
];

foreach ($boundaries as $category => $input) {
    try {
        $parserClass::parse($input);
        echo $category, ":accepted\n";
    } catch (Throwable $exception) {
        echo $category, ':wrong:', $exception::class, PHP_EOL;
    }
}

echo 'post-limit-reset:', $parserClass::parse('m')['text'], PHP_EOL;
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
empty:structured
whitespace-only:structured
trailing-operator:structured
unclosed-group:structured
invalid-number:structured
invalid-superscript:structured
invalid-offset:structured
extra-close:structured
multibyte-prefix:structured
input-bytes:limited
token-count:limited
nesting-depth:limited
token-bytes:limited
input-bytes-boundary:accepted
token-count-boundary:accepted
nesting-depth-boundary:accepted
token-bytes-boundary:accepted
post-limit-reset:m
