#!/usr/bin/env python3
"""Owns the machine-readable knob registry schema and semantic validation.

This module owns JSON shape, lifecycle enums, tombstone identity rules, and
observer-bit allocation invariants.  Repository source/build/doc surfaces are
owned by ``check_knobs.py`` and are deliberately kept separate.
"""

from __future__ import annotations

import json
import re
from pathlib import Path
from typing import Any


SCHEMA_NAME = "ti-c54x-knob-registry"
SCHEMA_VERSION = 1
ALLOWED_STATES = {"ACTIVE", "PLANNED"}
ALLOWED_CLASSES = {"TEST", "OBSERVE", "A/B"}
ALLOWED_DISPOSITIONS = {"KEEP", "TEMP", "RETIRE"}
ALLOWED_SURFACES = {"compile", "runtime"}
ID_RE = re.compile(r"^[A-Za-z][A-Za-z0-9_.-]*$")
C_IDENTIFIER_RE = re.compile(r"^TI_C54X_TRACE_[A-Z][A-Z0-9_]*$")
MASK_IDENTIFIER_RE = re.compile(r"^TI_C54X_TRACE_MASK_[A-Z][A-Z0-9_]*$")


class RegistryError(ValueError):
    """Raised when a registry is unreadable or violates its schema."""


def _require(condition: bool, message: str, errors: list[str]) -> None:
    if not condition:
        errors.append(message)


def _required_string(entry: dict[str, Any], field: str, path: str, errors: list[str]) -> None:
    value = entry.get(field)
    _require(isinstance(value, str) and bool(value.strip()),
             f"{path}: {field} must be a non-empty string", errors)


def _parse_mask(value: Any, path: str, errors: list[str]) -> int | None:
    if not isinstance(value, str) or not re.fullmatch(r"0x[0-9a-fA-F]{1,16}", value):
        errors.append(f"{path}: mask must be a hexadecimal 64-bit string")
        return None
    parsed = int(value, 16)
    if parsed == 0 or parsed > ((1 << 64) - 1) or parsed & (parsed - 1):
        errors.append(f"{path}: mask must contain exactly one non-zero bit")
        return None
    return parsed


def _validate_bit(entry: dict[str, Any], path: str, mask: int | None,
                  errors: list[str]) -> None:
    bit = entry.get("bit")
    _require(isinstance(bit, int) and not isinstance(bit, bool) and 0 <= bit < 64,
             f"{path}.bit must be an integer from 0 through 63", errors)
    if isinstance(bit, int) and not isinstance(bit, bool) and mask is not None:
        _require(mask == (1 << bit),
                 f"{path}.bit does not match its one-hot mask", errors)


def _validate_tombstone(entry: Any, path: str, errors: list[str]) -> str | None:
    """Validate common tombstone metadata and return its identifier."""

    _require(isinstance(entry, dict), f"{path} must be an object", errors)
    if not isinstance(entry, dict):
        return None
    identifier = entry.get("id")
    _require(isinstance(identifier, str) and bool(ID_RE.fullmatch(identifier or "")),
             f"{path}.id must be a unique identifier", errors)
    retired_abi = entry.get("retired_abi_major")
    _require(isinstance(retired_abi, int) and not isinstance(retired_abi, bool)
             and retired_abi >= 0,
             f"{path}.retired_abi_major must be a non-negative integer", errors)
    _required_string(entry, "reason", path, errors)
    _require("replacement" in entry
             and (entry.get("replacement") is None
                  or (isinstance(entry.get("replacement"), str)
                      and bool(entry["replacement"].strip()))),
             f"{path}.replacement must be a non-empty string or null", errors)
    return identifier if isinstance(identifier, str) else None


