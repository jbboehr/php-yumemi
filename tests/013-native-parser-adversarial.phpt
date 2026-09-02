--TEST--
yumemi native parser remains structurally sound across deterministic adversarial inputs
--EXTENSIONS--
yumemi
--FILE--
<?php
declare(strict_types=1);

use jbboehr\Yumemi\Parser\NativeLimitException;
use jbboehr\Yumemi\Parser\NativeParseException;
use jbboehr\Yumemi\Parser\NativeParser;

final class DeterministicInputGenerator
{
    private int $state = 0x13579bdf;

    public function validExpression(int $depth = 0): string
    {
        if ($depth >= 3) {
            return $this->atom();
        }

        return match ($this->nextInt(5)) {
            0 => $this->atom(),
            1 => '-(' . $this->validExpression($depth + 1) . ')',
            2 => '--(' . $this->validExpression($depth + 1) . ')',
            3 => '(' . $this->validExpression($depth + 1) . ')',
            default => '(' . $this->validExpression($depth + 1) . ')'
                . $this->choose([' + ', ' - ', '*', '/', '^', ' ', '.', ' · '])
                . '(' . $this->validExpression($depth + 1) . ')',
        };
    }

    public function mutate(string $input): string
    {
        $offset = $this->nextInt(strlen($input) + 1);

        return match ($this->nextInt(4)) {
            0 => substr($input, 0, $offset)
                . $this->choose(["\0", "\xff", '(', ')', '@', '^', '+', '⁺'])
                . substr($input, $offset),
            1 => $input === ''
                ? ')'
                : substr($input, 0, min($offset, strlen($input) - 1))
                    . substr($input, min($offset, strlen($input) - 1) + 1),
            2 => $input === ''
                ? "\xff"
                : substr($input, 0, min($offset, strlen($input) - 1))
                    . chr($this->nextInt(256))
                    . substr($input, min($offset, strlen($input) - 1) + 1),
            default => $this->choose(['+', '/', '@', '(', '⁻']) . $input . $this->choose(['', ')', '^', '@']),
        };
    }

    public function bytes(int $maximumLength): string
    {
        $length = $this->nextInt($maximumLength + 1);
        $bytes = '';

        for ($index = 0; $index < $length; ++$index) {
            $bytes .= chr($this->nextInt(256));
        }

        return $bytes;
    }

    private function atom(): string
    {
        return match ($this->nextInt(4)) {
            0 => $this->choose(['meter', 'a_b', 'μs', '°C', 'αβ', '中文', '𐐀', "a\0b"]),
            1 => $this->choose(['0', '0001', '1.25', '1e-3', '2.5E+3']),
            2 => $this->choose(['meter²', 'second⁻²', 'μs⁰', 'αβ¹²']),
            default => $this->choose(['meter @ 0', '°C @ -273.15', 'αβ @ 1e3']),
        };
    }

    private function choose(array $values): mixed
    {
        return $values[$this->nextInt(count($values))];
    }

    private function nextInt(int $upperBound): int
    {
        $this->state = ($this->state * 1103515245 + 12345) & 0x7fffffff;

        return $this->state % $upperBound;
    }
}

function checkCondition(bool $condition, string $message): void
{
    if (!$condition) {
        throw new RuntimeException($message);
    }
}

function astShape(array $node, string $input, string $label, int &$nodeCount = 0): array
{
    ++$nodeCount;
    checkCondition($nodeCount <= 512, $label . ': AST contains too many nodes');
    checkCondition(isset($node['kind']) && is_string($node['kind']), $label . ': missing AST kind');
    checkCondition(array_key_exists('start', $node), $label . ': missing AST start');
    checkCondition(array_key_exists('end', $node), $label . ': missing AST end');

    $kind = $node['kind'];
    $start = $node['start'];
    $end = $node['end'];
    $hasSpan = is_int($start) && is_int($end);

    checkCondition(
        $hasSpan || ($start === null && $end === null),
        $label . ': AST span must contain two integers or two nulls',
    );

    if ($hasSpan) {
        checkCondition(
            0 <= $start && $start <= $end && $end <= strlen($input),
            sprintf('%s: AST span %d..%d is outside 0..%d', $label, $start, $end, strlen($input)),
        );
    }

    if (in_array($kind, ['integer', 'decimal-number', 'identifier'], true)) {
        checkCondition(isset($node['text']) && is_string($node['text']), $label . ': leaf text is missing');
        checkCondition(
            !array_key_exists('left', $node) && !array_key_exists('right', $node),
            $label . ': leaf unexpectedly has children',
        );
        checkCondition(
            $hasSpan || ($kind === 'integer' && $node['text'] === '-1'),
            $label . ': only the synthetic negative-one leaf may omit its span',
        );

        return [$kind, $node['text']];
    }

    checkCondition(
        in_array($kind, ['add', 'sub', 'mul', 'div', 'pow', 'at'], true),
        $label . ': unknown AST kind ' . $kind,
    );
    checkCondition($hasSpan, $label . ': binary AST node is missing its span');
    checkCondition(isset($node['left']) && is_array($node['left']), $label . ': binary left child is missing');
    checkCondition(isset($node['right']) && is_array($node['right']), $label . ': binary right child is missing');
    checkCondition(!array_key_exists('text', $node), $label . ': binary node unexpectedly has text');

    foreach ([$node['left'], $node['right']] as $child) {
        if (is_int($child['start'] ?? null) && is_int($child['end'] ?? null)) {
            checkCondition(
                $start <= $child['start'] && $child['end'] <= $end,
                $label . ': child span lies outside its parent span',
            );
        }
    }

    return [
        $kind,
        astShape($node['left'], $input, $label, $nodeCount),
        astShape($node['right'], $input, $label, $nodeCount),
    ];
}

