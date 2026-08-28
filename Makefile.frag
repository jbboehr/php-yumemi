.PHONY: generate-lexer generate-parser generate-sources check-generated-lexer check-generated-parser check-generated-sources

generate-lexer:
	"$(srcdir)/scripts/generate-lexer.sh"

generate-parser:
	"$(srcdir)/scripts/generate-parser.sh"

generate-sources: generate-lexer generate-parser

check-generated-lexer:
	"$(srcdir)/scripts/generate-lexer.sh" --check

check-generated-parser:
	"$(srcdir)/scripts/generate-parser.sh" --check

check-generated-sources: check-generated-lexer check-generated-parser
