# Native Parser ABI

The native parser interface is an internal, versioned integration seam between php-yumemi and yumemi.php. Applications
must not consume its arrays or exceptions directly.

## Selection contract

The current ABI version is `1`, exposed as
`jbboehr\Yumemi\Parser\NativeParser::ABI_VERSION`. A compatible yumemi.php adapter selects the native parser only when:

1. `NativeParser` is already loaded, without triggering autoload;
2. `ABI_VERSION === 1`;
3. `NativeParser::isCompatible()` returns `true`; and
4. the process-level `YUMEMI_NATIVE_PARSER` setting permits native parsing.

Every failed check selects the generated PHP parser. Native parsing is an optimization and never a semantic dependency.
The setting is case-insensitive: unset, `1`, `true`, `on`, and `yes` permit native selection, while `0`, `false`, `off`,
`no`, and the empty string select the PHP fallback. Any other explicit value also fails closed to the fallback.

`supports(int $abiVersion)` is the preferred gate for the next coordinated adapter update. It returns `true` only when
the requested ABI equals the installed ABI and PHP's runtime `PCRE_VERSION` exactly matches the Unicode tables committed
with the extension. This prevents an adapter from checking one condition but accidentally omitting the other. It also
prevents the native lexer and yumemi.php's PCRE-based lexer from assigning different boundaries to the same Unicode
identifier.

`ABI_VERSION` and `isCompatible()` remain available as inspection seams. `isCompatible()` reports only the Unicode
condition. The current yumemi.php adapter still checks these two seams separately. Once it migrates to `supports()`, an
older extension without that method will be incompatible and must fall back to the generated PHP parser.

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

The exception message retains Bison's detailed human-readable diagnostic and appends the byte span. Consumers translate
the structured properties rather than parsing that prose.

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

## Native lexer seam

`NativeLexer::tokenize(string $input): array` remains independently available for compatibility and differential tests.
Each token contains a machine-readable `type`, exact `text`, and zero-based half-open `start` and `end` byte
offsets. Returned token types are `integer`, `superscript-integer`, `invalid-superscript`, `decimal-number`, `dot`,
`mul`, `div`, `pow`, `sub`, `add`, `identifier`, `left-paren`, `right-paren`, `at`, and `invalid-number`. The lexer
shares the parser's Unicode compatibility gate and resource limits.

The lexer seam is not a second supported application API. yumemi.php selects `NativeParser`, not `NativeLexer`, for
ordinary parsing.

## Compatibility changes

The current yumemi.php adapter accepts ABI version `1` exactly and rejects malformed nodes even when the version
matches. Any incompatible change to node kinds, required fields, span meaning, or structured failure metadata requires
a new ABI integer. While this seam remains internal and experimental, a coordinated release may stop accepting an older
ABI without preserving it in the extension; yumemi.php must retain its fail-closed PHP fallback and name compatible
extension versions in its release notes.
