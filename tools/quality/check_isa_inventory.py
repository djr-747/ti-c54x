#!/usr/bin/env python3
"""Check the authoritative TI C54x ISA instruction-form inventory.

Module contract
---------------
Owns: repository-level inventory validation, exact production-catalog symbol
binding, provisional coverage arithmetic, and a deterministic CLI report.
Does not own: opcode values/masks, runtime execution, instruction semantics,
or the audit source data itself.  No environment or network state is read.
"""

from __future__ import annotations

import argparse
from pathlib import Path
import sys
from typing import Any, Iterable

try:
    from tools.isa.catalog.opcode_catalog_schema import OpcodeCatalogError, parse_catalog
    from tools.isa.inventory.schema import InventoryError, load_inventory
except ImportError:  # pragma: no cover - direct CLI fallback
    _ROOT = Path(__file__).resolve().parents[2]
    if str(_ROOT) not in sys.path:
        sys.path.insert(0, str(_ROOT))
    from tools.isa.catalog.opcode_catalog_schema import OpcodeCatalogError, parse_catalog
    from tools.isa.inventory.schema import InventoryError, load_inventory


def _inventory_path(root: Path) -> Path:
    return root / "config" / "isa" / "inventory.json"


def _catalog_path(root: Path) -> Path:
    return root / "src" / "isa" / "catalog" / "opcode_catalog.def"


def _catalog_symbols(root: Path) -> tuple[set[str], list[str]]:
    path = _catalog_path(root)
    try:
        rows = parse_catalog(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, OpcodeCatalogError) as error:
        return set(), [f"{path.relative_to(root)}: cannot parse catalog: {error}"]
    return {row.symbol for row in rows}, []


def check_repository(root: Path | str) -> list[str]:
    """Return sorted inventory and exact production-catalog binding errors."""

    resolved = Path(root).expanduser().resolve()
    if not resolved.is_dir():
        return ["ROOT: repository root does not exist or is not a directory"]
    try:
        inventory = load_inventory(_inventory_path(resolved))
    except InventoryError as error:
        return sorted(str(error).splitlines())
    catalog, errors = _catalog_symbols(resolved)
    if errors:
        return sorted(errors)
    bound = {entry["catalog_symbol"] for entry in inventory["catalog_bindings"]}
    for symbol in sorted(catalog - bound):
        errors.append(f"catalog symbol {symbol} is missing from inventory bindings")
    for symbol in sorted(bound - catalog):
        errors.append(f"inventory binding names unknown catalog symbol {symbol}")
    return sorted(set(errors))


def report_inventory(root: Path | str) -> dict[str, Any]:
    """Return the provisional semantic coverage report for a valid repository."""

    resolved = Path(root).expanduser().resolve()
    errors = check_repository(resolved)
    if errors:
        raise InventoryError("\n".join(errors))
    inventory = load_inventory(_inventory_path(resolved))
    denominator = inventory["basis"]["form_count"]
    bindings = inventory["catalog_bindings"]
    # Report semantic forms, not raw catalog rows: schema v1 requires one
    # binding per bound form, but using distinct IDs keeps aliases/multi-binding
    # edits from inflating the denominator arithmetic.
    bound_form_ids = {binding["form_id"] for binding in bindings}
    family = len(bound_form_ids)
    complete = sum(
        form["coverage"] == "COMPLETE" and form["id"] in bound_form_ids
        for form in inventory["forms"]
    )
    family_percent = family * 100.0 / denominator
    complete_percent = complete * 100.0 / denominator
    return {
        "catalog_family_progress": {"covered": family, "denominator": denominator},
        "strict_complete_lower_bound": {"covered": complete, "denominator": denominator},
        "catalog_family_percent": family_percent,
        "strict_complete_percent": complete_percent,
        "text": (
            f"Provisional ISA coverage: catalog-family {family}/{denominator} "
            f"({family_percent:.3f}%), strict complete-form lower bound "
            f"{complete}/{denominator} ({complete_percent:.3f}%)."
        ),
    }


def main(argv: Iterable[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[2])
    args = parser.parse_args(list(argv) if argv is not None else None)
    errors = check_repository(args.root)
    if errors:
        for error in errors:
            print(error, file=sys.stderr)
        print(f"isa inventory: FAIL ({len(errors)} error(s))", file=sys.stderr)
        return 1
    report = report_inventory(args.root)
    print(report["text"])
    return 0


if __name__ == "__main__":  # pragma: no cover
    raise SystemExit(main())


__all__ = ["check_repository", "main", "report_inventory"]
