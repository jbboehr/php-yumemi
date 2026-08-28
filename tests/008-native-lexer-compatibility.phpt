--TEST--
yumemi native syntax components fail closed when Unicode tables cannot match runtime PCRE
--EXTENSIONS--
yumemi
--FILE--
<?php
$className = 'jbboehr\\Yumemi\\Parser\\NativeLexer';
$parserClass = 'jbboehr\\Yumemi\\Parser\\NativeParser';

var_dump(is_string($className::UNICODE_PCRE_VERSION));

$compatible = $className::isCompatible();
var_dump(is_bool($compatible));
var_dump($compatible === ($className::UNICODE_PCRE_VERSION === PCRE_VERSION));

try {
    $className::tokenize("a\u{1e5d0}b");
    $outcome = 'tokenized';
} catch (RuntimeException $exception) {
    $outcome = str_contains($exception->getMessage(), $className::UNICODE_PCRE_VERSION)
        && str_contains($exception->getMessage(), PCRE_VERSION)
            ? 'guarded'
            : 'wrong-exception';
}

var_dump($outcome === ($compatible ? 'tokenized' : 'guarded'));

var_dump($parserClass::isCompatible() === $compatible);

try {
    $parserClass::parse("a\u{1e5d0}b");
    $outcome = 'parsed';
} catch (RuntimeException $exception) {
    $outcome = str_contains($exception->getMessage(), $className::UNICODE_PCRE_VERSION)
        && str_contains($exception->getMessage(), PCRE_VERSION)
            ? 'guarded'
            : 'wrong-exception';
}

var_dump($outcome === ($compatible ? 'parsed' : 'guarded'));
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
