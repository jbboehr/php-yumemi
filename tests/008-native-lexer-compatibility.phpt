--TEST--
yumemi native syntax components use their committed Unicode tables independently of runtime PCRE
--EXTENSIONS--
yumemi
--FILE--
<?php
declare(strict_types=1);

$className = 'jbboehr\\Yumemi\\Parser\\NativeLexer';
$parserClass = 'jbboehr\\Yumemi\\Parser\\NativeParser';

function hasLegacyCompatibilityHook(string $class): bool
{
    $method = new ReflectionMethod($class, 'isCompatible');
    $returnType = $method->getReturnType();

    return $method->isPublic()
        && $method->isStatic()
        && $method->getNumberOfParameters() === 0
        && $method->getNumberOfRequiredParameters() === 0
        && $returnType instanceof ReflectionNamedType
        && (string) $returnType === 'bool'
        && !$returnType->allowsNull();
}

var_dump($className::UNICODE_PCRE_VERSION === '10.46 2025-08-27');
var_dump(hasLegacyCompatibilityHook($className));
var_dump($className::isCompatible());

try {
    $tokens = $className::tokenize("a\u{1e5d0}b");
    $lexerAvailable = count($tokens) === 1
        && $tokens[0]['type'] === 'identifier'
        && $tokens[0]['text'] === "a\u{1e5d0}b";
} catch (RuntimeException) {
    $lexerAvailable = false;
}

var_dump($lexerAvailable);

var_dump(hasLegacyCompatibilityHook($parserClass));
var_dump($parserClass::isCompatible());

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

var_dump($parserClass::supports($parserClass::ABI_VERSION));
var_dump($parserClass::supports(0));
var_dump($parserClass::supports($parserClass::ABI_VERSION + 1));

$exactForAllBoundaries = true;
foreach ([PHP_INT_MIN, -1, 0, $parserClass::ABI_VERSION - 1, $parserClass::ABI_VERSION,
          $parserClass::ABI_VERSION + 1, PHP_INT_MAX] as $abiVersion) {
    $expected = $abiVersion === $parserClass::ABI_VERSION;
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
    $ast = $parserClass::parse("a\u{1e5d0}b");
    $parserAvailable = $ast['kind'] === 'identifier'
        && $ast['text'] === "a\u{1e5d0}b"
        && $ast['start'] === 0
        && $ast['end'] === 6;
} catch (RuntimeException) {
    $parserAvailable = false;
}

var_dump($parserAvailable);
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
bool(true)
bool(false)
bool(false)
bool(true)
bool(true)
bool(true)
