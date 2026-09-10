#!/usr/bin/env python3
"""Project-contract checks for the single-source opcode catalog.

Owns: production-catalog and source-shape checks that prove the catalog/decode
seam derives from the sole exact/masked inventory. Does not own: schema
parsing, decoder behavior, opcode semantics, build manifests, or TI
architectural evidence.
"""

from __future__ import annotations

import dataclasses
import re
import unittest
from pathlib import Path

from tools.isa.catalog.opcode_catalog_schema import OpcodeRow, parse_catalog


LAYOUT_NONE = "TI_C54X_OPERAND_LAYOUT_NONE"
LAYOUT_STATUS_BIT = "TI_C54X_OPERAND_LAYOUT_STATUS_BIT"
LAYOUT_CONDITION_ARF = "TI_C54X_OPERAND_LAYOUT_CONDITION_ARF"
LAYOUT_ARF = "TI_C54X_OPERAND_LAYOUT_ARF"
LAYOUT_ARF_EXTENSION16 = "TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16"
LAYOUT_ACCUMULATOR_ARF = "TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF"
LAYOUT_ARF_MOD = "TI_C54X_OPERAND_LAYOUT_ARF_MOD"
LAYOUT_ARF_EXTENSION16_MOD = "TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16_MOD"
LAYOUT_ACCUMULATOR_ARF_MOD = "TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD"
LAYOUT_ACCUMULATOR_ARF_MOD_SHIFT5 = (
    "TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD_SHIFT5"
)
LAYOUT_ACCUMULATOR_SOURCE = "TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE"
LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT5 = (
    "TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT5"
)
LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT4_EXTENSION16 = (
    "TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT4_EXTENSION16"
)
LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_EXTENSION16_SHIFT16 = (
    "TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_EXTENSION16_SHIFT16"
)
REPEAT_UNRESOLVED = "TI_C54X_REPEATABILITY_UNRESOLVED"
REPEAT_INELIGIBLE = "TI_C54X_REPEATABILITY_INELIGIBLE"
RESOURCES_NONE = "TI_C54X_INPUT_RESOURCES_NONE"
RESOURCES_STATUS_SXM = "TI_C54X_INPUT_RESOURCES_STATUS_SXM"
RESOURCES_STATUS_SXM_OVM = "TI_C54X_INPUT_RESOURCES_STATUS_SXM_OVM"
RESOURCES_STATUS_SXM_T_REGISTER = (
    "TI_C54X_INPUT_RESOURCES_STATUS_SXM_T_REGISTER"
)
RESOURCES_ACCUMULATORS = "TI_C54X_INPUT_RESOURCES_ACCUMULATORS"
RESOURCES_ACCUMULATORS_STATUS_C = (
    "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_C"
)
RESOURCES_ACCUMULATORS_STATUS_TC = (
    "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_TC"
)
RESOURCES_AUXILIARY_REGISTERS = (
    "TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS"
)
RESOURCES_T_REGISTER = "TI_C54X_INPUT_RESOURCES_T_REGISTER"


