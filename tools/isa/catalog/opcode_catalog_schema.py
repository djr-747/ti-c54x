"""Parse and validate the standalone ISA opcode catalog.

Module contract
---------------
Owns: the small, deterministic source schema for exact, masked, and
second-word-qualified first-word opcode rows, including lexical validation,
immutable normalized records, and pairwise overlap checks.  The overlap
exceptions are an exact word declared before a covering masked row, which
keeps its identity under the first-match lookup order, and two qualified
rows whose second-word patterns discriminate every shared first word.  It
is tooling code used before C compilation.
Does not own: opcode generation, C macro consumers, operand extraction,
instruction semantics, timing, or architectural evidence.  Parsing returns
new immutable records; no caller-owned mutable object is retained.
"""

from __future__ import annotations

from dataclasses import dataclass
import re
from typing import Final, Iterable


class OpcodeCatalogError(ValueError):
    """Raised when catalog text violates the source contract."""


@dataclass(frozen=True, slots=True)
class OpcodeRow:
    """One normalized first-word pattern from the declarative catalog.

    ``first_word_value`` and ``first_word_mask`` are canonical 16-bit
    integers. ``program_word_count`` carries its unit explicitly.
    ``operand_layout`` and ``repeatability`` retain the fully namespaced
    catalog tokens. ``input_resources`` is the namespaced resource-mask token.
    ``form`` is the source spelling (``EXACT``, ``MASKED``, or ``MASKED2``),
    and ``source_line`` is the one-based source line where the macro
    declaration begins. ``second_word_value`` and ``second_word_mask`` carry
    the word-2 qualifier for ``MASKED2`` rows and stay ``None`` otherwise.
    """

    symbol: str
    handler: str
    first_word_value: int
    first_word_mask: int
    program_word_count: int
    operand_layout: str
    repeatability: str
    form: str
    source_line: int
    input_resources: str
    second_word_value: int | None = None
    second_word_mask: int | None = None


