#!/usr/bin/env python3
"""Regression tests for the LCD-X Arduino CI routing matrix."""

from __future__ import annotations

import contextlib
import io
import json
import os
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock


sys.path.insert(0, str(Path(__file__).resolve().parent))
import discover_arduino_examples as discovery  # noqa: E402


EXAMPLES = (
    "examples/arduino/examples/01_DisplayColorBars",
    "examples/arduino/examples/02_TouchDrawing",
)


class ArduinoDiscoveryTests(unittest.TestCase):
    def test_inventory_matches_canonical_sketches(self) -> None:
        self.assertEqual(discovery.CANONICAL_ROOT, Path("examples/arduino/examples"))
        self.assertEqual(tuple(discovery.list_examples()), EXAMPLES)
        for sketch in EXAMPLES:
            self.assertTrue((Path(sketch) / f"{Path(sketch).name}.ino").is_file())

    def test_full_matrix_compiles_each_sketch_for_three_variants(self) -> None:
        matrix = discovery.build_matrix(list(EXAMPLES))["include"]
        self.assertEqual(len(matrix), 6)
        self.assertEqual(
            {(row["path"], row["display_variant"]) for row in matrix},
            {(example, variant) for example in EXAMPLES for variant, _ in discovery.DISPLAY_VARIANTS},
        )
        self.assertEqual(
            {row["display_variant"]: row["display_value"] for row in matrix[:3]},
            {"lcd-7": "7", "lcd-8": "8", "lcd-10-1": "101"},
        )

    def test_direct_sketch_change_selects_only_that_sketch_for_all_variants(self) -> None:
        selected = discovery.discover_from_paths([f"{EXAMPLES[1]}/02_TouchDrawing.ino"], set(EXAMPLES))
        self.assertEqual(selected, [EXAMPLES[1]])
        self.assertEqual(len(discovery.build_matrix(selected)["include"]), 3)

    def test_shared_library_and_arduino_ci_inputs_select_every_sketch(self) -> None:
        for path in (
            "examples/arduino/libraries/lcd_x/src/lcd_x_board.h",
            ".github/workflows/arduino-examples.yml",
            ".github/scripts/discover_arduino_examples.py",
            ".github/scripts/test_discover_arduino_examples.py",
            ".github/ci-routing-config.json",
        ):
            with self.subTest(path=path):
                selected = discovery.discover_from_paths([path], set(EXAMPLES))
                self.assertEqual(selected, list(EXAMPLES))
                self.assertEqual(len(discovery.build_matrix(selected)["include"]), 6)

    def test_docs_firmware_and_esp_idf_only_paths_do_not_select_arduino(self) -> None:
        for path in (
            "README.md",
            f"{EXAMPLES[0]}/README.md",
            "examples/arduino/libraries/lcd_x/LICENSE",
            "firmware/brookesia/main/app_main.c",
            "examples/esp-idf/01_HowToCreateProject/main/main.c",
            ".github/workflows/esp-idf-examples.yml",
            ".github/workflows/maintained-firmware.yml",
        ):
            with self.subTest(path=path):
                self.assertEqual(discovery.discover_from_paths([path], set(EXAMPLES)), [])

    def test_rename_and_unknown_paths_are_conservative(self) -> None:
        paths = discovery.parse_name_status(["R100", f"{EXAMPLES[0]}/01_DisplayColorBars.ino", "config/shared-build-input.h"])
        self.assertEqual(discovery.discover_from_paths(paths, set(EXAMPLES)), list(EXAMPLES))
        self.assertEqual(discovery.discover_from_paths(["unclassified-source.txt"], set(EXAMPLES)), list(EXAMPLES))

    def test_empty_and_malformed_diff_scopes_fail_closed(self) -> None:
        with self.assertRaises(discovery.DiffScopeError):
            discovery.parse_name_status([])
        with self.assertRaises(discovery.DiffScopeError):
            discovery.parse_name_status(["R100", "old-path"])
        with contextlib.redirect_stderr(io.StringIO()), mock.patch.object(discovery, "changed_paths_from_git", side_effect=discovery.DiffScopeError("missing")):
            self.assertEqual(discovery.main(["--base-ref", "base", "--head-ref", "head"]), 2)

    def test_cli_outputs_match_workflow_contract(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "github-output.txt"
            stdout = io.StringIO()
            with mock.patch.dict(os.environ, {"GITHUB_OUTPUT": str(output)}), contextlib.redirect_stdout(stdout):
                self.assertEqual(discovery.main(["--example", "01_DisplayColorBars", "--variant", "lcd-8"]), 0)
            values = dict(line.split("=", 1) for line in output.read_text(encoding="utf-8").splitlines())
        self.assertEqual(len(json.loads(values["matrix"])["include"]), 1)
        self.assertEqual(values["has_examples"], "true")
        self.assertEqual(values["examples"], EXAMPLES[0])
        self.assertEqual(json.loads(stdout.getvalue())["include"][0]["display_value"], "8")
        with contextlib.redirect_stderr(io.StringIO()):
            self.assertEqual(discovery.main(["--example", "unknown"]), 1)

    def test_workflow_uses_pinned_toolchain_and_display_build_property(self) -> None:
        workflow = Path(".github/workflows/arduino-examples.yml").read_text(encoding="utf-8")
        for required in (
            'ARDUINO_CLI_VERSION: "1.5.1"',
            'ARDUINO_CORE_VERSION: "3.3.11"',
            'ChipVariant=postv3',
            'PSRAM=enabled',
            'FlashSize=32M',
            'PartitionScheme=app13M_data7M_32MB',
            'compiler.cpp.extra_flags=-DLCD_X_DISPLAY_VARIANT=${{ matrix.display_value }}',
            'name: Arduino CI gate',
            'cancel-in-progress: true',
            'matrix: ${{ fromJson(needs.discover.outputs.matrix) }}',
        ):
            with self.subTest(required=required):
                self.assertIn(required, workflow)


if __name__ == "__main__":
    unittest.main(verbosity=2)