class OpcodeCatalogContractTests(unittest.TestCase):
    def test_parser_accepts_the_frozen_exact_row(self) -> None:
        self.assertEqual(
            parse_catalog(
                f"TI_C54X_OPCODE_EXACT(NOP, nop, 0xF495, 1, {LAYOUT_NONE}, "
                f"{REPEAT_UNRESOLVED}, {RESOURCES_NONE})\n"
            ),
            [OpcodeRow("NOP", "nop", 0xF495, 0xFFFF, 1, LAYOUT_NONE,
                       REPEAT_UNRESOLVED, "EXACT", 1, RESOURCES_NONE)],
        )

    def test_parser_rejects_malformed_rows(self) -> None:
        malformed = (
            "TI_C54X_OPCODE_EXACT(NOP, nop, 0xF495)",
            "TI_C54X_OPCODE_EXACT(NOP, nop, 0xF495, 1, extra)",
            "TI_C54X_OPCODE(NOP, nop, 0xF495, 1)",
            "TI_C54X_OPCODE_EXACT(NOP, nop, 0xF495, one)",
            "TI_C54X_OPCODE_EXACT(NOP, nop, 0xF495, 1) trailing",
        )
        for text in malformed:
            with self.subTest(text=text):
                with self.assertRaises(ValueError):
                    parse_catalog(text)

    def test_parser_rejects_duplicate_and_reserved_ids(self) -> None:
        duplicate = "\n".join(
            (
                f"TI_C54X_OPCODE_EXACT(NOP, nop, 0xF495, 1, {LAYOUT_NONE}, {REPEAT_UNRESOLVED}, {RESOURCES_NONE})",
                f"TI_C54X_OPCODE_EXACT(NOP, nop, 0x1234, 1, {LAYOUT_NONE}, {REPEAT_UNRESOLVED}, {RESOURCES_NONE})",
            )
        )
        for symbol in ("UNKNOWN", "COUNT"):
            with self.subTest(symbol=symbol):
                with self.assertRaises(ValueError):
                    parse_catalog(
                        f"TI_C54X_OPCODE_EXACT({symbol}, nop, 0x0000, 1, "
                        f"{LAYOUT_NONE}, {REPEAT_UNRESOLVED}, {RESOURCES_NONE})"
                    )
        with self.assertRaises(ValueError):
            parse_catalog(duplicate)

    def test_parser_rejects_duplicate_first_words_and_handler_tokens(self) -> None:
        duplicate_word = "\n".join(
            (
                f"TI_C54X_OPCODE_EXACT(FOO, foo, 0x1234, 1, {LAYOUT_NONE}, {REPEAT_UNRESOLVED}, {RESOURCES_NONE})",
                f"TI_C54X_OPCODE_EXACT(BAR, bar, 0x1234, 2, {LAYOUT_NONE}, {REPEAT_UNRESOLVED}, {RESOURCES_NONE})",
            )
        )
        duplicate_handler = "\n".join(
            (
                f"TI_C54X_OPCODE_EXACT(FOO, shared, 0x1234, 1, {LAYOUT_NONE}, {REPEAT_UNRESOLVED}, {RESOURCES_NONE})",
                f"TI_C54X_OPCODE_EXACT(BAR, shared, 0x5678, 1, {LAYOUT_NONE}, {REPEAT_UNRESOLVED}, {RESOURCES_NONE})",
            )
        )
        for text in (duplicate_word, duplicate_handler):
            with self.subTest(text=text):
                with self.assertRaises(ValueError):
                    parse_catalog(text)

    def test_parser_rejects_handler_tokens_and_bad_ranges(self) -> None:
        invalid = (
            "TI_C54X_OPCODE_EXACT(NOP, NOP, 0xF495, 1, "
            f"{LAYOUT_NONE}, {REPEAT_UNRESOLVED}, {RESOURCES_NONE})",
            "TI_C54X_OPCODE_EXACT(NOP, nop-handler, 0xF495, 1, "
            f"{LAYOUT_NONE}, {REPEAT_UNRESOLVED}, {RESOURCES_NONE})",
            f"TI_C54X_OPCODE_EXACT(NOP, nop, 0x10000, 1, {LAYOUT_NONE}, {REPEAT_UNRESOLVED}, {RESOURCES_NONE})",
            f"TI_C54X_OPCODE_EXACT(NOP, nop, 0x-1, 1, {LAYOUT_NONE}, {REPEAT_UNRESOLVED}, {RESOURCES_NONE})",
            f"TI_C54X_OPCODE_EXACT(NOP, nop, 0xF495, 0, {LAYOUT_NONE}, {REPEAT_UNRESOLVED}, {RESOURCES_NONE})",
            f"TI_C54X_OPCODE_EXACT(NOP, nop, 0xF495, 256, {LAYOUT_NONE}, {REPEAT_UNRESOLVED}, {RESOURCES_NONE})",
        )
        for text in invalid:
            with self.subTest(text=text):
                with self.assertRaises(ValueError):
                    parse_catalog(text)

    def test_real_catalog_contains_current_catalog_rows(self) -> None:
        root = Path(__file__).resolve().parents[3]
        catalog_rows = parse_catalog(
            (root / "src/isa/catalog/opcode_catalog.def").read_text()
        )
        self.assertEqual(len(catalog_rows), 146)
        for row, symbol, handler, value in (
            (catalog_rows[1], "SSBX_STATUS", "ssbx_status", 0xF5B0),
            (catalog_rows[2], "RSBX_STATUS", "rsbx_status", 0xF4B0),
        ):
            with self.subTest(symbol=symbol):
                self.assertEqual(row.symbol, symbol)
                self.assertEqual(row.handler, handler)
                self.assertEqual(row.first_word_value, value)
                self.assertEqual(row.first_word_mask, 0xFDF0)
                self.assertEqual(row.program_word_count, 1)
                self.assertEqual(row.operand_layout, LAYOUT_STATUS_BIT)
                self.assertEqual(row.repeatability, REPEAT_INELIGIBLE)
                self.assertEqual(row.input_resources, RESOURCES_NONE)

        # Preserve the historical positional assertions below while the
        # current catalog carries the status rows immediately after NOP and
        # the W49B ADD/SUB rows before the immediate logical forms.
        rows = [
            catalog_rows[0],
            *(row for row in catalog_rows[3:]
              if row.symbol not in {
                  "LD_SMEM_TS",
                  "ADD_ACC_SHIFT",
                  "SUB_ACC_SHIFT",
              }),
        ]
        self.assertEqual(
            dataclasses.replace(rows[0], source_line=0),
            OpcodeRow("NOP", "nop", 0xF495, 0xFFFF, 1, LAYOUT_NONE,
                      REPEAT_UNRESOLVED, "EXACT", 0, RESOURCES_NONE),
        )
        self.assertGreater(rows[0].source_line, 0)
        self.assertEqual(rows[1].symbol, "LD_IMMEDIATE_DP")
        self.assertEqual(rows[1].first_word_value, 0xEA00)
        self.assertEqual(rows[1].first_word_mask, 0xFE00)
        self.assertEqual(rows[2].symbol, "LD_IMMEDIATE_ASM")
        self.assertEqual(rows[2].first_word_value, 0xED00)
        self.assertEqual(rows[2].first_word_mask, 0xFFE0)
        self.assertEqual(rows[3].symbol, "LD_IMMEDIATE_ARP")
        self.assertEqual(rows[3].first_word_value, 0xF4A0)
        self.assertEqual(rows[3].first_word_mask, 0xFFF8)
        self.assertEqual(rows[4].symbol, "LD_SMEM")
        self.assertEqual(rows[4].handler, "ld_smem")
        self.assertEqual(rows[4].first_word_value, 0x1080)
        self.assertEqual(rows[4].first_word_mask, 0xFEE0)
        ld_smem_ts = next(
            row for row in catalog_rows if row.symbol == "LD_SMEM_TS"
        )
        self.assertEqual(ld_smem_ts.handler, "ld_smem_ts")
        self.assertEqual(ld_smem_ts.first_word_value, 0x1480)
        self.assertEqual(ld_smem_ts.first_word_mask, 0xFEE0)
        self.assertEqual(ld_smem_ts.program_word_count, 1)
        self.assertEqual(ld_smem_ts.operand_layout, LAYOUT_ACCUMULATOR_ARF_MOD)
        self.assertEqual(ld_smem_ts.repeatability, REPEAT_UNRESOLVED)
        self.assertEqual(
            ld_smem_ts.input_resources, RESOURCES_STATUS_SXM_T_REGISTER
        )
        self.assertEqual(rows[5].symbol, "STL_SMEM")
        self.assertEqual(rows[5].handler, "stl_smem")
        self.assertEqual(rows[5].first_word_value, 0x8080)
        self.assertEqual(rows[5].first_word_mask, 0xFEE0)
        self.assertEqual(rows[6].symbol, "MAX_ACC")
        self.assertEqual(rows[6].first_word_value, 0xF486)
        self.assertEqual(rows[6].first_word_mask, 0xFEFF)
        self.assertEqual(rows[6].input_resources, RESOURCES_ACCUMULATORS)
        self.assertEqual(rows[7].symbol, "MIN_ACC")
        self.assertEqual(rows[7].first_word_value, 0xF487)
        self.assertEqual(rows[7].first_word_mask, 0xFEFF)
        self.assertEqual(rows[7].input_resources, RESOURCES_ACCUMULATORS)
        self.assertEqual(rows[8].symbol, "EXP_ACC")
        self.assertEqual(rows[8].handler, "exp_acc")
        self.assertEqual(rows[8].first_word_value, 0xF48E)
        self.assertEqual(rows[8].first_word_mask, 0xFEFF)
        self.assertEqual(rows[8].program_word_count, 1)
        self.assertEqual(rows[8].operand_layout, LAYOUT_ACCUMULATOR_SOURCE)
        self.assertEqual(rows[8].repeatability, REPEAT_UNRESOLVED)
        self.assertEqual(rows[8].input_resources, RESOURCES_ACCUMULATORS)
        self.assertEqual(rows[9].symbol, "CMPL_ACC")
        self.assertEqual(rows[9].first_word_value, 0xF493)
        self.assertEqual(rows[9].first_word_mask, 0xFCFF)
        self.assertEqual(rows[9].input_resources, RESOURCES_ACCUMULATORS)
        self.assertEqual(rows[10].symbol, "ROL_ACC")
        self.assertEqual(rows[10].handler, "rol_acc")
        self.assertEqual(rows[10].first_word_value, 0xF491)
        self.assertEqual(rows[10].first_word_mask, 0xFEFF)
        self.assertEqual(rows[10].program_word_count, 1)
        self.assertEqual(rows[10].operand_layout, LAYOUT_ACCUMULATOR_SOURCE)
        self.assertEqual(rows[10].repeatability, REPEAT_UNRESOLVED)
        self.assertEqual(rows[10].input_resources,
                         RESOURCES_ACCUMULATORS_STATUS_C)
        self.assertEqual(rows[11].symbol, "ROR_ACC")
        self.assertEqual(rows[11].handler, "ror_acc")
        self.assertEqual(rows[11].first_word_value, 0xF490)
        self.assertEqual(rows[11].first_word_mask, 0xFEFF)
        self.assertEqual(rows[11].program_word_count, 1)
        self.assertEqual(rows[11].operand_layout, LAYOUT_ACCUMULATOR_SOURCE)
        self.assertEqual(rows[11].repeatability, REPEAT_UNRESOLVED)
        self.assertEqual(rows[11].input_resources,
                         RESOURCES_ACCUMULATORS_STATUS_C)
        for row, symbol, handler, value, resources in (
            (rows[12], "ROLTC_ACC", "roltc_acc", 0xF492,
             RESOURCES_ACCUMULATORS_STATUS_TC),
            (rows[13], "SFTC_ACC", "sftc_acc", 0xF494,
             RESOURCES_ACCUMULATORS),
        ):
            with self.subTest(symbol=symbol):
                self.assertEqual(row.symbol, symbol)
                self.assertEqual(row.handler, handler)
                self.assertEqual(row.first_word_value, value)
                self.assertEqual(row.first_word_mask, 0xFEFF)
                self.assertEqual(row.program_word_count, 1)
                self.assertEqual(row.operand_layout,
                                 LAYOUT_ACCUMULATOR_SOURCE)
                self.assertEqual(row.repeatability, REPEAT_UNRESOLVED)
                self.assertEqual(row.input_resources, resources)
        self.assertEqual(rows[14].symbol, "SFTL_ACC")
        self.assertEqual(rows[14].handler, "sftl_acc")
        self.assertEqual(rows[14].first_word_value, 0xF0E0)
        self.assertEqual(rows[14].first_word_mask, 0xFCE0)
        self.assertEqual(rows[14].program_word_count, 1)
        self.assertEqual(rows[14].operand_layout,
                         LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT5)
        self.assertEqual(rows[14].repeatability, REPEAT_UNRESOLVED)
        self.assertEqual(rows[14].input_resources, RESOURCES_ACCUMULATORS)
        for row, symbol, handler, value in (
            (rows[15], "AND_ACC_SHIFT", "and_acc_shift", 0xF080),
            (rows[16], "OR_ACC_SHIFT", "or_acc_shift", 0xF0A0),
            (rows[17], "XOR_ACC_SHIFT", "xor_acc_shift", 0xF0C0),
        ):
            with self.subTest(symbol=symbol):
                self.assertEqual(row.symbol, symbol)
                self.assertEqual(row.handler, handler)
                self.assertEqual(row.first_word_value, value)
                self.assertEqual(row.first_word_mask, 0xFCE0)
                self.assertEqual(row.program_word_count, 1)
                self.assertEqual(
                    row.operand_layout,
                    LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT5,
                )
                self.assertEqual(row.repeatability, REPEAT_UNRESOLVED)
                self.assertEqual(row.input_resources, RESOURCES_ACCUMULATORS)
        for row, symbol, handler, value, mask, layout in (
            (rows[18], "AND_IMMEDIATE_SHIFT", "and_immediate_shift",
             0xF030, 0xFCF0,
             LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT4_EXTENSION16),
            (rows[19], "AND_IMMEDIATE_SHIFT16", "and_immediate_shift16",
             0xF063, 0xFCFF,
             LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_EXTENSION16_SHIFT16),
            (rows[20], "OR_IMMEDIATE_SHIFT", "or_immediate_shift",
             0xF040, 0xFCF0,
             LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT4_EXTENSION16),
            (rows[21], "OR_IMMEDIATE_SHIFT16", "or_immediate_shift16",
             0xF064, 0xFCFF,
             LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_EXTENSION16_SHIFT16),
            (rows[22], "XOR_IMMEDIATE_SHIFT", "xor_immediate_shift",
             0xF050, 0xFCF0,
             LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT4_EXTENSION16),
            (rows[23], "XOR_IMMEDIATE_SHIFT16", "xor_immediate_shift16",
             0xF065, 0xFCFF,
             LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_EXTENSION16_SHIFT16),
        ):
            with self.subTest(symbol=symbol):
                self.assertEqual(row.symbol, symbol)
                self.assertEqual(row.handler, handler)
                self.assertEqual(row.first_word_value, value)
                self.assertEqual(row.first_word_mask, mask)
                self.assertEqual(row.program_word_count, 2)
                self.assertEqual(row.operand_layout, layout)
                self.assertEqual(row.repeatability, REPEAT_UNRESOLVED)
                self.assertEqual(row.input_resources, RESOURCES_ACCUMULATORS)

        rows_by_symbol = {row.symbol: row for row in catalog_rows}

        cmpr = rows_by_symbol["CMPR_AR"]
        self.assertEqual(cmpr.symbol, "CMPR_AR")
        self.assertEqual(cmpr.handler, "cmpr")
        self.assertEqual(cmpr.first_word_value, 0xF4A8)
        self.assertEqual(cmpr.first_word_mask, 0xFCF8)
        self.assertEqual(cmpr.program_word_count, 1)
        self.assertEqual(cmpr.operand_layout, LAYOUT_CONDITION_ARF)
        self.assertEqual(cmpr.repeatability, REPEAT_INELIGIBLE)
        self.assertEqual(cmpr.input_resources, RESOURCES_AUXILIARY_REGISTERS)

        ldu = rows_by_symbol["LDU_SMEM"]
        self.assertEqual(ldu.symbol, "LDU_SMEM")
        self.assertEqual(ldu.handler, "ldu_smem")
        self.assertEqual(ldu.first_word_value, 0x1280)
        self.assertEqual(ldu.first_word_mask, 0xFEE0)
        self.assertEqual(ldu.program_word_count, 1)
        self.assertEqual(ldu.operand_layout, LAYOUT_ACCUMULATOR_ARF_MOD)
        self.assertEqual(ldu.repeatability, REPEAT_UNRESOLVED)
        self.assertEqual(ldu.input_resources, RESOURCES_NONE)

        ld_t = rows_by_symbol["LD_T_SMEM"]
        self.assertEqual(ld_t.symbol, "LD_T_SMEM")
        self.assertEqual(ld_t.handler, "ld_t_smem")
        self.assertEqual(ld_t.first_word_value, 0x3080)
        self.assertEqual(ld_t.first_word_mask, 0xFFE0)
        self.assertEqual(ld_t.program_word_count, 1)
        self.assertEqual(ld_t.operand_layout, LAYOUT_ARF_MOD)
        self.assertEqual(ld_t.repeatability, REPEAT_UNRESOLVED)
        self.assertEqual(ld_t.input_resources, RESOURCES_NONE)

        st_immediate = rows_by_symbol["ST_IMMEDIATE_SMEM"]
        self.assertEqual(st_immediate.symbol, "ST_IMMEDIATE_SMEM")
        self.assertEqual(st_immediate.handler, "st_immediate_smem")
        self.assertEqual(st_immediate.first_word_value, 0x7680)
        self.assertEqual(st_immediate.first_word_mask, 0xFFE0)
        self.assertEqual(st_immediate.program_word_count, 2)
        self.assertEqual(st_immediate.operand_layout, LAYOUT_ARF_EXTENSION16_MOD)
        self.assertEqual(st_immediate.repeatability, REPEAT_UNRESOLVED)
        self.assertEqual(st_immediate.input_resources, RESOURCES_NONE)

        ld_dp = rows_by_symbol["LD_DP_SMEM"]
        self.assertEqual(ld_dp.symbol, "LD_DP_SMEM")
        self.assertEqual(ld_dp.handler, "ld_dp_smem")
        self.assertEqual(ld_dp.first_word_value, 0x4680)
        self.assertEqual(ld_dp.first_word_mask, 0xFFE0)
        self.assertEqual(ld_dp.program_word_count, 1)
        self.assertEqual(ld_dp.operand_layout, LAYOUT_ARF_MOD)
        self.assertEqual(ld_dp.repeatability, REPEAT_INELIGIBLE)
        self.assertEqual(ld_dp.input_resources, RESOURCES_NONE)

        ld_asm = rows_by_symbol["LD_ASM_SMEM"]
        self.assertEqual(ld_asm.symbol, "LD_ASM_SMEM")
        self.assertEqual(ld_asm.handler, "ld_asm_smem")
        self.assertEqual(ld_asm.first_word_value, 0x3280)
        self.assertEqual(ld_asm.first_word_mask, 0xFFE0)
        self.assertEqual(ld_asm.program_word_count, 1)
        self.assertEqual(ld_asm.operand_layout, LAYOUT_ARF_MOD)
        self.assertEqual(ld_asm.repeatability, REPEAT_UNRESOLVED)
        self.assertEqual(ld_asm.input_resources, RESOURCES_NONE)

        ld_smem_shift16 = rows_by_symbol["LD_SMEM_SHIFT16"]
        self.assertEqual(ld_smem_shift16.symbol, "LD_SMEM_SHIFT16")
        self.assertEqual(ld_smem_shift16.handler, "ld_smem_shift16")
        self.assertEqual(ld_smem_shift16.first_word_value, 0x4480)
        self.assertEqual(ld_smem_shift16.first_word_mask, 0xFEE0)
        self.assertEqual(ld_smem_shift16.program_word_count, 1)
        self.assertEqual(ld_smem_shift16.operand_layout,
                         LAYOUT_ACCUMULATOR_ARF_MOD)
        self.assertEqual(ld_smem_shift16.repeatability, REPEAT_UNRESOLVED)
        self.assertEqual(ld_smem_shift16.input_resources, RESOURCES_STATUS_SXM)

        ld_smem_shift = rows_by_symbol["LD_SMEM_SHIFT"]
        self.assertEqual(ld_smem_shift.symbol, "LD_SMEM_SHIFT")
        self.assertEqual(ld_smem_shift.handler, "ld_smem_shift")
        self.assertEqual(ld_smem_shift.first_word_value, 0x6F80)
        self.assertEqual(ld_smem_shift.first_word_mask, 0xFFE0)
        self.assertEqual(ld_smem_shift.program_word_count, 2)
        self.assertEqual(ld_smem_shift.operand_layout,
                         LAYOUT_ACCUMULATOR_ARF_MOD_SHIFT5)
        self.assertEqual(ld_smem_shift.repeatability, REPEAT_UNRESOLVED)
        self.assertEqual(ld_smem_shift.input_resources,
                         RESOURCES_STATUS_SXM_OVM)

        st_t = rows_by_symbol["ST_T_SMEM"]
        self.assertEqual(st_t.symbol, "ST_T_SMEM")
        self.assertEqual(st_t.handler, "st_t_smem")
        self.assertEqual(st_t.first_word_value, 0x8C80)
        self.assertEqual(st_t.first_word_mask, 0xFFE0)
        self.assertEqual(st_t.program_word_count, 1)
        self.assertEqual(st_t.operand_layout, LAYOUT_ARF_MOD)
        self.assertEqual(st_t.repeatability, REPEAT_UNRESOLVED)
        self.assertEqual(st_t.input_resources, RESOURCES_T_REGISTER)

    def test_catalog_has_distinct_x_macro_consumers_and_generated_lookup(self) -> None:
        root = Path(__file__).resolve().parents[3]
        catalog = root / "src/isa/catalog/opcode_catalog.def"
        source_paths = tuple((root / "src/isa").rglob("*.c")) + tuple(
            (root / "src/isa").rglob("*.h")
        )
        consumers = {
            path: path.read_text()
            for path in source_paths
            if path != catalog and "opcode_catalog.def" in path.read_text()
        }
        self.assertGreaterEqual(len(consumers), 3)
        for path, text in consumers.items():
            with self.subTest(path=path):
                self.assertIn("#define TI_C54X_OPCODE_EXACT", text)
                self.assertIn("#define TI_C54X_OPCODE_MASKED", text)

        all_sources = "\n".join(path.read_text() for path in source_paths)
        self.assertIn("TI_C54X_OPCODE_EXACT", all_sources)
        self.assertIn("TI_C54X_OPCODE_MASKED", all_sources)
        self.assertIn("static const uint16_t", all_sources)
        self.assertRegex(all_sources, r"\[\s*UINT16_MAX\s*\+\s*1U\s*\]")
        self.assertIn("_Static_assert", all_sources)
        self.assertIn("first_word_value", all_sources)
        self.assertIn("ti_c54x_op_##handler", all_sources)

        catalog_source = (root / "src/isa/catalog/opcode_catalog.c").read_text()
        self.assertIn(
            '#include "../../../generated/isa/catalog/opcode_first_word.inc"',
            catalog_source,
        )
        self.assertEqual(catalog_source.count("ti_c54x_opcode_direct_lookup"), 4)
        self.assertEqual(catalog_source.count('#include "opcode_catalog.def"'), 1)
        self.assertEqual(
            catalog_source.count(
                '#include "../../../generated/isa/catalog/opcode_first_word.inc"'
            ),
            1,
        )

        generated = root / "generated/isa/catalog/opcode_first_word.inc"
        self.assertTrue(generated.is_file())
        self.assertIn("TI_C54X_OPCODE_ID_NOP", generated.read_text())
        handwritten = "\n".join(
            path.read_text() for path in source_paths
            if path != catalog
        )
        self.assertIsNone(
            re.search(r"0\s*x\s*f\s*4\s*9\s*5", handwritten, flags=re.IGNORECASE)
        )


if __name__ == "__main__":  # pragma: no cover
    unittest.main()
