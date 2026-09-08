"""Authoritative TI C54x instruction-form inventory tooling.

Module contract
---------------
Owns: immutable-in-spirit parsing and validation of the audit inventory
metadata in ``config/isa/inventory.json``.  Does not own: opcode values or
masks, which remain solely in the production X-macro catalog, or runtime ISA
execution.  Callers receive freshly decoded containers and retain ownership
of no parser state.
"""

from .schema import InventoryError, load_inventory, validate_inventory

__all__ = ["InventoryError", "load_inventory", "validate_inventory"]
