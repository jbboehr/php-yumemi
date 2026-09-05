--TEST--
yumemi native parser releases Zend-managed syntax-error formatting memory
--EXTENSIONS--
yumemi
--FILE--
<?php
declare(strict_types=1);

use jbboehr\Yumemi\Parser\NativeParseException;
use jbboehr\Yumemi\Parser\NativeParser;

function consumeSyntaxFailure(string $input): void
{
    try {
        NativeParser::parse($input);
    } catch (NativeParseException) {
        return;
    }

    throw new RuntimeException('Expected a syntax failure');
}

for ($attempt = 0; $attempt < 100; ++$attempt) {
    consumeSyntaxFailure('meter @ )');
    consumeSyntaxFailure('+');
}

gc_collect_cycles();
$before = memory_get_usage(false);

for ($attempt = 0; $attempt < 5_000; ++$attempt) {
    consumeSyntaxFailure($attempt % 2 === 0 ? 'meter @ )' : '+');
}

gc_collect_cycles();
$retained = memory_get_usage(false) - $before;

echo 'syntax-format-memory:', $retained <= 65_536 ? 'bounded' : 'grew', PHP_EOL;
?>
--EXPECT--
syntax-format-memory:bounded