function parseShape(string $input, string $label): array
{
    $nodeCount = 0;

    return astShape(NativeParser::parse($input), $input, $label, $nodeCount);
}

function exerciseAdversarialInput(
    string $input,
    string $label,
    int &$accepted,
    int &$rejected,
): void {
    try {
        parseShape($input, $label);
        ++$accepted;
    } catch (NativeParseException $exception) {
        ++$rejected;
        checkCondition($exception->input === $input, $label . ': parse exception lost its input');
        checkCondition(
            0 <= $exception->start
                && $exception->start <= $exception->end
                && $exception->end <= strlen($input),
            $label . ': parse exception span is outside its input',
        );
        checkCondition(
            $exception->unexpected === null || is_string($exception->unexpected),
            $label . ': unexpected-token metadata has the wrong type',
        );
        foreach ($exception->expected as $expected) {
            checkCondition(is_string($expected), $label . ': expected-token metadata has the wrong type');
        }

        checkCondition(
            parseShape('meter', $label . '-reset') === ['identifier', 'meter'],
            $label . ': parser state leaked after a syntax failure',
        );
    } catch (NativeLimitException $exception) {
        throw new RuntimeException($label . ': bounded input unexpectedly reached ' . $exception->limit, 0, $exception);
    } catch (Throwable $throwable) {
        throw new RuntimeException($label . ': unexpected ' . $throwable::class, 0, $throwable);
    }
}

$generator = new DeterministicInputGenerator();
$validInputs = [];

for ($index = 0; $index < 256; ++$index) {
    $input = $generator->validExpression();
    $label = sprintf('valid-%03d; bytes=%s', $index, bin2hex($input));
    checkCondition(strlen($input) <= 1024, $label . ': generator exceeded its input bound');

    $ast = NativeParser::parse($input);
    $nodeCount = 0;
    $shape = astShape($ast, $input, $label, $nodeCount);
    checkCondition(NativeParser::parse($input) === $ast, $label . ': repeated parse changed the AST');
    checkCondition(
        parseShape(" \t(" . $input . ")\n", $label . '-wrapped') === $shape,
        $label . ': grouping or surrounding whitespace changed the AST shape',
    );

    $validInputs[] = $input;
}

$adversarialInputs = [
    '',
    ' ',
    '+',
    '/',
    '@',
    '(',
    ')',
    'meter /',
    'meter @ )',
    '1.2.3',
    '⁺',
    "\x80",
    "\xc0\xaf",
    "\xe0\x80\x80",
    "\xed\xa0\x80",
    "\xf4\x90\x80\x80",
    "meter\xff/second",
    str_repeat('(', 64),
];

while (count($adversarialInputs) < 512) {
    $index = count($adversarialInputs);
    $adversarialInputs[] = $index % 3 === 0
        ? $generator->bytes(96)
        : $generator->mutate($validInputs[$index % count($validInputs)]);
}

$accepted = 0;
$rejected = 0;

foreach ($adversarialInputs as $index => $input) {
    exerciseAdversarialInput(
        $input,
        sprintf('adversarial-%03d; bytes=%s', $index, bin2hex($input)),
        $accepted,
        $rejected,
    );
}

checkCondition($accepted >= 64, 'adversarial corpus did not exercise enough accepted inputs');
checkCondition($rejected >= 64, 'adversarial corpus did not exercise enough structured failures');

echo "valid:256\n";
echo "adversarial:512\n";
echo "mixed-outcomes:yes\n";
echo "post-corpus:", NativeParser::parse('meter')['text'], "\n";
?>
--EXPECT--
valid:256
adversarial:512
mixed-outcomes:yes
post-corpus:meter
