--TEST--
yumemi native syntax components fail closed when Unicode tables cannot match runtime PCRE
--EXTENSIONS--
yumemi
--FILE--
<?php
declare(strict_types=1);

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

$hasSupports = method_exists($parserClass, 'supports');
var_dump($hasSupports);

if (!$hasSupports) {
    return;
}

$supports = new ReflectionMethod($parserClass, 'supports');
$parameters = $supports->getParameters();

var_dump($supports->isPublic());
var_dump($supports->isStatic());
var_dump($supports->getNumberOfRequiredParameters() === 1);
var_dump(count($parameters) === 1
    && $parameters[0]->getName() === 'abiVersion'
    && (string) $parameters[0]->getType() === 'int'
    && !$parameters[0]->allowsNull());
var_dump((string) $supports->getReturnType() === 'bool' && !$supports->getReturnType()->allowsNull());

var_dump($parserClass::supports($parserClass::ABI_VERSION) === $compatible);
var_dump($parserClass::supports(0));
var_dump($parserClass::supports($parserClass::ABI_VERSION + 1));

$exactForAllBoundaries = true;
foreach ([PHP_INT_MIN, -1, 0, $parserClass::ABI_VERSION - 1, $parserClass::ABI_VERSION,
          $parserClass::ABI_VERSION + 1, PHP_INT_MAX] as $abiVersion) {
    $expected = $abiVersion === $parserClass::ABI_VERSION && $compatible;
    $exactForAllBoundaries = $exactForAllBoundaries && $parserClass::supports($abiVersion) === $expected;
}
var_dump($exactForAllBoundaries);

try {
    $parserClass::supports('1');
    $strictTypeRejected = false;
} catch (TypeError) {
    $strictTypeRejected = true;
}
var_dump($strictTypeRejected);

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
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(false)
bool(false)
bool(true)
bool(true)
bool(true)