_ID_RE: Final = re.compile(r"^[A-Z][A-Z0-9_]*$")
_HANDLER_RE: Final = re.compile(r"^[a-z][a-z0-9_]*$")
_HEX_RE: Final = re.compile(r"^0[xX][0-9A-Fa-f]+$")
_WIDTH_RE: Final = re.compile(r"^[0-9]+$")
_OPERAND_LAYOUT_TOKENS: Final = frozenset(
    {
        "TI_C54X_OPERAND_LAYOUT_NONE",
        "TI_C54X_OPERAND_LAYOUT_RPT_IMMEDIATE_K",
        "TI_C54X_OPERAND_LAYOUT_RPT_IMMEDIATE_LK",
        "TI_C54X_OPERAND_LAYOUT_STATUS_BIT",
        "TI_C54X_OPERAND_LAYOUT_CONDITION_ARF",
        "TI_C54X_OPERAND_LAYOUT_LD_IMMEDIATE_DP",
        "TI_C54X_OPERAND_LAYOUT_LD_IMMEDIATE_ASM",
        "TI_C54X_OPERAND_LAYOUT_LD_IMMEDIATE_ARP",
        "TI_C54X_OPERAND_LAYOUT_ARF",
        "TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16",
        "TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF",
        "TI_C54X_OPERAND_LAYOUT_ARF_MOD",
        "TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16_MOD",
        "TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD",
        "TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4",
        "TI_C54X_OPERAND_LAYOUT_ARF_MOD4",
        "TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16_MOD4",
        "TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_EXTENSION16_MOD4",
        "TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD_SHIFT5",
        "TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4_SHIFT5",
        "TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_DIRECT7_SHIFT5",
        "TI_C54X_OPERAND_LAYOUT_MAX_MIN_ACC",
        "TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE",
        "TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION",
        "TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT5",
        "TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_IMMEDIATE_K8",
        "TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_IMMEDIATE_LK16",
        "TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_IMMEDIATE_SHIFT4_LK16",
        "TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT4_EXTENSION16",
        "TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_EXTENSION16_SHIFT16",
        "TI_C54X_OPERAND_LAYOUT_DUAL_DESTINATION_XAR_YAR",
        "TI_C54X_OPERAND_LAYOUT_DUAL_ROUND_SOURCE_DESTINATION_XAR_YAR",
        "TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_DUAL_XMOD_XAR_SHIFT4",
        "TI_C54X_OPERAND_LAYOUT_BRANCH_PMAD16",
        "TI_C54X_OPERAND_LAYOUT_BRANCH_COND_PMAD16",
        "TI_C54X_OPERAND_LAYOUT_BRANCH_COND8",
        "TI_C54X_OPERAND_LAYOUT_BRANCH_SIND_PMAD16",
        "TI_C54X_OPERAND_LAYOUT_DIRECT7_EXTENSION16",
        "TI_C54X_OPERAND_LAYOUT_ROUND_SELECT_INDIRECT_ADDR7",
        "TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7",
        "TI_C54X_OPERAND_LAYOUT_N_CONDITION8",
        "TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7",
        "TI_C54X_OPERAND_LAYOUT_SELECT_DUAL_XMOD_XAR_COND4",
        "TI_C54X_OPERAND_LAYOUT_SOURCE_DESTINATION_INDIRECT_ADDR7",
        "TI_C54X_OPERAND_LAYOUT_DESTINATION_INDIRECT_ADDR7",
        "TI_C54X_OPERAND_LAYOUT_ROUND_INDIRECT_ADDR7",
        "TI_C54X_OPERAND_LAYOUT_SOURCE_DESTINATION_ROUND",
        "TI_C54X_OPERAND_LAYOUT_DESTINATION_INDIRECT_ADDR7_PMAD16",
        "TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7_PA16",
        "TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7_SOURCE_DESTINATION_SHIFT5",
        "TI_C54X_OPERAND_LAYOUT_DESTINATION_INDIRECT_ADDR7_EXTENSION16",
        "TI_C54X_OPERAND_LAYOUT_SOURCE_DESTINATION_INDIRECT_ADDR7_EXTENSION16",
        "TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7_EXTENSION16",
    }
)
_REPEATABILITY_TOKENS: Final = frozenset(
    {
        "TI_C54X_REPEATABILITY_UNRESOLVED",
        "TI_C54X_REPEATABILITY_ELIGIBLE",
        "TI_C54X_REPEATABILITY_INELIGIBLE",
    }
)
_INPUT_RESOURCE_TOKENS: Final = frozenset(
    {
        "TI_C54X_INPUT_RESOURCES_NONE",
        "TI_C54X_INPUT_RESOURCES_ACCUMULATORS",
        "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STACK_POINTER",
        "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_ASM",
        "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_AUXILIARY_REGISTERS",
        "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_AUXILIARY_REGISTERS_STATUS_SXM_ASM",
        "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_AUXILIARY_REGISTERS_BRC_REGISTER",
        "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_AUXILIARY_REGISTERS_T_REGISTER",
        "TI_C54X_INPUT_RESOURCES_STATUS_DP",
        "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_DP",
        "TI_C54X_INPUT_RESOURCES_STATUS_C",
        "TI_C54X_INPUT_RESOURCES_STATUS_TC",
        "TI_C54X_INPUT_RESOURCES_STATUS_SXM",
        "TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS",
        "TI_C54X_INPUT_RESOURCES_T_REGISTER",
        "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_C",
        "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_TC",
        "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_C_TC",
        "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_C_OVM",
        "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_OVM",
        "TI_C54X_INPUT_RESOURCES_STATUS_SXM_T_REGISTER",
        "TI_C54X_INPUT_RESOURCES_STATUS_SXM_OVM",
        "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM",
        "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_SST",
        "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_OVM",
        "TI_C54X_INPUT_RESOURCES_STATUS_ASM",
        "TI_C54X_INPUT_RESOURCES_STACK_POINTER",
        "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_OVM_ASM",
        "TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS_STATUS_C_SXM_OVM",
        "TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS_STATUS_SXM_OVM",
        "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_C_TC_STACK_POINTER",
        "TI_C54X_INPUT_RESOURCES_STACK_POINTER_RETURN_REGISTER",
        "TI_C54X_INPUT_RESOURCES_STATUS_FRCT",
        "TI_C54X_INPUT_RESOURCES_STATUS_FRCT_OVM",
        "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_T_REGISTER_STATUS_FRCT_OVM",
        "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_T_REGISTER_STATUS_SXM_OVM",
        "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_FRCT_OVM",
        "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_AUXILIARY_REGISTERS_STATUS_FRCT_OVM",
        "TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS_STATUS_FRCT_OVM",
        "TI_C54X_INPUT_RESOURCES_TRN_REGISTER",
        "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_ASM",
    }
)
_ROW_RE: Final = re.compile(
    r"^TI_C54X_OPCODE_(EXACT|MASKED2|MASKED)\((.*)\)$"
)

