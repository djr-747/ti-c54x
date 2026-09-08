"""ILP32 carrier-layout probe for the W49A input/effect seams.

Module contract: owns a bounded compile-only ABI check for the W49A test
assertions. It does not own production ABI policy, generated metadata, or
runtime execution coverage.
"""

from __future__ import annotations

import shutil
import subprocess
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[4]
SOURCES = (
    ROOT / "tests/unit/foundation/test_arithmetic40_addsub.c",
    ROOT / "tests/unit/state/status/test_overflow_fields.c",
    ROOT / "tests/unit/cpu/input/test_collect_ovm_status.c",
    ROOT / "tests/unit/sequencer/accumulator/test_accumulator_overflow_effect.c",
)


class W49ACarrierAbiTests(unittest.TestCase):
    """Compile W49A's assertion-bearing tests under a complete ILP32 ABI."""

    def test_supported_m32_assertions_compile(self) -> None:
        compiler = shutil.which("cc")
        if compiler is None:
            self.skipTest("native C compiler unavailable")

        probe = subprocess.run(
            [compiler, "-m32", "-std=c11", "-x", "c", "-fsyntax-only", "-"],
            input="#include <stdint.h>\nint main(void){return sizeof(uint64_t)!=8;}\n",
            cwd=ROOT,
            capture_output=True,
            text=True,
            check=False,
        )
        if probe.returncode != 0:
            self.skipTest("compiler does not support complete standard-header -m32")

        for source in SOURCES:
            result = subprocess.run(
                [
                    compiler,
                    "-m32",
                    "-std=c11",
                    "-fsyntax-only",
                    "-Iinclude",
                    "-Isrc",
                    str(source),
                ],
                cwd=ROOT,
                capture_output=True,
                text=True,
                check=False,
            )
            self.assertEqual(result.returncode, 0, result.stderr or str(source))


if __name__ == "__main__":
    unittest.main()
