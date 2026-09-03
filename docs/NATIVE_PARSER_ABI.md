# Native Parser ABI

The native parser interface is versioned and internal to php-yumemi and yumemi.php. Applications should not use its
arrays or exceptions directly.

## Selection contract

The current ABI version is `1`, exposed as
`jbboehr\Yumemi\Parser\NativeParser::ABI_VERSION`. A compatible yumemi.php adapter selects the native parser only when:

1. the process-level `YUMEMI_NATIVE_PARSER` setting permits native parsing.
2. `NativeParser` is already loaded, without triggering autoload.
3. `NativeParser::supports(1)` exists and returns `true`.

If any check fails, yumemi.php uses the generated PHP parser. Native parsing is an optimization, not a dependency.
The setting is case-insensitive: unset, `1`, `true`, `on`, and `yes` permit native selection, while `0`, `false`, `off`,
`no`, and the empty string select the PHP fallback. Any other explicit value also fails closed to the fallback.

`supports(int $abiVersion)` is the current atomic compatibility check. It returns `true` only when the requested ABI
equals the installed ABI.

`ABI_VERSION` and `isCompatible()` remain available for older adapters. `isCompatible()` always returns `true` because
the extension no longer disables native parsing based on PHP's runtime PCRE version. The current adapter sends an older
extension without `supports()` to the generated PHP parser.

## Unicode snapshot

The native lexer uses committed Unicode classification tables generated from the PCRE version exposed as
`NativeLexer::UNICODE_PCRE_VERSION`. That constant records the source of the tables. It is not a runtime compatibility
requirement.

yumemi.php's generated lexer classifies Unicode with the PCRE version loaded by PHP. A PCRE release with different
Unicode data can therefore classify a newly added or reclassified code point differently from the native lexer. This
can change token boundaries for rare identifiers that contain those code points. The extension accepts that edge case
so native parsing remains available across normal PHP installations. Regenerating the committed tables is an
intentional parser-compatibility change and requires differential testing against yumemi.php.

## Parser result

`NativeParser::parse(string $input): array` returns one nested root node. All source spans are zero-based, half-open
byte offsets into the original input.

### Leaf node

```php
[
    'kind' => 'identifier',
    'start' => 0,
    'end' => 5,
    'text' => 'meter',
]
```

Leaf nodes contain:

| Field | Type | Meaning |
| --- | --- | --- |
| `kind` | `string` | ABI token or synthesized leaf kind |
| `start` | `int|null` | Inclusive source byte offset |
| `end` | `int|null` | Exclusive source byte offset |
| `text` | `string` | Exact source lexeme, or synthesized text |

The leaf kinds in ABI 1 are `integer`, `decimal-number`, and `identifier`. Synthesized nodes, such as the `-1` left
operand used to represent negation of a non-numeric expression, have null spans.

### Binary node

```php
[
    'kind' => 'mul',
    'start' => 0,
    'end' => 14,
    'left' => [/* node */],
    'right' => [/* node */],
]
```

Binary nodes contain:

| Field | Type | Meaning |
| --- | --- | --- |
| `kind` | `string` | ABI operator kind |
| `start` | `int|null` | Inclusive source byte offset |
| `end` | `int|null` | Exclusive source byte offset |
| `left` | `array` | Left child node |
| `right` | `array` | Right child node |

The binary kinds in ABI 1 are `add`, `sub`, `mul`, `div`, `pow`, and `at`.

The native parser does not resolve identifiers, consult registries, normalize units, or construct yumemi.php objects.
The yumemi.php adapter validates every field before translating the neutral arrays into its existing AST classes.

## Syntax failures

Invalid syntax throws the internal `NativeParseException`, a final `RuntimeException` descendant with natively typed,
public readonly metadata:

| Property | Type at runtime | Meaning |
| --- | --- | --- |
| `input` | `string` | Original parser input |
| `start` | `int` | Inclusive failure byte offset |
| `end` | `int` | Exclusive failure byte offset |
| `unexpected` | `string|null` | Bison grammar-symbol label for the unexpected token |
| `expected` | `list<string>` | Bison grammar-symbol labels accepted at the failure point |

The exception message includes Bison's diagnostic and the byte span. Consumers should translate the structured
properties instead of parsing the message.

These labels are not parser AST `kind` values or native-lexer `type` values. They use the grammar's display names, such
as `decimal number`, `end of file`, and `superscript sign without digits`, plus literal punctuation such as `(` or `)`.

## Resource failures

Input, token count, nesting depth, and token size are bounded before semantic resolution. A violation throws the
internal `NativeLimitException`, a final `LengthException` descendant with natively typed, public readonly metadata:

| Property | Type at runtime | Meaning |
| --- | --- | --- |
| `limit` | `string` | `input-bytes`, `token-count`, `nesting-depth`, or `token-bytes` |
| `maximum` | `int` | Configured maximum |
| `observed` | `int` | Observed input value |
| `start` | `int` | Inclusive source byte offset |
| `end` | `int` | Exclusive source byte offset |

yumemi.php translates these values into its public parser-limit exception contract.

## Native lexer interface

`NativeLexer::tokenize(string $input): array` is available for compatibility and differential tests. Each token contains
a machine-readable `type`, exact `text`, and zero-based half-open `start` and `end` byte offsets. Returned token types are
`integer`, `superscript-integer`, `invalid-superscript`, `decimal-number`, `dot`,
`mul`, `div`, `pow`, `sub`, `add`, `identifier`, `left-paren`, `right-paren`, `at`, and `invalid-number`. The lexer
shares the parser's committed Unicode snapshot and resource limits.

The lexer is not a second application API. yumemi.php selects `NativeParser`, not `NativeLexer`, for normal parsing.

## Compatibility changes

The current yumemi.php adapter accepts only ABI version `1` and still rejects malformed nodes when the version matches.
Any incompatible change to node kinds, required fields, span meaning, or structured failure metadata requires a new ABI
integer. Because the interface is internal and experimental, a coordinated release may drop an older ABI. yumemi.php
must keep its fail-closed PHP fallback and name compatible extension versions in its release notes.