def validate_registry(registry: Any) -> list[str]:
    """Return all schema and semantic errors in a decoded registry object."""

    errors: list[str] = []
    _require(isinstance(registry, dict), "registry root must be an object", errors)
    if not isinstance(registry, dict):
        return errors

    schema = registry.get("schema")
    _require(isinstance(schema, dict), "schema must be an object", errors)
    if isinstance(schema, dict):
        _require(schema.get("name") == SCHEMA_NAME,
                 f"schema.name must be {SCHEMA_NAME!r}", errors)
        _require(schema.get("version") == SCHEMA_VERSION,
                 f"schema.version must be {SCHEMA_VERSION}", errors)
        _require(isinstance(schema.get("abi_major"), int)
                 and not isinstance(schema.get("abi_major"), bool)
                 and schema.get("abi_major", -1) >= 0,
                 "schema.abi_major must be a non-negative integer", errors)

    knobs = registry.get("knobs")
    _require(isinstance(knobs, list) and bool(knobs), "knobs must be a non-empty list", errors)
    knob_ids: set[str] = set()
    knob_by_id: dict[str, dict[str, Any]] = {}
    if isinstance(knobs, list):
        for index, entry in enumerate(knobs):
            path = f"knobs[{index}]"
            _require(isinstance(entry, dict), f"{path} must be an object", errors)
            if not isinstance(entry, dict):
                continue
            identifier = entry.get("id")
            _require(isinstance(identifier, str) and bool(ID_RE.fullmatch(identifier or "")),
                     f"{path}.id must be a unique identifier", errors)
            if isinstance(identifier, str):
                _require(identifier not in knob_ids, f"duplicate knob id: {identifier}", errors)
                knob_ids.add(identifier)
                knob_by_id[identifier] = entry
            for field in ("state", "class", "disposition", "owner", "scope",
                          "surface", "architectural_effect", "exit_criterion"):
                _required_string(entry, field, path, errors)
            _require(entry.get("state") in ALLOWED_STATES,
                     f"{path}.state must be ACTIVE or PLANNED", errors)
            _require(entry.get("class") in ALLOWED_CLASSES,
                     f"{path}.class is not a supported knob class", errors)
            _require(entry.get("disposition") in ALLOWED_DISPOSITIONS,
                     f"{path}.disposition is not a supported disposition", errors)
            _require(entry.get("surface") in ALLOWED_SURFACES,
                     f"{path}.surface must be compile or runtime", errors)
            _require("default" in entry, f"{path}.default is required", errors)
            effect = entry.get("architectural_effect")
            _require(isinstance(effect, str) and effect.startswith("none"),
                     f"{path}.architectural_effect must begin with 'none'", errors)
            tests = entry.get("tests")
            _require(isinstance(tests, list) and bool(tests)
                     and all(isinstance(test, str) and bool(test.strip()) for test in tests),
                     f"{path}.tests must be a non-empty list of strings", errors)
            if entry.get("class") == "OBSERVE":
                _require(entry.get("surface") in {"compile", "runtime"},
                         f"{path}: OBSERVE surface is invalid", errors)
            if entry.get("class") == "A/B":
                implementations = entry.get("implementations")
                _require(isinstance(implementations, list)
                         and len(implementations) == 2
                         and all(isinstance(item, str) and bool(item.strip())
                                 for item in implementations)
                         and len(set(implementations)) == 2,
                         f"{path}.implementations must name two distinct implementations", errors)
                for field in ("comparison_corpus", "decision_owner", "selection_criterion"):
                    _required_string(entry, field, path, errors)

    bits = registry.get("observer_bits")
    observer_tombstones = registry.get("observer_bit_tombstones")
    knob_tombstones = registry.get("knob_tombstones")
    _require(isinstance(bits, list), "observer_bits must be a list", errors)
    _require(isinstance(observer_tombstones, list),
             "observer_bit_tombstones must be a list", errors)
    _require(isinstance(knob_tombstones, list), "knob_tombstones must be a list", errors)
    bit_ids: set[str] = set()
    c_bit_ids: set[str] = set()
    bit_values: dict[int, str] = {}
    if isinstance(bits, list):
        for index, entry in enumerate(bits):
            path = f"observer_bits[{index}]"
            _require(isinstance(entry, dict), f"{path} must be an object", errors)
            if not isinstance(entry, dict):
                continue
            identifier = entry.get("id")
            _require(isinstance(identifier, str) and bool(ID_RE.fullmatch(identifier or "")),
                     f"{path}.id must be a unique identifier", errors)
            if isinstance(identifier, str):
                _require(identifier not in bit_ids,
                         f"duplicate observer-bit id: {identifier}", errors)
                bit_ids.add(identifier)
            _validate_observer_identifiers(entry, path, c_bit_ids, errors)
            value = _parse_mask(entry.get("mask"), f"{path}.mask", errors)
            _validate_bit(entry, path, value, errors)
            if value is not None:
                _require(value not in bit_values,
                         f"duplicate observer-bit mask: {entry['mask']}", errors)
                bit_values[value] = str(identifier)
            for field in ("knob_id", "state", "meaning"):
                _required_string(entry, field, path, errors)
            _require(entry.get("state") == "ACTIVE",
                     f"{path}.state must be ACTIVE; removed bits belong in tombstones", errors)
            knob_id = entry.get("knob_id")
            _require(knob_id in knob_by_id,
                     f"{path}.knob_id must reference a declared knob", errors)
            if knob_id in knob_by_id:
                _require(knob_by_id[knob_id].get("class") == "OBSERVE",
                         f"{path}.knob_id must reference an OBSERVE knob", errors)
                _require(knob_by_id[knob_id].get("state") == "ACTIVE",
                         f"{path}.knob_id must reference an ACTIVE knob", errors)

    if isinstance(observer_tombstones, list):
        for index, entry in enumerate(observer_tombstones):
            path = f"observer_bit_tombstones[{index}]"
            _require(isinstance(entry, dict), f"{path} must be an object", errors)
            if not isinstance(entry, dict):
                continue
            identifier = entry.get("id")
            _require(isinstance(identifier, str) and bool(ID_RE.fullmatch(identifier or "")),
                     f"{path}.id must be a unique identifier", errors)
            if isinstance(identifier, str):
                _require(identifier not in bit_ids,
                         f"observer-bit id is active or tombstoned twice: {identifier}", errors)
                bit_ids.add(identifier)
            _validate_observer_identifiers(entry, path, c_bit_ids, errors)
            value = _parse_mask(entry.get("mask"), f"{path}.mask", errors)
            _validate_bit(entry, path, value, errors)
            if value is not None:
                _require(value not in bit_values,
                         f"observer-bit mask is active or tombstoned twice: {entry['mask']}", errors)
                bit_values[value] = str(identifier)
            _validate_tombstone(entry, path, errors)

    abi_major = schema.get("abi_major") if isinstance(schema, dict) else None
    if isinstance(abi_major, int) and not isinstance(abi_major, bool):
        for index, entry in enumerate(observer_tombstones or []):
            if isinstance(entry, dict) and isinstance(entry.get("retired_abi_major"), int):
                _require(entry["retired_abi_major"] <= abi_major,
                         f"observer_bit_tombstones[{index}] retired after current ABI major", errors)

    knob_tombstone_ids: set[str] = set()
    if isinstance(knob_tombstones, list):
        for index, entry in enumerate(knob_tombstones):
            path = f"knob_tombstones[{index}]"
            identifier = _validate_tombstone(entry, path, errors)
            if identifier is not None:
                _require(identifier not in knob_ids,
                         f"knob tombstone reuses active identifier: {identifier}", errors)
                _require(identifier not in knob_tombstone_ids,
                         f"duplicate knob tombstone id: {identifier}", errors)
                knob_tombstone_ids.add(identifier)
            if isinstance(abi_major, int) and isinstance(entry, dict):
                retired = entry.get("retired_abi_major")
                if isinstance(retired, int):
                    _require(retired <= abi_major,
                             f"{path} retired after current ABI major", errors)
    return errors


