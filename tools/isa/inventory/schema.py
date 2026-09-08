"""Schema validation for the TI C54x instruction-form audit inventory.

Module contract
---------------
Owns: JSON shape, lexical fields, summary checkpoints, stable form identities,
and internal form/catalog cross-reference validation, including multiple
addressing-mode bindings for one stable form identity.  Does not own: opcode
values or masks, production catalog membership, instruction semantics, or
coverage claims beyond the explicit inventory records.  Parsing retains no
caller-owned mutable state and performs no environment or network reads.
"""

from __future__ import annotations

import json
from pathlib import Path
import re
from typing import Any


class InventoryError(ValueError):
    """Raised when an inventory cannot be loaded or violates its schema."""


ROOT_KEYS = {"schema", "target_id", "source_id", "basis", "forms", "catalog_bindings"}
SCHEMA_KEYS = {"name", "version"}
BASIS_KEYS = {
    "chapter", "tables", "printed_pages", "table_form_counts", "form_count",
    "mnemonic_count", "parallel_bundle_count",
}
FORM_KEYS = {
    "id", "ordinal", "table", "table_ordinal", "mnemonic", "parallel",
    "scope", "coverage", "catalog_symbols",
}
BINDING_KEYS = {"catalog_symbol", "form_id", "coverage", "addressing_subset"}
SCOPES = frozenset({"COMMON_CANDIDATE", "PROFILE_ONLY", "OUT_OF_SCOPE", "UNRESOLVED"})
COVERAGE = frozenset({"COMPLETE", "SUBSET", "UNIMPLEMENTED"})
FORM_COUNT = 205
MNEMONIC_COUNT = 126
PARALLEL_BUNDLE_COUNT = 13
TABLE_COUNT = 26
_ID_RE = re.compile(r"^FORM-T2-(?P<table>[0-9]{2})-(?P<ordinal>[0-9]{2})$")
_SYMBOL_RE = re.compile(r"^[A-Z][A-Z0-9_]*$")
_ADDRESSING_TOKEN_RE = re.compile(r"^[A-Z][A-Z0-9_]*$")
_FORBIDDEN_KEYS = frozenset({
    "opcode", "opcode_value", "opcode_mask", "first_word_value",
    "first_word_mask", "mask", "value",
})


def _error(errors: list[str], location: str, message: str) -> None:
    errors.append(f"{location}: {message}")


def _is_int(value: Any) -> bool:
    return isinstance(value, int) and not isinstance(value, bool)


def _enum_member(value: Any, allowed: frozenset[str]) -> bool:
    """Return whether *value* is a hashable string in an enum set."""

    return isinstance(value, str) and value in allowed


def _string(value: Any, location: str, errors: list[str]) -> bool:
    if not isinstance(value, str) or not value.strip():
        _error(errors, location, "must be a non-empty string")
        return False
    return True


def _exact_keys(value: Any, expected: set[str], location: str, errors: list[str]) -> bool:
    if not isinstance(value, dict):
        _error(errors, location, "must be an object")
        return False
    actual = set(value)
    if actual != expected:
        _error(errors, location, f"keys must be exactly {sorted(expected)}")
        return False
    return True


def _scan_forbidden(value: Any, location: str, errors: list[str]) -> None:
    if isinstance(value, dict):
        for key, child in value.items():
            if key in _FORBIDDEN_KEYS:
                _error(errors, f"{location}.{key}", "opcode authority fields are forbidden")
            _scan_forbidden(child, f"{location}.{key}", errors)
    elif isinstance(value, list):
        for index, child in enumerate(value):
            _scan_forbidden(child, f"{location}[{index}]", errors)


