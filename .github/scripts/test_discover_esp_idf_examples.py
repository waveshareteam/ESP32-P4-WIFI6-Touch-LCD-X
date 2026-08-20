#!/usr/bin/env python3
"""Regression tests for the display-aware ESP-IDF CI matrix."""

from __future__ import annotations

import contextlib
import io
import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock


sys.path.insert(0, str(Path(__file__).resolve().parent))
import discover_esp_idf_examples as discovery  # noqa: E402


SHARED = (
    "examples/esp-idf/01_HowToCreateProject",
    "examples/esp-idf/02_HelloWorld",
    "examples/esp-idf/03_i2c_tools",
    "examples/esp-idf/04_wifistation",
    "examples/esp-idf/05_sdmmc",
    "examples/esp-idf/06_I2SCodec",
)
DISPLAY = (
    "examples/esp-idf/07_Displaycolorbar",
    "examples/esp-idf/08_lvgl_demo_v9",
    "examples/esp-idf/09_video_lcd_display",
    "examples/esp-idf/10_mp4_player",
    "examples/esp-idf/11_esp_brookesia_phone",
    "examples/esp-idf/12_usb_extend_screen",
)
EXAMPLES = SHARED + DISPLAY


class DiscoveryTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.manifest = discovery.load_display_manifest()

    def matrix(self, selected: tuple[str, ...] = EXAMPLES, variant: str = "all") -> list[dict[str, object]]:
        return discovery.build_matrix(list(selected), self.manifest, variant)["include"]

    def test_manifest_matches_repository_contract(self) -> None:
        discovery.validate_manifest_against_repository(self.manifest, set(EXAMPLES))
        self.assertEqual(tuple(self.manifest["display_examples"]), DISPLAY)
        self.assertEqual([entry["slug"] for entry in self.manifest["variants"]], ["lcd-7", "lcd-8", "lcd-10-1"])

    def test_gitignore_keeps_lcd_overlays_trackable(self) -> None:
        gitignore = Path(".gitignore").read_text(encoding="utf-8")
        self.assertIn("!config/sdkconfig/", gitignore)
        self.assertIn("!config/sdkconfig/*.defaults", gitignore)

    def test_full_matrix_is_48_with_18_artifacts(self) -> None:
        matrix = self.matrix()
        self.assertEqual(len(matrix), 48)
        self.assertEqual(sum(entry["package_artifact"] == "true" for entry in matrix), 18)
        self.assertEqual(sum(entry["display_variant"] == "shared" for entry in matrix), 12)
        with_sha = discovery.build_matrix(list(DISPLAY[:1]), self.manifest, "lcd-7", "0123456789abcdef0123456789abcdef01234567")["include"]
        self.assertEqual(with_sha[1]["artifact_name"], "ESP32-P4-WIFI6-Touch-LCD-7-07_Displaycolorbar-rev3_x-6.0.2-0123456789ab")
        self.assertTrue(all(entry["revision_profile"] == "rev3_x" for entry in matrix))

    def test_shared_and_display_direct_changes_have_expected_rows(self) -> None:
        self.assertEqual(len(self.matrix((SHARED[0],))), 2)
        self.assertEqual(len(self.matrix((DISPLAY[0],))), 6)

    def test_single_overlay_routes_only_six_display_examples_for_two_idfs(self) -> None:
        for variant in ("lcd-7", "lcd-8", "lcd-10-1"):
            with self.subTest(variant=variant):
                selected, routed_variant = discovery.discover_selection_from_paths(
                    [f"config/sdkconfig/{variant}.defaults"], set(EXAMPLES), self.manifest
                )
                self.assertEqual(selected, list(DISPLAY))
                self.assertEqual(routed_variant, variant)
                matrix = self.matrix(tuple(selected), routed_variant)
                self.assertEqual(len(matrix), 12)
                self.assertEqual(
                    {entry["display_variant"] for entry in matrix}, {variant}
                )

    def test_mixed_overlay_inputs_lock_all_variants_in_every_order(self) -> None:
        cases = (
            ("shared", ["config/sdkconfig/lcd-7.defaults", f"{SHARED[0]}/main/main.c"], 38),
            ("display", ["config/sdkconfig/lcd-7.defaults", f"{DISPLAY[0]}/main/main.c"], 36),
            ("global", ["config/sdkconfig/lcd-7.defaults", ".github/workflows/esp-idf-examples.yml"], 48),
        )
        for name, paths, count in cases:
            for ordered in (paths, list(reversed(paths))):
                with self.subTest(name=name, paths=ordered):
                    selected, variant = discovery.discover_selection_from_paths(ordered, set(EXAMPLES), self.manifest)
                    self.assertEqual(variant, "all")
                    self.assertEqual(len(self.matrix(tuple(selected), variant)), count)

    def test_two_overlays_and_rename_other_end_lock_all_variants(self) -> None:
        selected, variant = discovery.discover_selection_from_paths(["config/sdkconfig/lcd-7.defaults", "config/sdkconfig/lcd-8.defaults"], set(EXAMPLES), self.manifest)
        self.assertEqual(selected, list(DISPLAY))
        self.assertEqual(len(self.matrix(tuple(selected), variant)), 36)
        paths = discovery.parse_name_status(["R100", "config/sdkconfig/lcd-7.defaults", f"{SHARED[0]}/main/main.c"])
        selected, variant = discovery.discover_selection_from_paths(paths, set(EXAMPLES), self.manifest)
        self.assertEqual(variant, "all")
        self.assertEqual(len(self.matrix(tuple(selected), variant)), 38)

    def test_global_inputs_select_full_matrix(self) -> None:
        for path in (
            "config/display-variants.json",
            ".gitignore",
            ".github/workflows/esp-idf-examples.yml",
            ".github/scripts/build_esp_idf_artifact.py",
        ):
            with self.subTest(path=path):
                selected, variant = discovery.discover_selection_from_paths([path], set(EXAMPLES), self.manifest)
                self.assertEqual(len(self.matrix(tuple(selected), variant)), 48)

    def test_docs_and_firmware_do_not_select_examples(self) -> None:
        for path in (
            "README.md",
            "docs/ci-artifacts.md",
            "firmware/factory.bin",
            "firmware/brookesia/main/app_main.c",
            "examples/arduino/examples/01_DisplayColorBars/01_DisplayColorBars.ino",
            "examples/arduino/libraries/lcd_x/src/lcd_x_board.h",
            ".github/workflows/arduino-examples.yml",
            ".github/scripts/discover_arduino_examples.py",
            ".github/scripts/test_discover_arduino_examples.py",
        ):
            with self.subTest(path=path):
                selected, _ = discovery.discover_selection_from_paths([path], set(EXAMPLES), self.manifest)
                self.assertEqual(selected, [])

    def test_arduino_only_ci_inputs_do_not_trigger_maintained_firmware(self) -> None:
        for path in (
            "examples/arduino/examples/01_DisplayColorBars/01_DisplayColorBars.ino",
            "examples/arduino/libraries/lcd_x/src/lcd_x_board.h",
            ".github/workflows/arduino-examples.yml",
            ".github/scripts/discover_arduino_examples.py",
            ".github/scripts/test_discover_arduino_examples.py",
        ):
            with self.subTest(path=path):
                self.assertTrue(discovery.is_arduino_only_path(path))
                self.assertFalse(discovery.firmware_build_impact([path]))

    def test_all_lightweight_inputs_are_no_build_and_cli_wired(self) -> None:
        expected = frozenset(
            {
                ".gitattributes",
                ".github/markdown-audit-config.json",
                ".github/scripts/audit_markdown.py",
                ".github/scripts/check_markdown_links.py",
                ".github/scripts/test_audit_markdown.py",
                ".github/scripts/test_check_markdown_links.py",
                ".github/workflows/docs.yml",
                "flash-ci-firmware.cmd",
                "scripts/flash-ci-firmware.ps1",
            }
        )
        self.assertEqual(discovery.LIGHTWEIGHT_ONLY_PATHS, expected)
        for path in sorted(expected):
            with self.subTest(path=path):
                selected, variant = discovery.discover_selection_from_paths(
                    [path], set(EXAMPLES), self.manifest
                )
                self.assertEqual(selected, [])
                self.assertEqual(self.matrix(tuple(selected), variant), [])
                self.assertTrue(discovery.is_lightweight_only_path(path))
                self.assertFalse(discovery.is_docs_only_path(path))

                with tempfile.TemporaryDirectory() as directory:
                    output = Path(directory) / "github-output.txt"
                    with (
                        mock.patch.object(
                            discovery, "changed_paths_from_git", return_value=[path]
                        ),
                        mock.patch.dict(
                            os.environ, {"GITHUB_OUTPUT": str(output)}
                        ),
                        contextlib.redirect_stdout(io.StringIO()),
                    ):
                        self.assertEqual(
                            discovery.main(["--base-ref", "base", "--head-ref", "head"]),
                            0,
                        )
                    values = dict(
                        line.split("=", 1)
                        for line in output.read_text(encoding="utf-8").splitlines()
                    )
                self.assertEqual(json.loads(values["matrix"]), {"include": []})
                self.assertEqual(values["has_examples"], "false")
                self.assertEqual(values["docs_only"], "false")
                self.assertEqual(
                    values["firmware_build_impact"],
                    "true" if discovery.firmware_build_impact([path]) else "false",
                )

    def test_shared_ci_safety_inputs_trigger_firmware_builds_but_docs_do_not(self) -> None:
        expected = frozenset(
            {
                ".github/ci-routing-config.json",
                ".github/scripts/build_esp_idf_artifact.py",
                ".github/scripts/discover_esp_idf_examples.py",
                ".github/scripts/test_build_esp_idf_artifact.py",
                ".github/scripts/test_discover_esp_idf_examples.py",
                ".github/scripts/test_repository_policy.py",
                ".github/workflows/esp-idf-examples.yml",
                ".github/workflows/maintained-firmware.yml",
            }
        )
        self.assertEqual(discovery.SHARED_CI_SAFETY_INPUTS, expected)
        for path in expected:
            with self.subTest(path=path):
                self.assertTrue(discovery.firmware_build_impact([path]))
        self.assertFalse(discovery.firmware_build_impact([".github/workflows/docs.yml"]))
        for path in ("README.md", "docs/ci-artifacts.md"):
            with self.subTest(path=path):
                self.assertFalse(discovery.firmware_build_impact([path]))

    def test_docs_root_file_kinds_and_document_like_sources(self) -> None:
        for path in ("README.md", "docs/guide.md", "docs/guide.pdf", "assets/board.png", "schematic/board.pdf"):
            with self.subTest(path=path):
                self.assertTrue(discovery.is_docs_only_path(path))
        for path in ("docs/generate.py", "assets/generate.py", "schematic/check.py", f"{SHARED[0]}/main/security_manager.c"):
            with self.subTest(path=path):
                self.assertFalse(discovery.is_docs_only_path(path))
        selected, variant = discovery.discover_selection_from_paths([f"{SHARED[0]}/main/security_manager.c"], set(EXAMPLES), self.manifest)
        self.assertEqual(len(self.matrix(tuple(selected), variant)), 2)

    def test_documentation_assets_and_nested_licenses_do_not_select_examples(self) -> None:
        paths = (
            "assets/ESP32-P4-WIFI6-Touch-LCD-X.jpg",
            "schematic/ESP32-P4-Connect-Adapter-Schematic.pdf",
            "schematic/ESP32-P4-WIFI6-Touch-LCD-X-Schematic.pdf",
            "examples/esp-idf/08_lvgl_demo_v9/components/bsp_extra/LICENSE",
            "examples/esp-idf/11_esp_brookesia_phone/components/brookesia_core/license.txt",
            "examples/esp-idf/12_usb_extend_screen/components/bsp_extra/LICENSE",
            "examples/esp-idf/11_esp_brookesia_phone/components/brookesia_core/docs/_static/readme/app_launcher_demo.png",
            "examples/esp-idf/11_esp_brookesia_phone/components/brookesia_core/systems/speaker/docs/assets/animation/boot/boot_animation_360_360.gif",
        )
        for path in paths:
            with self.subTest(path=path):
                self.assertTrue(discovery.is_docs_only_path(path))
                selected, variant = discovery.discover_selection_from_paths(
                    [path], set(EXAMPLES), self.manifest
                )
                self.assertEqual(selected, [])
                self.assertEqual(self.matrix(tuple(selected), variant), [])

    def test_unknown_path_is_conservative_global_input(self) -> None:
        selected, variant = discovery.discover_selection_from_paths(["tools/build-policy.json"], set(EXAMPLES), self.manifest)
        self.assertEqual(len(self.matrix(tuple(selected), variant)), 48)

    def test_firmware_file_kinds_and_rename_semantics(self) -> None:
        self.assertEqual(discovery.classify_firmware_path("firmware/factory.bin"), "binary")
        self.assertEqual(discovery.classify_firmware_path("firmware/delivery.zip"), "archive")
        paths = discovery.parse_name_status(["R100", f"{DISPLAY[0]}/main/old.c", "docs/renamed.md"])
        selected, _ = discovery.discover_selection_from_paths(paths, set(EXAMPLES), self.manifest)
        self.assertEqual(selected, [DISPLAY[0]])

    def test_firmware_all_kinds_and_outputs_remain_backward_compatible(self) -> None:
        paths = ["firmware/README.md", "firmware/main/app_main.c", "firmware/CMakeLists.txt", "firmware/factory.bin", "firmware/delivery.zip", "firmware/notes.dat"]
        self.assertEqual([discovery.classify_firmware_path(path) for path in paths], ["markdown", "source", "config", "binary", "archive", "other"])
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "github-output.txt"
            with mock.patch.object(discovery, "changed_paths_from_git", return_value=paths), mock.patch.dict(os.environ, {"GITHUB_OUTPUT": str(output)}), contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(discovery.main(["--base-ref", "base"]), 0)
            values = dict(line.split("=", 1) for line in output.read_text(encoding="utf-8").splitlines())
        self.assertEqual(values["has_examples"], "false")
        self.assertEqual(values["firmware_kinds"], "archive,binary,config,markdown,other,source")
        self.assertEqual(values["release_review"], "true")

    def test_only_real_brookesia_sources_configs_and_resources_trigger_maintained_build(self) -> None:
        for path in (
            "firmware/brookesia/main/app_main.c",
            "firmware/brookesia/CMakeLists.txt",
            "firmware/brookesia/assets/startup.dat",
        ):
            with self.subTest(path=path):
                self.assertTrue(discovery.firmware_build_impact([path]))
        for path in (
            "firmware/brookesia/README.md",
            "firmware/brookesia/docs/architecture.md",
            "firmware/brookesia/FactoryOnly.bin",
            "firmware/ESP32-P4-WIFI6-Touch-LCD-7-FactoryOnly.bin",
            "firmware/release.zip",
        ):
            with self.subTest(path=path):
                self.assertFalse(discovery.firmware_build_impact([path]))

    def test_canonical_inventory_and_nested_projects_are_excluded(self) -> None:
        self.assertEqual(discovery.list_examples(), list(EXAMPLES))
        self.assertNotIn("examples/esp-idf/11_esp_brookesia_phone/components/brookesia_core/test_apps", discovery.list_examples())

    def test_only_canonical_direct_children_are_discovered(self) -> None:
        old_cwd = Path.cwd()
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for project in (root / "examples" / "esp-idf" / "direct", root / "examples" / "esp-idf" / "direct" / "components" / "upstream" / "test_app", root / "examples" / "esp-idf-v5" / "legacy"):
                (project / "main").mkdir(parents=True)
                (project / "CMakeLists.txt").touch()
            try:
                os.chdir(root)
                self.assertEqual(discovery.list_examples(), ["examples/esp-idf/direct"])
            finally:
                os.chdir(old_cwd)

    def test_only_wifi_and_sdmmc_have_sdkconfig_ci(self) -> None:
        with_overlays = {entry["example"] for entry in self.matrix() if entry["sdkconfig_ci"]}
        self.assertEqual(with_overlays, {"examples/esp-idf/04_wifistation", "examples/esp-idf/05_sdmmc"})

    def test_wifi_ci_credentials_fit_esp_idf_bounds(self) -> None:
        values = dict(line.split("=", 1) for line in Path("examples/esp-idf/04_wifistation/sdkconfig.ci").read_text(encoding="utf-8").splitlines())
        self.assertLessEqual(len(values["CONFIG_ESP_WIFI_SSID"].strip('"').encode("utf-8")), 31)
        self.assertLessEqual(len(values["CONFIG_ESP_WIFI_PASSWORD"].strip('"').encode("utf-8")), 63)

    def test_invalid_variant_and_malformed_manifest_fail_closed(self) -> None:
        with self.assertRaises(ValueError):
            discovery.build_matrix(list(EXAMPLES), self.manifest, "lcd-42")
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "variants.json"
            path.write_text('{"display_examples":[],"variants":[]}', encoding="utf-8")
            with self.assertRaises(discovery.ManifestError):
                discovery.load_display_manifest(path)

    def test_malformed_empty_and_unavailable_git_scope_fail_closed(self) -> None:
        for record in (["R100", "old.c"], ["C100", "old.c"], ["M"], []):
            with self.subTest(record=record), self.assertRaises(discovery.DiffScopeError):
                discovery.parse_name_status(record)
        with mock.patch.object(discovery, "run_git", side_effect=subprocess.CalledProcessError(128, ["git", "diff"])), contextlib.redirect_stderr(io.StringIO()):
            self.assertEqual(discovery.main(["--base-ref", "base"]), 2)

    def test_deleted_path_retains_its_build_impact(self) -> None:
        paths = discovery.parse_name_status(["D", f"{SHARED[0]}/main/removed.c"])
        selected, variant = discovery.discover_selection_from_paths(paths, set(EXAMPLES), self.manifest)
        self.assertEqual(len(self.matrix(tuple(selected), variant)), 2)

    def test_real_git_base_head_scope_when_git_is_available(self) -> None:
        if not shutil.which("git"):
            self.skipTest("git is unavailable to this Python process")
        old_cwd = Path.cwd()
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            try:
                subprocess.run(["git", "init", "--quiet"], cwd=root, check=True)
                subprocess.run(["git", "config", "user.email", "ci@example.invalid"], cwd=root, check=True)
                subprocess.run(["git", "config", "user.name", "CI Test"], cwd=root, check=True)
                (root / "README.md").write_text("base\n", encoding="utf-8")
                subprocess.run(["git", "add", "."], cwd=root, check=True)
                subprocess.run(["git", "commit", "--quiet", "-m", "base"], cwd=root, check=True)
                base = subprocess.run(["git", "rev-parse", "HEAD"], cwd=root, check=True, text=True, stdout=subprocess.PIPE).stdout.strip()
                (root / "README.md").write_text("changed\n", encoding="utf-8")
                subprocess.run(["git", "commit", "-am", "docs", "--quiet"], cwd=root, check=True)
                os.chdir(root)
                self.assertEqual(discovery.changed_paths_from_git(base, "HEAD"), ["README.md"])
            finally:
                os.chdir(old_cwd)

    def test_cli_writes_backward_compatible_outputs(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "github-output.txt"
            with mock.patch.object(discovery, "changed_paths_from_git", return_value=["README.md"]), mock.patch.dict(os.environ, {"GITHUB_OUTPUT": str(output)}), contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(discovery.main(["--base-ref", "base", "--head-ref", "head"]), 0)
            values = dict(line.split("=", 1) for line in output.read_text(encoding="utf-8").splitlines())
            self.assertEqual(json.loads(values["matrix"]), {"include": []})
            self.assertEqual(values["has_examples"], "false")
            self.assertEqual(values["docs_only"], "true")
            self.assertEqual(values["firmware_touched"], "false")
            self.assertEqual(values["release_review"], "false")
            self.assertIn("examples", values)
            self.assertIn("firmware_kinds", values)

    def test_dispatch_variant_and_unknown_example(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "github-output.txt"
            stdout = io.StringIO()
            with mock.patch.dict(os.environ, {"GITHUB_OUTPUT": str(output)}), contextlib.redirect_stdout(stdout):
                self.assertEqual(discovery.main(["--example", "all", "--variant", "lcd-7"]), 0)
            self.assertEqual(len(json.loads(stdout.getvalue())["include"]), 12)
        with contextlib.redirect_stderr(io.StringIO()):
            self.assertEqual(discovery.main(["--example", "missing-example"]), 1)
        for request in ("02_HelloWorld", "examples/esp-idf/02_HelloWorld"):
            with self.subTest(request=request), contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(discovery.main(["--example", request]), 0)

    def test_workflow_uses_variant_matrix_and_artifact_upload(self) -> None:
        workflow = Path(".github/workflows/esp-idf-examples.yml").read_text(encoding="utf-8")
        self.assertIn("inputs.variant", workflow)
        self.assertIn("matrix.display_variant", workflow)
        self.assertIn("matrix.package_artifact == 'true'", workflow)
        self.assertIn("actions/upload-artifact@v7", workflow)
        self.assertIn("github.event.pull_request.head.sha || github.sha", workflow)
        self.assertIn("--expected-checkout-sha", workflow)
        self.assertIn("archive: false", workflow)
        self.assertNotIn("name: source-built-", workflow)
        self.assertIn("name: ESP-IDF CI gate", workflow)
        self.assertIn("needs: [discover, build]", workflow)
        self.assertIn("if: always()", workflow)
        self.assertNotIn("--fallback-all", workflow)
        self.assertIn("test_build_esp_idf_artifact.py", workflow)
        self.assertIn("if: always()", workflow)


if __name__ == "__main__":
    unittest.main(verbosity=2)
