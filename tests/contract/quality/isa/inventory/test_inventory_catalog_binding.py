#!/usr/bin/env python3
"""Contracts for inventory-to-catalog identity binding.

Owns: one-to-one symbol identities, multi-binding subset forms, and the
    frozen 27 complete/93 subset checkpoint, including the DLD/DST LONG32
    MOD13 projection. Does not
own: opcode value/mask parsing or instruction execution semantics.
"""

from __future__ import annotations

import copy
import json
from pathlib import Path
import shutil
import tempfile
import unittest

from tools.quality.check_isa_inventory import check_repository
from tools.isa.inventory.schema import load_inventory, validate_inventory


STANDARD_INDIRECT_SUBSET = {
    "LD_SMEM",
    "LD_SMEM_TS",
    "STL_SMEM",
    "LDU_SMEM",
    "LD_T_SMEM",
    "ST_IMMEDIATE_SMEM",
    "LD_DP_SMEM",
    "LD_ASM_SMEM",
    "LD_SMEM_SHIFT16",
    "ST_T_SMEM",
}
ATOMIC_LOGICAL_SUBSET = {
    "ANDM_SMEM_MOD0_3",
    "ORM_SMEM_MOD0_3",
    "XORM_SMEM_MOD0_3",
    "ADDM_SMEM_MOD0_3",
}
DUAL_ARITHMETIC_SUBSET = {
    "ADD_DUAL_MOD0",
    "SUB_DUAL_MOD0",
    *(f"{mnemonic}_DUAL_M{x}_M{y}"
      for mnemonic in ("ADD", "SUB")
      for x in range(4)
      for y in range(4)
      if (x, y) != (0, 0)),
}
MAC_DUAL_SUBSET = {
    f"MAC_DUAL_M{x}_M{y}" for x in range(3) for y in range(3)
}
MPY_DUAL_SUBSET = {
    f"MPY_DUAL_M{x}_M{y}" for x in range(3) for y in range(3)
}
MAS_DUAL_SUBSET = {
    f"MAS_DUAL_M{x}_M{y}" for x in range(3) for y in range(3)
}
MACSU_DUAL_SUBSET = {
    f"MACSU_DUAL_M{x}_M{y}" for x in range(3) for y in range(3)
}


class InventoryCatalogBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.root = Path(__file__).resolve().parents[5]
        cls.path = cls.root / "config/isa/inventory.json"

    def inventory(self) -> dict:
        return load_inventory(self.path)

    def test_current_catalog_snapshot_binds_uniquely_with_frozen_coverage(self) -> None:
        inventory = self.inventory()
        bindings = inventory["catalog_bindings"]
        self.assertEqual(len(bindings),304)
        self.assertEqual(len({binding["catalog_symbol"] for binding in bindings}), 298)
        self.assertEqual(sum(binding["coverage"] == "COMPLETE" for binding in bindings), 40)
        self.assertEqual(sum(binding["coverage"] == "SUBSET" for binding in bindings), 264)
        self.assertEqual(
            {binding["catalog_symbol"] for binding in bindings if binding["coverage"] == "SUBSET"},
            STANDARD_INDIRECT_SUBSET
            | {f"{symbol}_MOD{mode}" for symbol in STANDARD_INDIRECT_SUBSET for mode in (4, 5, 6, 7, 8, 10)}
            | ATOMIC_LOGICAL_SUBSET
            | MAC_DUAL_SUBSET
            | MPY_DUAL_SUBSET
            | MAS_DUAL_SUBSET
            | {"DLD_LONG32_MOD13", "DST_LONG32_MOD13"}
            | DUAL_ARITHMETIC_SUBSET
            | {"LD_SMEM_SHIFT", "LDR", "LDR_MOD5", "LDR_MOD6", "B_PMAD", "BD", "BC_PMAD", "BCD", "BANZ", "BANZD", "CALL_PMAD", "CALLD", "CCD", "CC", "RET", "RETD", "RETE", "RETED", "RETF", "RC", "RCD", "XC", "BACC", "BACCD", "CALA", "CALAD", "STM", "RPT_K", "RPT_LK", "MAC_SMEM", "MPY_SMEM", "MPYA_SMEM", "MAS_SMEM", "MPYU_SMEM", "MACA_SMEM", "MPYA_DST", "SQUR_SMEM", "SQUR_ADST", "MACA_T", "SQURA_SMEM", "SQURS_SMEM", "MACP_SMEM", "MACD_SMEM", "MASA_SMEM", "MASA_T", "ADD_SMEM", "ADD_SMEM_TS", "ADD_SMEM_SH16", "STH_SMEM", "STH_SMEM_MOD4", "STH_SMEM_MOD5", "STH_SMEM_MOD6", "STH_SMEM_MOD7", "STH_SMEM_MOD8", "STH_SMEM_MOD10", "STH_ASM_SMEM", "STH_ASM_SMEM_MOD4", "STH_ASM_SMEM_MOD5", "STH_ASM_SMEM_MOD6", "STH_ASM_SMEM_MOD7", "STH_ASM_SMEM_MOD8", "STH_ASM_SMEM_MOD10", "STH_SHIFT_DIRECT", "STH_SHIFT_SMEM_MOD4", "STH_SHIFT_SMEM_MOD5", "STH_SHIFT_SMEM_MOD6", "STH_SHIFT_SMEM_MOD7", "STH_SHIFT_SMEM_MOD8", "STH_SHIFT_SMEM_MOD10",
               "STH_SHIFT_SMEM_MOD0", "STH_SHIFT_SMEM_MOD1",
               "STH_SHIFT_SMEM_MOD2", "STH_SHIFT_SMEM_MOD3",
               "MACSU_DUAL_M0_M0", "MACSU_DUAL_M0_M1",
               "MACSU_DUAL_M0_M2", "MACSU_DUAL_M1_M0",
               "MACSU_DUAL_M1_M1", "MACSU_DUAL_M1_M2",
               "MACSU_DUAL_M2_M0", "MACSU_DUAL_M2_M1",
               "MACSU_DUAL_M2_M2", "PORTR_DIRECT", "PORTR_SMEM",
               "PORTW_DIRECT", "PORTW_SMEM",
               "SUB_SMEM", "SUB_SMEM_TS", "SUB_SMEM_SH16",
               "AND_SMEM", "OR_SMEM", "XOR_SMEM", "SUBB_SMEM",
               "SUBS_SMEM", "ADDC_SMEM", "ADDS_SMEM", "SUBC_SMEM",
               "LTD_SMEM",
               "DELAY_SMEM",
               "ST_T_SMEM_DIRECT",
               "ST_TRN_SMEM_DIRECT",
               "STL_SMEM_DIRECT",
               "STL_ASM_SMEM_DIRECT",
               "STLM_SMEM_DIRECT",
               "LDM_SMEM_DIRECT",
               "ADD_LK_SHIFT", "ADD_LK_16",
               "SUB_LK_SHIFT", "SUB_LK_16",
               "ADD_SMEM_SHIFT", "SUB_SMEM_SHIFT",
               "MPY_SMEM_LK", "MAC_SMEM_LK",
               "RND_ACC", "SAT_ACC",
               "BITF_SMEM_LK", "CMPM_SMEM_LK", "BITT_SMEM",
               "ST_LK_SMEM",
               "ABS_ACC", "NEG_ACC", "NORM_ACC",
               "SFTA_ACC"},
        )
        for binding in bindings:
            if binding["coverage"] == "COMPLETE":
                self.assertEqual(binding["addressing_subset"], "ALL")
            elif binding["catalog_symbol"] in {"DLD_LONG32_MOD13", "DST_LONG32_MOD13"}:
                self.assertEqual(binding["addressing_subset"], "LONG_OFFSET_MOD13_CMPT0")
            elif binding["catalog_symbol"] in DUAL_ARITHMETIC_SUBSET | MAC_DUAL_SUBSET | MPY_DUAL_SUBSET | MAS_DUAL_SUBSET | MACSU_DUAL_SUBSET:
                symbol = binding["catalog_symbol"]
                if symbol.endswith("_MOD0"):
                    self.assertEqual(binding["addressing_subset"], "DUAL_INDIRECT_MOD0_MOD0")
                else:
                    modes = symbol.rsplit("_M", 2)
                    self.assertEqual(
                        binding["addressing_subset"],
                        f"DUAL_INDIRECT_MOD{modes[1]}_MOD{modes[2]}",
                    )
            else:
                expected = (
                    "ABSOLUTE_PMAD_NONDELAYED"
                    if binding["catalog_symbol"] in {"B_PMAD", "CALL_PMAD"}
                    else "ABSOLUTE_PMAD_DELAYED"
                    if binding["catalog_symbol"] in {"BD", "CALLD"}
                    else "ABSOLUTE_PMAD_COND"
                    if binding["catalog_symbol"] in ("BC_PMAD", "CC")
                    else "ABSOLUTE_PMAD_COND_DELAYED"
                    if binding["catalog_symbol"] in ("BCD", "CCD")
                    else "ABSOLUTE_PMAD_AUXILIARY"
                    if binding["catalog_symbol"] == "BANZ"
                    else "ABSOLUTE_PMAD_AUXILIARY_DELAYED"
                    if binding["catalog_symbol"] == "BANZD"
                    else "STACK_POP_NONDELAYED"
                    if binding["catalog_symbol"] in ("RET", "RETE", "RC")
                    else "STACK_POP_DELAYED"
                    if binding["catalog_symbol"] in ("RETD", "RETED", "RCD")
                    else "RTN_POP_NONDELAYED"
                    if binding["catalog_symbol"] == "RETF"
                    else "ACCUMULATOR_NONDELAYED"
                    if binding["catalog_symbol"] in ("BACC", "CALA")
                    else "ACCUMULATOR_DELAYED"
                    if binding["catalog_symbol"] in ("BACCD", "CALAD")
                    else "REGISTER_DST"
                    if binding["catalog_symbol"] in ("MPYA_DST", "SQUR_ADST",
                                                       "MACA_T", "MASA_T",
                                                       "RND_ACC", "SAT_ACC",
                                                       "ABS_ACC", "NEG_ACC",
                                                       "NORM_ACC",
                                                       "SFTA_ACC")
                    else "DIRECT_PAGE0"
                    if binding["catalog_symbol"] in ("STM", "ST_LK_SMEM",
                                                       "ST_T_SMEM_DIRECT",
                                                       "ST_TRN_SMEM_DIRECT",
                                                       "STL_SMEM_DIRECT",
                                                       "STL_ASM_SMEM_DIRECT",
                                                       "STLM_SMEM_DIRECT",
                                                       "LDM_SMEM_DIRECT")
                    else "DIRECT_PAGE0_STANDARD_INDIRECT_MOD0_MOD3"
                    if binding["catalog_symbol"] in ("MAC_SMEM", "MPY_SMEM",
                                                       "MPYA_SMEM", "MAS_SMEM",
                                                       "MPYU_SMEM", "MACA_SMEM",
                                                       "SQUR_SMEM", "SQURA_SMEM",
                                                       "SQURS_SMEM", "MACP_SMEM",
                                                       "MACD_SMEM", "MASA_SMEM",
                                                       "ADD_SMEM", "ADD_SMEM_TS",
                                                       "ADD_SMEM_SH16",
                                                       "SUB_SMEM", "SUB_SMEM_TS",
                                                       "SUB_SMEM_SH16",
                                                       "AND_SMEM", "OR_SMEM",
                                                       "XOR_SMEM", "SUBB_SMEM",
                                                       "SUBS_SMEM",
                                                       "ADDC_SMEM",
                                                       "ADDS_SMEM",
                                                       "SUBC_SMEM",
                                                       "LTD_SMEM",
                                                       "DELAY_SMEM",
                                                       "ADD_SMEM_SHIFT",
                                                       "SUB_SMEM_SHIFT",
                                                       "MPY_SMEM_LK",
                                                       "MAC_SMEM_LK",
                                                       "BITF_SMEM_LK",
                                                       "CMPM_SMEM_LK",
                                                       "BITT_SMEM")
                    else "IMMEDIATE_K"
                    if binding["catalog_symbol"] == "RPT_K"
                    else "IMMEDIATE_LK"
                    if binding["catalog_symbol"] in ("RPT_LK", "ADD_LK_SHIFT",
                                                     "ADD_LK_16",
                                                     "SUB_LK_SHIFT",
                                                     "SUB_LK_16")
                    else "STANDARD_INDIRECT_MOD0_MOD3"
                    if binding["catalog_symbol"] in ATOMIC_LOGICAL_SUBSET
                    or binding["catalog_symbol"] == "LD_SMEM_SHIFT"
                    else "STANDARD_INDIRECT_MOD0_MOD3_MOD5_MOD6"
                    if binding["catalog_symbol"]
                    in {"LDR", "LDR_MOD5", "LDR_MOD6"}
                    else "N_CONDITION"
                    if binding["catalog_symbol"] == "XC"
                    else "DIRECT_DP_STANDARD_INDIRECT_MOD0_MOD3_MOD4_MOD5_MOD6_MOD7_MOD8_MOD10"
                    if binding["catalog_symbol"]
                    in {"STH_SHIFT_DIRECT", "STH_SHIFT_SMEM_MOD0",
                        "STH_SHIFT_SMEM_MOD1", "STH_SHIFT_SMEM_MOD2",
                        "STH_SHIFT_SMEM_MOD3", "STH_SHIFT_SMEM_MOD4",
                        "STH_SHIFT_SMEM_MOD5", "STH_SHIFT_SMEM_MOD6",
                        "STH_SHIFT_SMEM_MOD7", "STH_SHIFT_SMEM_MOD8",
                        "STH_SHIFT_SMEM_MOD10"}
                    else "DIRECT_DP"
                    if binding["catalog_symbol"]
                    in ("PORTR_DIRECT", "PORTW_DIRECT")
                    else "STANDARD_INDIRECT_MOD0_MOD3_MOD4_MOD5_MOD6_MOD7_MOD8_MOD10"
                )
                self.assertEqual(binding["addressing_subset"], expected)
        self.assertTrue(all(binding["form_id"].startswith("FORM-T2-") for binding in bindings))

    def test_unknown_form_binding_and_duplicate_are_rejected_by_schema(self) -> None:
        inventory = self.inventory()
        unknown_form = copy.deepcopy(inventory)
        unknown_form["catalog_bindings"][0]["form_id"] = "FORM-T2-99-99"
        errors = validate_inventory(unknown_form)
        self.assertTrue(any("form" in error.lower() or "binding" in error.lower() for error in errors))

        duplicate = copy.deepcopy(inventory)
        duplicate["catalog_bindings"].append(copy.deepcopy(duplicate["catalog_bindings"][0]))
        errors = validate_inventory(duplicate)
        self.assertTrue(any("duplicate" in error.lower() or "once" in error.lower() for error in errors))

    def _repository_with_inventory(self, inventory: dict) -> Path:
        tempdir = tempfile.TemporaryDirectory()
        self.addCleanup(tempdir.cleanup)
        root = Path(tempdir.name)
        (root / "config/isa").mkdir(parents=True)
        (root / "src/isa/catalog").mkdir(parents=True)
        shutil.copy2(self.path, root / "config/isa/inventory.json")
        shutil.copy2(self.root / "src/isa/catalog/opcode_catalog.def", root / "src/isa/catalog/opcode_catalog.def")
        (root / "config/isa/inventory.json").write_text(json.dumps(inventory), encoding="utf-8")
        return root

    def test_unknown_catalog_symbol_and_missing_binding_are_checker_errors(self) -> None:
        inventory = self.inventory()
        unknown = copy.deepcopy(inventory)
        unknown["catalog_bindings"][0]["catalog_symbol"] = "NOT_A_CATALOG_SYMBOL"
        errors = check_repository(self._repository_with_inventory(unknown))
        self.assertTrue(any("catalog" in error.lower() or "unknown" in error.lower() for error in errors))

        missing = copy.deepcopy(inventory)
        missing["catalog_bindings"].pop()
        errors = check_repository(self._repository_with_inventory(missing))
        self.assertTrue(any("binding" in error.lower() or "catalog" in error.lower() for error in errors))