def validate_inventory(value: Any) -> list[str]:
    """Return deterministic schema/cross-reference errors for *value*.

    This is intentionally repository-independent.  The quality checker is the
    only layer that compares inventory symbols with ``opcode_catalog.def``.
    """

    errors: list[str] = []
    if not _exact_keys(value, ROOT_KEYS, "root", errors):
        return sorted(errors)
    _scan_forbidden(value, "root", errors)

    schema = value["schema"]
    if _exact_keys(schema, SCHEMA_KEYS, "schema", errors):
        if schema.get("name") != "ti-c54x-isa-inventory":
            _error(errors, "schema.name", "must equal ti-c54x-isa-inventory")
        if schema.get("version") != 1 or not _is_int(schema.get("version")):
            _error(errors, "schema.version", "must equal integer 1")
    if value.get("target_id") != "C54X_CORE_COMMON_V1":
        _error(errors, "target_id", "must equal C54X_CORE_COMMON_V1")
    if value.get("source_id") != "SRC-TI-SPRU172C":
        _error(errors, "source_id", "must equal SRC-TI-SPRU172C")

    basis = value["basis"]
    if _exact_keys(basis, BASIS_KEYS, "basis", errors):
        expected = {
            "chapter": "2.1-2.4",
            "tables": "2-1 through 2-26",
            "printed_pages": "2-2 through 2-18",
            "form_count": FORM_COUNT,
            "mnemonic_count": MNEMONIC_COUNT,
            "parallel_bundle_count": PARALLEL_BUNDLE_COUNT,
        }
        for key, expected_value in expected.items():
            if basis.get(key) != expected_value:
                _error(errors, f"basis.{key}", f"must equal {expected_value!r}")
        vector = basis.get("table_form_counts")
        if not isinstance(vector, list):
            _error(errors, "basis.table_form_counts", "must be a list")
        else:
            if len(vector) != TABLE_COUNT:
                _error(errors, "basis.table_form_counts", f"must contain exactly {TABLE_COUNT} table counts")
            for index, count in enumerate(vector, start=1):
                if not _is_int(count) or count <= 0:
                    _error(errors, f"basis.table_form_counts[{index - 1}]", "must be a positive integer")

    forms = value["forms"]
    form_ids: set[str] = set()
    symbols_by_form: dict[str, set[str]] = {}
    mnemonics: set[str] = set()
    parallel_count = 0
    table_ordinals: dict[str, set[int]] = {}
    table_counts: dict[str, int] = {}
    coverage_by_form: dict[str, str] = {}
    if not isinstance(forms, list):
        _error(errors, "forms", "must be a list")
        forms = []
    for index, form in enumerate(forms):
        location = f"forms[{index}]"
        if not _exact_keys(form, FORM_KEYS, location, errors):
            continue
        identifier = form["id"]
        if not _string(identifier, f"{location}.id", errors):
            continue
        match = _ID_RE.fullmatch(identifier)
        if match is None:
            _error(errors, f"{location}.id", "must match FORM-T2-XX-YY")
        elif identifier in form_ids:
            _error(errors, f"{location}.id", f"duplicate form ID {identifier}")
        form_ids.add(identifier)
        ordinal = form["ordinal"]
        if not _is_int(ordinal) or ordinal != index + 1:
            _error(errors, f"{location}.ordinal", "must be the one-based global form ordinal")
        table = form["table"]
        table_match = re.fullmatch(r"2-([0-9]+)", table) if isinstance(table, str) else None
        if table_match is None or not 1 <= int(table_match.group(1)) <= 26:
            _error(errors, f"{location}.table", "must be a table identifier from 2-1 through 2-26")
        table_ordinal = form["table_ordinal"]
        if not _is_int(table_ordinal) or table_ordinal <= 0:
            _error(errors, f"{location}.table_ordinal", "must be a positive integer")
        elif isinstance(table, str):
            seen_ordinals = table_ordinals.setdefault(table, set())
            if table_ordinal in seen_ordinals:
                _error(errors, f"{location}.table_ordinal", "duplicate ordinal within table")
            seen_ordinals.add(table_ordinal)
            table_counts[table] = len(seen_ordinals)
        if match is not None and table_match is not None:
            if int(match.group("table")) != int(table_match.group(1)) or int(match.group("ordinal")) != table_ordinal:
                _error(errors, f"{location}.id", "does not match table and table ordinal")
        if _string(form["mnemonic"], f"{location}.mnemonic", errors):
            mnemonics.add(form["mnemonic"])
        parallel = form["parallel"]
        if not isinstance(parallel, bool):
            _error(errors, f"{location}.parallel", "must be a boolean")
        elif parallel:
            parallel_count += 1
        if not _enum_member(form["scope"], SCOPES):
            _error(errors, f"{location}.scope", "is not a supported scope")
        coverage = form["coverage"]
        if not _enum_member(coverage, COVERAGE):
            _error(errors, f"{location}.coverage", "is not a supported coverage state")
        symbols = form["catalog_symbols"]
        if not isinstance(symbols, list):
            _error(errors, f"{location}.catalog_symbols", "must be a list")
            symbols = []
        symbol_set: set[str] = set()
        for symbol_index, symbol in enumerate(symbols):
            symbol_location = f"{location}.catalog_symbols[{symbol_index}]"
            if not isinstance(symbol, str) or _SYMBOL_RE.fullmatch(symbol) is None:
                _error(errors, symbol_location, "must be an uppercase catalog symbol")
            elif symbol in symbol_set:
                _error(errors, symbol_location, f"duplicate symbol {symbol}")
            symbol_set.add(symbol)
        if coverage == "UNIMPLEMENTED" and symbol_set:
            _error(errors, location, "UNIMPLEMENTED forms must not list catalog symbols")
        if _enum_member(coverage, frozenset({"COMPLETE", "SUBSET"})) and not symbol_set:
            _error(errors, location, f"{coverage} forms must list catalog symbols")
        symbols_by_form[identifier] = symbol_set
        if _enum_member(coverage, COVERAGE):
            coverage_by_form[identifier] = coverage

    if len(forms) != FORM_COUNT:
        _error(errors, "forms", f"must contain exactly {FORM_COUNT} records")
    if len(mnemonics) != MNEMONIC_COUNT:
        _error(errors, "forms.mnemonic", f"must contain exactly {MNEMONIC_COUNT} normalized mnemonics")
    if parallel_count != PARALLEL_BUNDLE_COUNT:
        _error(errors, "forms.parallel", f"must contain exactly {PARALLEL_BUNDLE_COUNT} parallel bundles")

    # The basis vector is a checkpoint for the forms themselves, not a second
    # hand-maintained source of table counts.  Derive the counts and ordinals
    # here so a moved form, skipped ordinal, or duplicated ordinal cannot pass
    # by updating only one side of the checkpoint.
    vector = basis.get("table_form_counts") if isinstance(basis, dict) else None
    derived_counts = [table_counts.get(f"2-{table_number}", 0) for table_number in range(1, TABLE_COUNT + 1)]
    if isinstance(vector, list) and vector != derived_counts:
        _error(errors, "basis.table_form_counts", "does not match counts derived from forms")
    for table_number in range(1, TABLE_COUNT + 1):
        table = f"2-{table_number}"
        ordinals = sorted(table_ordinals.get(table, set()))
        expected = list(range(1, len(ordinals) + 1))
        if ordinals != expected:
            _error(errors, f"forms.table_ordinal.{table}", "must be contiguous starting at 1")

    bindings = value["catalog_bindings"]
    if not isinstance(bindings, list):
        _error(errors, "catalog_bindings", "must be a list")
        bindings = []
    seen_symbols: set[str] = set()
    seen_pairs: set[tuple[str, str]] = set()
    binding_symbols_by_form: dict[str, set[str]] = {}
    bindings_by_form: dict[str, list[dict[str, Any]]] = {}
    for index, binding in enumerate(bindings):
        location = f"catalog_bindings[{index}]"
        if not _exact_keys(binding, BINDING_KEYS, location, errors):
            continue
        symbol = binding["catalog_symbol"]
        form_id = binding["form_id"]
        coverage = binding["coverage"]
        if not isinstance(symbol, str) or _SYMBOL_RE.fullmatch(symbol) is None:
            _error(errors, f"{location}.catalog_symbol", "must be an uppercase symbol")
        elif symbol in seen_symbols:
            _error(errors, location, f"duplicate catalog symbol {symbol}")
        seen_symbols.add(symbol)
        if form_id not in form_ids:
            _error(errors, f"{location}.form_id", f"unknown form ID {form_id!r}")
        pair = (symbol, form_id)
        if pair in seen_pairs:
            _error(errors, location, "duplicate catalog/form binding")
        seen_pairs.add(pair)
        if not isinstance(coverage, str) or coverage not in {"COMPLETE", "SUBSET"}:
            _error(errors, f"{location}.coverage", "must be COMPLETE or SUBSET")
        addressing_subset = binding["addressing_subset"]
        if not isinstance(addressing_subset, str) or _ADDRESSING_TOKEN_RE.fullmatch(addressing_subset) is None:
            _error(errors, f"{location}.addressing_subset", "must be a non-empty uppercase token")
        elif coverage == "COMPLETE" and addressing_subset != "ALL":
            _error(errors, location, "COMPLETE bindings must use ALL addressing_subset")
        elif coverage == "SUBSET" and addressing_subset == "ALL":
            _error(errors, location, "SUBSET bindings must use a non-ALL addressing_subset")
        binding_symbols_by_form.setdefault(form_id, set()).add(symbol)
        bindings_by_form.setdefault(form_id, []).append(binding)

    # A COMPLETE form remains one production catalog row. A SUBSET form may
    # expose multiple disjoint addressing-mode rows (for example neutral
    # MOD0--3 plus indexed MOD5/MOD6), but every binding must retain the same
    # audit coverage state and the form's symbol list must match exactly.
    for form_id, symbols in symbols_by_form.items():
        coverage = coverage_by_form.get(form_id)
        records = bindings_by_form.get(form_id, [])
        if coverage == "UNIMPLEMENTED":
            if records:
                _error(errors, f"forms[{form_id}].coverage", "UNIMPLEMENTED forms must not have catalog bindings")
        elif coverage in {"COMPLETE", "SUBSET"}:
            if not records or (coverage == "COMPLETE" and len(records) != 1):
                expected = "exactly one" if coverage == "COMPLETE" else "at least one"
                _error(errors, f"forms[{form_id}].coverage", f"bound forms must have {expected} catalog binding")
            for binding in records:
                if binding.get("coverage") != coverage:
                    _error(errors, f"forms[{form_id}].coverage", "does not match its catalog binding coverage")
                addressing_subset = binding.get("addressing_subset")
                if coverage == "COMPLETE" and addressing_subset != "ALL":
                    _error(errors, f"forms[{form_id}].coverage", "must use ALL addressing_subset")
                elif coverage == "SUBSET" and addressing_subset == "ALL":
                    _error(errors, f"forms[{form_id}].coverage", "must use a non-ALL addressing_subset")

    for form_id, symbols in symbols_by_form.items():
        if symbols != binding_symbols_by_form.get(form_id, set()):
            _error(errors, f"forms[{form_id}].catalog_symbols", "does not match catalog binding records")
    for form_id, symbols in binding_symbols_by_form.items():
        if form_id not in symbols_by_form:
            continue
        for symbol in symbols:
            if symbol not in symbols_by_form[form_id]:
                _error(errors, f"catalog_bindings[{form_id}]", f"symbol {symbol} is absent from form catalog_symbols")
    return sorted(set(errors))


def load_inventory(path: Path | str) -> dict[str, Any]:
    """Load and validate a JSON inventory from *path*."""

    resolved = Path(path)
    try:
        value = json.loads(resolved.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as error:
        raise InventoryError(f"cannot parse inventory {resolved}: {error}") from error
    errors = validate_inventory(value)
    if errors:
        raise InventoryError("\n".join(errors))
    return value


__all__ = [
    "BASIS_KEYS", "BINDING_KEYS", "COVERAGE", "FORM_KEYS", "InventoryError",
    "ROOT_KEYS", "SCHEMA_KEYS", "SCOPES", "load_inventory", "validate_inventory",
]