_WORD_BITS = 16
_WORD_MAX = (1 << _WORD_BITS) - 1


def _strip_comments(text: str) -> str:
    """Remove C comments while preserving newlines for source line reporting."""

    result: list[str] = []
    index = 0
    in_block = False
    while index < len(text):
        if in_block:
            end = text.find("*/", index)
            if end < 0:
                raise OpcodeCatalogError("unterminated block comment")
            result.extend("\n" if char == "\n" else " " for char in text[index:end])
            result.extend("  ")
            index = end + 2
            in_block = False
            continue
        if text.startswith("/*", index):
            in_block = True
            index += 2
            continue
        if text.startswith("//", index):
            newline = text.find("\n", index)
            if newline < 0:
                result.extend(" " for _ in text[index:])
                break
            result.extend(" " for _ in text[index:newline])
            result.append("\n")
            index = newline + 1
            continue
        result.append(text[index])
        index += 1
    if in_block:  # Defensive: normally raised by the branch above.
        raise OpcodeCatalogError("unterminated block comment")
    return "".join(result)


def _parse_word(token: str, location: str) -> int:
    if not _HEX_RE.fullmatch(token):
        raise OpcodeCatalogError(f"{location}: word must be hexadecimal")
    value = int(token, 16)
    if value > _WORD_MAX:
        raise OpcodeCatalogError(f"{location}: word is outside 16-bit range")
    return value


def _parse_width(token: str, location: str) -> int:
    if not _WIDTH_RE.fullmatch(token):
        raise OpcodeCatalogError(f"{location}: width must be decimal")
    value = int(token, 10)
    if not 1 <= value <= 0xFF:
        raise OpcodeCatalogError(f"{location}: width must be from 1 through 255")
    return value


def _row_key(row: OpcodeRow) -> tuple:
    """Stable semantic ordering, independent of declaration order."""

    return (
        row.symbol,
        row.handler,
        row.first_word_value,
        row.first_word_mask,
        row.program_word_count,
        row.operand_layout,
        row.repeatability,
        row.input_resources,
        row.form,
        row.source_line,
        row.second_word_value if row.second_word_value is not None else -1,
        row.second_word_mask if row.second_word_mask is not None else -1,
    )


def _overlap_witness(first: OpcodeRow, second: OpcodeRow) -> int | None:
    """Return the lowest 16-bit word matching both patterns, if one exists."""

    shared_mask = first.first_word_mask & second.first_word_mask
    if ((first.first_word_value ^ second.first_word_value) & shared_mask) != 0:
        return None
    return first.first_word_value | second.first_word_value


def _is_single_word_shadow(first: OpcodeRow, second: OpcodeRow) -> bool:
    """Whether an exact word keeps its identity inside a later masked row.

    The generated lookup resolves words in catalog declaration order, so
    an exact row declared before a covering masked row deterministically
    keeps its single word (for example the unconditional word shared by
    an exact return row and a masked conditional-return row).  The
    reversed order stays an overlap error: first-match resolution would
    hand the word to the masked row instead.
    """

    if first.form == "EXACT" and second.form == "MASKED":
        exact, masked = first, second
    elif first.form == "MASKED" and second.form == "EXACT":
        exact, masked = second, first
    else:
        return False
    return exact.source_line < masked.source_line


def _word2_disjoint(first: OpcodeRow, second: OpcodeRow) -> bool:
    """Whether two word-2-qualified rows discriminate every shared word.

    Both rows must carry qualifiers and their patterns must disagree on at
    least one commonly masked bit; any other pairing keeps the overlap.
    """

    if (
        first.second_word_value is None
        or first.second_word_mask is None
        or second.second_word_value is None
        or second.second_word_mask is None
    ):
        return False
    shared = first.second_word_mask & second.second_word_mask
    return ((first.second_word_value ^ second.second_word_value) & shared) != 0


