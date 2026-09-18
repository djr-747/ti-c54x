#!/usr/bin/env python3
"""Inventory/catalog projection for the ADD/SUB accumulator-shift and SUB Smem rows.

Owns: exact row identity, form binding, and generated lookup growth
for W49B and P3-S19.  Does not own opcode execution, CPU integration,
or catalog parsing.
"""

from __future__ import annotations

from pathlib import Path
import unittest

from tools.isa.catalog.generate_opcode_lookup import render_lookup
from tools.isa.catalog.opcode_catalog_schema import parse_catalog
from tools.isa.inventory.schema import load_inventory
from tools.quality.check_isa_inventory import report_inventory


ROWS = {
    "ADD_ACC_SHIFT": {
        "form": "FORM-T2-01-09",
        "base": 0xF400,
        "mask": 0xFCE0,
        "layout": "TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT5",
        "resources": "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_OVM",
        "coverage": "COMPLETE",
        "addressing": "ALL",
    },
    "SUB_ACC_SHIFT": {
        "form": "FORM-T2-02-09",
        "base": 0xF420,
        "mask": 0xFCE0,
        "layout": "TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT5",
        "resources": "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_OVM",
        "coverage": "COMPLETE",
        "addressing": "ALL",
    },
    "SUB_SMEM": {
        "form": "FORM-T2-02-01",
        "base": 0x0800,
        "mask": 0xFE00,
        "layout": "TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7",
        "resources": "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_OVM",
        "coverage": "SUBSET",
        "addressing": "DIRECT_PAGE0_STANDARD_INDIRECT_MOD0_MOD3",
    },
    "SUB_SMEM_TS": {
        "form": "FORM-T2-02-02",
        "base": 0x0C00,
        "mask": 0xFE00,
        "layout": "TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7",
        "resources": "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_T_REGISTER_STATUS_SXM_OVM",
        "coverage": "SUBSET",
        "addressing": "DIRECT_PAGE0_STANDARD_INDIRECT_MOD0_MOD3",
    },
    "SUB_SMEM_SH16": {
        "form": "FORM-T2-02-03",
        "base": 0x4000,
        "mask": 0xFC00,
        "layout": "TI_C54X_OPERAND_LAYOUT_SOURCE_DESTINATION_INDIRECT_ADDR7",
        "resources": "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_OVM",
        "coverage": "SUBSET",
        "addressing": "DIRECT_PAGE0_STANDARD_INDIRECT_MOD0_MOD3",
    },
    "SUBB_SMEM": {
        "form": "FORM-T2-02-11",
        "base": 0x0E00,
        "mask": 0xFE00,
        "layout": "TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7",
        "resources": "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_C_OVM",
        "coverage": "SUBSET",
        "addressing": "DIRECT_PAGE0_STANDARD_INDIRECT_MOD0_MOD3",
    },
    "SUBS_SMEM": {
        "form": "FORM-T2-02-13",
        "base": 0x0A00,
        "mask": 0xFE00,
        "layout": "TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7",
        "resources": "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_OVM",
        "coverage": "SUBSET",
        "addressing": "DIRECT_PAGE0_STANDARD_INDIRECT_MOD0_MOD3",
    },
    "ADDC_SMEM": {
        "form": "FORM-T2-01-11",
        "base": 0x0600,
        "mask": 0xFE00,
        "layout": "TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7",
        "resources": "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_C_OVM",
        "coverage": "SUBSET",
        "addressing": "DIRECT_PAGE0_STANDARD_INDIRECT_MOD0_MOD3",
    },
    "ADDS_SMEM": {
        "form": "FORM-T2-01-13",
        "base": 0x0200,
        "mask": 0xFE00,
        "layout": "TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7",
        "resources": "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_OVM",
        "coverage": "SUBSET",
        "addressing": "DIRECT_PAGE0_STANDARD_INDIRECT_MOD0_MOD3",
    },
    "SUBC_SMEM": {
        "form": "FORM-T2-02-12",
        "base": 0x1E00,
        "mask": 0xFE00,
        "layout": "TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7",
        "resources": "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM",
        "coverage": "SUBSET",
        "addressing": "DIRECT_PAGE0_STANDARD_INDIRECT_MOD0_MOD3",
    },
    "ADD_LK_SHIFT": {
        "form": "FORM-T2-01-07",
        "words": 2,
        "base": 0xF000,
        "mask": 0xFCF0,
        "layout": "TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT4_EXTENSION16",
        "resources": "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_OVM",
        "coverage": "SUBSET",
        "addressing": "IMMEDIATE_LK",
    },
    "ADD_LK_16": {
        "form": "FORM-T2-01-08",
        "words": 2,
        "base": 0xF060,
        "mask": 0xFCFF,
        "layout": "TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_EXTENSION16_SHIFT16",
        "resources": "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_OVM",
        "coverage": "SUBSET",
        "addressing": "IMMEDIATE_LK",
    },
    "SUB_LK_SHIFT": {
        "form": "FORM-T2-02-07",
        "words": 2,
        "base": 0xF010,
        "mask": 0xFCF0,
        "layout": "TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT4_EXTENSION16",
        "resources": "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_OVM",
        "coverage": "SUBSET",
        "addressing": "IMMEDIATE_LK",
    },
    "SUB_LK_16": {
        "form": "FORM-T2-02-08",
        "words": 2,
        "base": 0xF061,
        "mask": 0xFCFF,
        "layout": "TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_EXTENSION16_SHIFT16",
        "resources": "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_OVM",
        "coverage": "SUBSET",
        "addressing": "IMMEDIATE_LK",
    },
    "ADD_XMEM": {
        "form": "FORM-T2-01-05",
        "base": 0x9000,
        "mask": 0xFE00,
        "layout": "TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_DUAL_XMOD_XAR_SHIFT4",
        "resources": "TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS_STATUS_SXM_OVM",
        "coverage": "COMPLETE",
        "addressing": "ALL",
    },
    "SUB_XMEM": {
        "form": "FORM-T2-02-05",
        "base": 0x9200,
        "mask": 0xFE00,
        "layout": "TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_DUAL_XMOD_XAR_SHIFT4",
        "resources": "TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS_STATUS_SXM_OVM",
        "coverage": "COMPLETE",
        "addressing": "ALL",
    },
    "ADD_SRC_ASM": {
        "form": "FORM-T2-01-10",
        "base": 0xF480,
        "mask": 0xFCFF,
        "layout": "TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION",
        "resources": "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_OVM_ASM",
        "coverage": "COMPLETE",
        "addressing": "ALL",
    },
    "SUB_SRC_ASM": {
        "form": "FORM-T2-02-10",
        "base": 0xF481,
        "mask": 0xFCFF,
        "layout": "TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION",
        "resources": "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_OVM_ASM",
        "coverage": "COMPLETE",
        "addressing": "ALL",
    },
    "ADD_SMEM_SHIFT": {
        "form": "FORM-T2-01-04",
        "words": 2,
        "base": 0x6F00,
        "mask": 0xFF00,
        "second_base": 0x0C00,
        "second_mask": 0xFCE0,
        "layout": "TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7_SOURCE_DESTINATION_SHIFT5",
        "resources": "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_OVM",
        "coverage": "SUBSET",
        "addressing": "DIRECT_PAGE0_STANDARD_INDIRECT_MOD0_MOD3",
    },
    "SUB_SMEM_SHIFT": {
        "form": "FORM-T2-02-04",
        "words": 2,
        "base": 0x6F00,
        "mask": 0xFF00,
        "second_base": 0x0C20,
        "second_mask": 0xFCE0,
        "layout": "TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7_SOURCE_DESTINATION_SHIFT5",
        "resources": "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_OVM",
        "coverage": "SUBSET",
        "addressing": "DIRECT_PAGE0_STANDARD_INDIRECT_MOD0_MOD3",
    },
}


class InventoryAddSubBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.root = Path(__file__).resolve().parents[5]
        cls.inventory_path = cls.root / "config/isa/inventory.json"
        cls.catalog_path = cls.root / "src/isa/catalog/opcode_catalog.def"

    def test_rows_forms_and_lookup_are_exact(self) -> None:
        inventory = load_inventory(self.inventory_path)
        rows = parse_catalog(self.catalog_path.read_text(encoding="utf-8"))
        by_symbol = {row.symbol: row for row in rows}
        by_form = {form["id"]: form for form in inventory["forms"]}
        bindings = {
            binding["catalog_symbol"]: binding
            for binding in inventory["catalog_bindings"]
        }

        self.assertEqual(len(rows), 291)
        self.assertEqual(len(rows) + 1, 292)
        self.assertEqual(len(inventory["catalog_bindings"]), 297)
        for symbol, expected in ROWS.items():
            with self.subTest(symbol=symbol):
                row = by_symbol[symbol]
                form = by_form[expected["form"]]
                binding = bindings[symbol]
                self.assertEqual(row.first_word_value, expected["base"])
                self.assertEqual(row.first_word_mask, expected["mask"])
                self.assertEqual(row.program_word_count, expected.get("words", 1))
                if "second_base" in expected:
                    self.assertEqual(row.second_word_value, expected["second_base"])
                    self.assertEqual(row.second_word_mask, expected["second_mask"])
                self.assertEqual(row.operand_layout, expected["layout"])
                self.assertEqual(
                    row.repeatability, "TI_C54X_REPEATABILITY_UNRESOLVED"
                )
                self.assertEqual(row.input_resources, expected["resources"])
                self.assertEqual(form["scope"], "COMMON_CANDIDATE")
                self.assertEqual(form["coverage"], expected["coverage"])
                self.assertEqual(form["catalog_symbols"], [symbol])
                self.assertEqual(binding["form_id"], expected["form"])
                self.assertEqual(binding["coverage"], expected["coverage"])
                self.assertEqual(
                    binding["addressing_subset"], expected["addressing"]
                )

        self.assertEqual(render_lookup(rows).count("= (uint16_t)"), 32265)
        report = report_inventory(self.root)
        self.assertEqual(
            report["catalog_family_progress"], {"covered": 139, "denominator": 205}
        )
        self.assertEqual(
            report["strict_complete_lower_bound"],
            {"covered": 40, "denominator": 205},
        )


if __name__ == "__main__":
    unittest.main()
