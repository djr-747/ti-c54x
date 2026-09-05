"""Generic C source lexical helpers owned by repository quality checks.

Owns comment/literal normalization, source-positioned tokenization, include
directive extraction, and shallow file-scope statement grouping.  Does not own
any TI C54x policy, path ownership, or diagnostic decision.
"""

from __future__ import annotations

import re
from dataclasses import dataclass
from typing import Iterator, Sequence


Token = tuple[str, int]

_INCLUDE_RE = re.compile(r"^\s*#\s*include\s*([<\"])([^>\"]+)[>\"]")
_PREPROCESSOR_RE = re.compile(r"^\s*#")


@dataclass(frozen=True)
class IncludeDirective:
    """One comment-free include directive with source spelling and line."""

    header: str
    spelling: str
    line: int


@dataclass(frozen=True)
class TopLevelStatement:
    """Tokens ending at a file-scope semicolon, excluding function bodies."""

    tokens: tuple[Token, ...]

    @property
    def line(self) -> int:
        return self.tokens[0][1]


def strip_comments(text: str) -> str:
    """Replace comments with spaces while retaining literals and line positions."""

    output: list[str] = []
    i = 0
    state = "code"
    n = len(text)
    while i < n:
        char = text[i]
        next_char = text[i + 1] if i + 1 < n else ""
        if state == "code":
            if char == "/" and next_char == "/":
                output.extend((" ", " "))
                i += 2
                state = "line_comment"
            elif char == "/" and next_char == "*":
                output.extend((" ", " "))
                i += 2
                state = "block_comment"
            else:
                output.append(char)
                i += 1
        elif state == "line_comment":
            if char == "\n":
                output.append("\n")
                state = "code"
            else:
                output.append(" ")
            i += 1
        else:
            if char == "*" and next_char == "/":
                output.extend((" ", " "))
                i += 2
                state = "code"
            else:
                output.append("\n" if char == "\n" else " ")
                i += 1
    return "".join(output)


def strip_literals_and_comments(text: str) -> str:
    """Replace comments and C string/character literals with spaces."""

    output: list[str] = []
    i = 0
    state = "code"
    n = len(text)
    while i < n:
        char = text[i]
        next_char = text[i + 1] if i + 1 < n else ""
        if state == "code":
            if char == "/" and next_char == "/":
                output.extend((" ", " "))
                i += 2
                state = "line_comment"
            elif char == "/" and next_char == "*":
                output.extend((" ", " "))
                i += 2
                state = "block_comment"
            elif char == '"' or char == "'":
                output.append(" ")
                i += 1
                state = "string" if char == '"' else "char"
            else:
                output.append(char)
                i += 1
        elif state == "line_comment":
            if char == "\n":
                output.append("\n")
                state = "code"
            else:
                output.append(" ")
            i += 1
        elif state == "block_comment":
            if char == "*" and next_char == "/":
                output.extend((" ", " "))
                i += 2
                state = "code"
            else:
                output.append("\n" if char == "\n" else " ")
                i += 1
        else:
            if char == "\\" and i + 1 < n:
                output.extend((" ", "\n" if next_char == "\n" else " "))
                i += 2
            elif (state == "string" and char == '"') or (
                state == "char" and char == "'"
            ):
                output.append(" ")
                i += 1
                state = "code"
            else:
                output.append("\n" if char == "\n" else " ")
                i += 1
    return "".join(output)


