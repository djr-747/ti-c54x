#!/usr/bin/env python3
"""Focused fixtures for the observer-disabled artifact checker.

Owns: checker parsing and command-boundary fixtures.  Does not own:
compilation, CMake/Make integration, or observer implementation behavior.
"""

from __future__ import annotations

import contextlib
import io
import tempfile
import unittest
from pathlib import Path

from tools.quality import check_observer_artifact


class ObserverArtifactTests(unittest.TestCase):
    def test_posix_nm_archive_headers_and_symbols_are_parsed(self) -> None:
        output = """artifact.a[observer.o]:
observer.c a 0
ti_c54x_cpu_step T 0 20
ti_c54x_observer_emit U
"""
        self.assertEqual(
            check_observer_artifact.parse_nm_symbols(output),
            ("observer.c", "ti_c54x_cpu_step", "ti_c54x_observer_emit"),
        )

    def test_only_observer_callback_and_gate_symbols_are_forbidden(self) -> None:
        symbols = (
            "ti_c54x_cpu_step",
            "ti_c54x_observer_emit",
            "ti_c54x_observer_init",
            "TI_C54X_OBSERVE_ENABLED",
        )
        self.assertEqual(
            check_observer_artifact.forbidden_symbols(symbols),
            (
                "TI_C54X_OBSERVE_ENABLED",
                "ti_c54x_observer_emit",
                "ti_c54x_observer_init",
            ),
        )

    def test_unrelated_trace_symbol_is_allowed(self) -> None:
        self.assertEqual(
            check_observer_artifact.forbidden_symbols(("ti_c54x_trace_mask_for_kind",)),
            (),
        )

    def test_missing_artifact_is_deterministic(self) -> None:
        errors = check_observer_artifact.check_artifact(Path("/missing/artifact.a"))
        self.assertEqual(errors, ("observer artifact does not exist: /missing/artifact.a",))

    def test_fake_nm_proves_both_definitions_and_references(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            artifact = root / "libcore.a"
            artifact.write_bytes(b"fixture")
            nm = root / "nm-fixture"
            nm.write_text(
                "#!/bin/sh\n"
                "printf '%s\\n' 'libcore.a[cpu.o]:' 'cpu.c a 0' 'ti_c54x_observer_emit U'\n",
                encoding="utf-8",
            )
            nm.chmod(0o755)
            errors = check_observer_artifact.check_artifact(artifact, nm)
            self.assertEqual(
                errors,
                ("observer-disabled artifact contains forbidden symbol: ti_c54x_observer_emit",),
            )

    def test_cli_reports_success_for_clean_fake_artifact(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            artifact = root / "libcore.a"
            artifact.write_bytes(b"fixture")
            nm = root / "nm-fixture"
            nm.write_text(
                "#!/bin/sh\n"
                "printf '%s\\n' 'libcore.a[cpu.o]:' 'ti_c54x_cpu_step T 0 20'\n",
                encoding="utf-8",
            )
            nm.chmod(0o755)
            stdout = io.StringIO()
            with contextlib.redirect_stdout(stdout):
                status = check_observer_artifact.main(
                    ["--artifact", str(artifact), "--nm", str(nm)]
                )
            self.assertEqual(status, 0)
            self.assertIn("observer-disabled artifact OK", stdout.getvalue())


if __name__ == "__main__":
    unittest.main()