def _validate_overlaps(rows: Iterable[OpcodeRow]) -> None:
    ordered = sorted(rows, key=_row_key)
    for index, first in enumerate(ordered):
        for second in ordered[index + 1 :]:
            witness = _overlap_witness(first, second)
            if witness is None:
                continue
            if _is_single_word_shadow(first, second):
                continue
            if _word2_disjoint(first, second):
                continue
            names = sorted((first.symbol, second.symbol))
            raise OpcodeCatalogError(
                "overlapping opcode patterns "
                f"{names[0]} and {names[1]} at 0x{witness:04X} "
                f"(lines {min(first.source_line, second.source_line)} and "
                f"{max(first.source_line, second.source_line)})"
            )


def _is_integer(value: object) -> bool:
    """Return whether ``value`` is an integer field, excluding booleans."""

    return isinstance(value, int) and not isinstance(value, bool)


def validate_catalog_rows(rows: Iterable[OpcodeRow]) -> tuple[OpcodeRow, ...]:
    """Validate directly supplied rows and return an immutable normalized tuple.

    ``parse_catalog`` and every generator entry point share this contract.  It
    deliberately accepts an empty iterable so a derivative may render an
    all-UNKNOWN lookup; the source parser separately rejects an empty catalog.
    No caller-owned container is retained.
    """

    try:
        normalized = tuple(rows)
    except TypeError as error:
        raise OpcodeCatalogError("opcode rows must be iterable") from error

    if len(normalized) > _WORD_MAX:
        raise OpcodeCatalogError(
            f"opcode identity count {len(normalized)} exceeds UINT16_MAX ({_WORD_MAX})"
        )

    symbols: set[str] = set()
    handlers: set[str] = set()
    for position, row in enumerate(normalized, 1):
        location = f"row {position}"
        if not isinstance(row, OpcodeRow):
            raise OpcodeCatalogError(f"{location}: expected OpcodeRow")
        if not isinstance(row.symbol, str) or _ID_RE.fullmatch(row.symbol) is None:
            raise OpcodeCatalogError(f"{location}: invalid or reserved opcode ID")
        if row.symbol in {"UNKNOWN", "COUNT"}:
            raise OpcodeCatalogError(f"{location}: invalid or reserved opcode ID")
        if not isinstance(row.handler, str) or _HANDLER_RE.fullmatch(row.handler) is None:
            raise OpcodeCatalogError(f"{location}: invalid handler token")
        if row.symbol in symbols:
            raise OpcodeCatalogError(f"{location}: duplicate opcode ID {row.symbol}")
        if row.handler in handlers:
            raise OpcodeCatalogError(f"{location}: duplicate handler token {row.handler}")
        if not _is_integer(row.first_word_value) or not 0 <= row.first_word_value <= _WORD_MAX:
            raise OpcodeCatalogError(f"{location}: value is outside 16-bit range")
        if not _is_integer(row.first_word_mask) or not 0 <= row.first_word_mask <= _WORD_MAX:
            raise OpcodeCatalogError(f"{location}: mask is outside 16-bit range")
        if not _is_integer(row.program_word_count) or not 1 <= row.program_word_count <= 0xFF:
            raise OpcodeCatalogError(f"{location}: width must be from 1 through 255")
        if (
            not isinstance(row.operand_layout, str)
            or row.operand_layout not in _OPERAND_LAYOUT_TOKENS
        ):
            raise OpcodeCatalogError(
                f"{location}: invalid or unresolved operand-layout token"
            )
        if (
            not isinstance(row.repeatability, str)
            or row.repeatability not in _REPEATABILITY_TOKENS
        ):
            raise OpcodeCatalogError(f"{location}: invalid repeatability token")
        if (
            not isinstance(row.input_resources, str)
            or row.input_resources not in _INPUT_RESOURCE_TOKENS
        ):
            raise OpcodeCatalogError(f"{location}: invalid input-resource token")
        if not isinstance(row.form, str) or row.form not in {"EXACT", "MASKED", "MASKED2"}:
            raise OpcodeCatalogError(f"{location}: unknown opcode form {row.form!r}")
        if row.form == "MASKED2":
            if row.program_word_count < 2:
                raise OpcodeCatalogError(
                    f"{location}: MASKED2 form needs at least two program words"
                )
            if not _is_integer(row.second_word_value) or not 0 <= row.second_word_value <= _WORD_MAX:
                raise OpcodeCatalogError(f"{location}: second word is outside 16-bit range")
            if not _is_integer(row.second_word_mask) or not 0 < row.second_word_mask <= _WORD_MAX:
                raise OpcodeCatalogError(f"{location}: second mask must be nonzero 16-bit")
            if row.second_word_value & ~row.second_word_mask:
                raise OpcodeCatalogError(f"{location}: second value sets bits outside its mask")
        elif row.second_word_value is not None or row.second_word_mask is not None:
            raise OpcodeCatalogError(f"{location}: second-word qualifier needs MASKED2 form")
        if not _is_integer(row.source_line) or row.source_line <= 0:
            raise OpcodeCatalogError(f"{location}: source line must be positive")
        if row.form == "EXACT" and row.first_word_mask != _WORD_MAX:
            raise OpcodeCatalogError(f"{location}: EXACT form requires a full 16-bit mask")
        if row.form == "MASKED" and row.first_word_mask in {0, _WORD_MAX}:
            raise OpcodeCatalogError(
                f"{location}: MASKED form requires a partial nonzero mask"
            )
        if row.first_word_value & ~row.first_word_mask:
            raise OpcodeCatalogError(f"{location}: value sets bits outside its mask")
        symbols.add(row.symbol)
        handlers.add(row.handler)

    _validate_overlaps(normalized)
    return normalized