def comments_only(text: str) -> str:
    """Keep comments and line positions while replacing code and literals."""

    output: list[str] = []
    i = 0
    state = "code"
    n = len(text)
    while i < n:
        char = text[i]
        next_char = text[i + 1] if i + 1 < n else ""
        if state == "code":
            if char == "/" and next_char == "/":
                output.extend(("/", "/"))
                i += 2
                state = "line_comment"
            elif char == "/" and next_char == "*":
                output.extend(("/", "*"))
                i += 2
                state = "block_comment"
            elif char == '"' or char == "'":
                output.append(" ")
                i += 1
                state = "string" if char == '"' else "char"
            else:
                output.append("\n" if char == "\n" else " ")
                i += 1
        elif state == "line_comment":
            if char == "\n":
                output.append("\n")
                state = "code"
            else:
                output.append(char)
            i += 1
        elif state == "block_comment":
            if char == "*" and next_char == "/":
                output.extend(("*", "/"))
                i += 2
                state = "code"
            else:
                output.append("\n" if char == "\n" else char)
                i += 1
        else:
            if char == "\\" and i + 1 < n:
                output.extend((" ", "\n" if next_char == "\n" else " "))
                i += 2
            elif (state == "string" and char == '"') or (
                state == "char" and char == "'"
            ):
                output.append(" ")
                i += 1
                state = "code"
            else:
                output.append("\n" if char == "\n" else " ")
                i += 1
    return "".join(output)


def drop_preprocessor_lines(text: str) -> str:
    """Blank preprocessor directives, including backslash continuations."""

    result: list[str] = []
    in_directive = False
    for line in text.splitlines():
        stripped = line.lstrip()
        if in_directive:
            result.append("")
            in_directive = line.rstrip().endswith("\\")
        elif stripped.startswith("#"):
            result.append("")
            in_directive = line.rstrip().endswith("\\")
        else:
            result.append(line)
    return "\n".join(result)


def tokens(text: str) -> list[Token]:
    """Return identifier/punctuation tokens with one-based source lines."""

    clean = strip_literals_and_comments(text)
    result: list[Token] = []
    for match in re.finditer(r"[A-Za-z_][A-Za-z0-9_]*|->|[{}();,*.=]", clean):
        result.append((match.group(0), clean.count("\n", 0, match.start()) + 1))
    return result


def line_of(text: str, offset: int) -> int:
    """Convert a character offset into a one-based source line."""

    return text.count("\n", 0, offset) + 1


def iter_includes(text: str) -> Iterator[IncludeDirective]:
    """Yield real include directives, ignoring directives inside comments."""

    for line_number, line in enumerate(strip_comments(text).splitlines(), start=1):
        match = _INCLUDE_RE.match(line)
        if not match:
            continue
        delimiter, header = match.groups()
        closing = ">" if delimiter == "<" else '"'
        yield IncludeDirective(header, f"{delimiter}{header}{closing}", line_number)


def looks_like_function_pointer(token_words: Sequence[str]) -> bool:
    """Return whether a declaration token sequence contains ``(*name)``."""

    return any(
        token == "(" and index + 1 < len(token_words) and token_words[index + 1] == "*"
        for index, token in enumerate(token_words)
    )


def iter_top_level_statements(text: str) -> Iterator[TopLevelStatement]:
    """Group file-scope semicolon statements while skipping function bodies."""

    statement: list[Token] = []
    brace_depth = 0
    compound_kind: str | None = None
    source_tokens = tokens(drop_preprocessor_lines(strip_literals_and_comments(text)))
    for token, line_number in source_tokens:
        if token == "{" and brace_depth >= 0:
            if brace_depth == 0:
                words = [item[0] for item in statement]
                if words and words[0] in {"typedef", "struct", "enum", "union"}:
                    compound_kind = words[0]
                elif "=" in words:
                    compound_kind = "initializer"
                else:
                    compound_kind = "function"
            brace_depth += 1
            if brace_depth > 0 and compound_kind == "function":
                statement.clear()
            continue
        if token == "}":
            brace_depth = max(0, brace_depth - 1)
            if brace_depth == 0 and compound_kind == "function":
                statement.clear()
                compound_kind = None
            elif brace_depth == 0:
                # Retain the close marker so policy code can distinguish a
                # named type definition from an object after the definition.
                statement.append((token, line_number))
            continue
        if brace_depth != 0:
            continue
        if token == ";":
            if statement:
                yield TopLevelStatement(tuple(statement))
            statement.clear()
            continue
        statement.append((token, line_number))