def _validate_observer_identifiers(entry: dict[str, Any], path: str,
                                   identifiers: set[str], errors: list[str]) -> None:
    kind_identifier = entry.get("kind_c_identifier")
    _require(isinstance(kind_identifier, str)
             and bool(C_IDENTIFIER_RE.fullmatch(kind_identifier)),
             f"{path}.kind_c_identifier must be a unique C trace identifier", errors)
    if isinstance(kind_identifier, str):
        _require(kind_identifier not in identifiers,
                 f"duplicate observer-bit kind C identifier: {kind_identifier}", errors)
        identifiers.add(kind_identifier)
    mask_identifier = entry.get("mask_c_identifier")
    _require(isinstance(mask_identifier, str)
             and bool(MASK_IDENTIFIER_RE.fullmatch(mask_identifier)),
             f"{path}.mask_c_identifier must be a unique C mask identifier", errors)
    if isinstance(mask_identifier, str):
        _require(mask_identifier not in identifiers,
                 f"duplicate observer-bit mask C identifier: {mask_identifier}", errors)
        identifiers.add(mask_identifier)


def load_registry(path: Path) -> dict[str, Any]:
    try:
        with path.open(encoding="utf-8") as stream:
            registry = json.load(stream)
    except (OSError, json.JSONDecodeError) as exc:
        raise RegistryError(f"cannot read registry {path}: {exc}") from exc
    errors = validate_registry(registry)
    if errors:
        raise RegistryError("\n".join(errors))
    return registry