def parse_catalog(text: str) -> list[OpcodeRow]:
    """Parse catalog macro rows into immutable normalized records.

    The accepted forms are ``TI_C54X_OPCODE_EXACT(symbol, handler, value,
    width, operand_layout, repeatability, input_resources)``,
    ``TI_C54X_OPCODE_MASKED(symbol, handler, value, mask, width,
    operand_layout, repeatability, input_resources)``, and
    ``TI_C54X_OPCODE_MASKED2(symbol, handler, value, mask, width,
    operand_layout, repeatability, input_resources, second_value,
    second_mask)``. Every non-empty non-comment line must contain exactly
    one row; comments and blank lines are otherwise ignored.
    """

    if not isinstance(text, str):
        raise OpcodeCatalogError("catalog text must be a string")
    rows: list[OpcodeRow] = []
    for line_number, raw_line in enumerate(_strip_comments(text).splitlines(), 1):
        line = raw_line.strip()
        if not line:
            continue
        match = _ROW_RE.fullmatch(line)
        if match is None:
            raise OpcodeCatalogError(f"malformed catalog row at line {line_number}")
        form, body = match.groups()
        fields = tuple(field.strip() for field in body.split(","))
        expected_fields = {"EXACT": 7, "MASKED": 8, "MASKED2": 10}[form]
        if len(fields) != expected_fields or any(not field for field in fields):
            raise OpcodeCatalogError(
                f"line {line_number}: {form} requires exactly "
                f"{expected_fields} fields"
            )
        symbol, handler, value_token = fields[:3]
        if form == "EXACT":
            mask_or_width, operand_layout, repeatability, input_resources = fields[3:]
        elif form == "MASKED":
            mask_or_width, width_token, operand_layout, repeatability, input_resources = fields[3:]
        else:
            (mask_or_width, width_token, operand_layout, repeatability,
             input_resources, second_value_token, second_mask_token) = fields[3:]
        location = f"line {line_number}"
        if form == "EXACT":
            value = _parse_word(value_token, f"{location}: value")
            mask = _WORD_MAX
            width = _parse_width(mask_or_width, f"{location}: width")
            second_value = None
            second_mask = None
        else:
            value = _parse_word(value_token, f"{location}: value")
            mask = _parse_word(mask_or_width, f"{location}: mask")
            width = _parse_width(width_token, f"{location}: width")
            if form == "MASKED2":
                second_value = _parse_word(second_value_token, f"{location}: second value")
                second_mask = _parse_word(second_mask_token, f"{location}: second mask")
            else:
                second_value = None
                second_mask = None

        rows.append(
            OpcodeRow(
                symbol,
                handler,
                value,
                mask,
                width,
                operand_layout,
                repeatability,
                form,
                line_number,
                input_resources,
                second_value,
                second_mask,
            )
        )

    normalized = validate_catalog_rows(rows)
    if not normalized:
        raise OpcodeCatalogError("catalog has no opcode rows")
    return list(normalized)


__all__ = [
    "OpcodeCatalogError",
    "OpcodeRow",
    "parse_catalog",
    "validate_catalog_rows",
]
