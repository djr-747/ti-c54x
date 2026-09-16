#!/usr/bin/env python3
"""RED-first W45B reverse-carry inventory multi-binding contract."""
from pathlib import Path
import unittest
from tools.isa.inventory.schema import load_inventory
from tools.quality.check_isa_inventory import report_inventory

TOKEN="STANDARD_INDIRECT_MOD0_MOD3_MOD4_MOD5_MOD6_MOD7_MOD8_MOD10"
FAMILIES={"LD_SMEM":"FORM-T2-19-02","LD_SMEM_TS":"FORM-T2-19-03","STL_SMEM":"FORM-T2-20-09","LDU_SMEM":"FORM-T2-19-20","LD_T_SMEM":"FORM-T2-19-12","ST_IMMEDIATE_SMEM":"FORM-T2-20-04","LD_DP_SMEM":"FORM-T2-19-13","LD_ASM_SMEM":"FORM-T2-19-17","LD_SMEM_SHIFT16":"FORM-T2-19-04","ST_T_SMEM":"FORM-T2-20-02"}
class ReverseCarryInventoryTests(unittest.TestCase):
 @classmethod
 def setUpClass(cls): cls.root=Path(__file__).resolve().parents[5]; cls.path=cls.root/"config/isa/inventory.json"
 def test_projection_and_bindings(self):
  d=load_inventory(self.path); self.assertEqual(len(d["catalog_bindings"]),166)
  by={f["id"]:f for f in d["forms"]}
  for fam,fid in FAMILIES.items():
   self.assertEqual(len([b for b in d["catalog_bindings"] if b["form_id"]==fid]),7)
   self.assertEqual({b["addressing_subset"] for b in d["catalog_bindings"] if b["form_id"]==fid},{TOKEN})
   self.assertTrue({fam,fam+"_MOD4",fam+"_MOD5",fam+"_MOD6",fam+"_MOD7",fam+"_MOD8",fam+"_MOD10"} <= set(by[fid]["catalog_symbols"]))
 def test_coverage_unchanged(self):
  r=report_inventory(self.root); self.assertEqual(r["catalog_family_progress"],{"covered":66,"denominator":205}); self.assertEqual(r["strict_complete_lower_bound"],{"covered":33,"denominator":205})
if __name__=="__main__": unittest.main()
