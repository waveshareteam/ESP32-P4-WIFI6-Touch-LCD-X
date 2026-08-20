#!/usr/bin/env python3
"""Unit tests for source-built ESP-IDF CI artifact packaging."""

from __future__ import annotations

import json
import shutil
import sys
import tempfile
import unittest
import zipfile
from pathlib import Path
from unittest import mock


sys.path.insert(0, str(Path(__file__).resolve().parent))
import build_esp_idf_artifact as artifact  # noqa: E402


VARIANT = {
    "slug": "lcd-7",
    "product": "ESP32-P4-WIFI6-Touch-LCD-7",
    "label": "7-inch ILI9881C",
    "overlay": "config/sdkconfig/lcd-7.defaults",
    "kconfig": "CONFIG_BSP_LCD_TYPE_720_1280_7_INCH",
    "resolution": "720x1280",
    "panel": "ILI9881C",
}


class ArtifactTests(unittest.TestCase):
    def fixture(self) -> tuple[Path, Path]:
        root = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, root, ignore_errors=True)
        project = root / "examples" / "esp-idf" / "demo"
        (project / "build" / "bootloader").mkdir(parents=True)
        (root / "config" / "sdkconfig").mkdir(parents=True)
        (project / "sdkconfig.defaults").write_text("CONFIG_DEFAULT=y\n", encoding="utf-8")
        (project / "sdkconfig.ci").write_text("CONFIG_CI=y\n", encoding="utf-8")
        (root / VARIANT["overlay"]).write_text("CONFIG_BSP_LCD_TYPE_720_1280_7_INCH=y\n", encoding="utf-8")
        (root / "config" / "sdkconfig" / "rev1_3.defaults").write_text("CONFIG_ESP32P4_REV_MIN_100=y\n", encoding="utf-8")
        (root / "config" / "sdkconfig" / "rev3_x.defaults").write_text("CONFIG_ESP32P4_REV_MIN_300=y\n", encoding="utf-8")
        (root / "config" / "display-variants.json").write_text(json.dumps({"display_examples": ["examples/esp-idf/demo"], "variants": [VARIANT]}), encoding="utf-8")
        (project / "build" / "bootloader" / "bootloader.bin").write_bytes(b"boot")
        (project / "build" / "demo.bin").write_bytes(b"app")
        (project / "build" / "flasher_args.json").write_text(json.dumps({"write_flash_args": ["--flash-mode", "dio", "--flash-size", "32MB"], "flash_files": {"0x1000": "bootloader/bootloader.bin", "0x10000": "demo.bin"}}), encoding="utf-8")
        return root, project

    def maintained_fixture(self) -> tuple[Path, Path]:
        root = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, root, ignore_errors=True)
        project = root / "firmware" / "brookesia"
        (project / "build" / "bootloader").mkdir(parents=True)
        (root / "config").mkdir(parents=True)
        (project / "sdkconfig.defaults").write_text("CONFIG_DEFAULT=y\n", encoding="utf-8")
        (project / "sdkconfig.defaults.rev3_x").write_text("CONFIG_ESP32P4_REV_MIN_300=y\n", encoding="utf-8")
        for variant in ("lcd-7", "lcd-8", "lcd-10-1"):
            (project / f"sdkconfig.defaults.{variant}").write_text("CONFIG_VARIANT=y\n", encoding="utf-8")
        (root / "config" / "display-variants.json").write_text(json.dumps({"display_examples": ["examples/esp-idf/demo"], "variants": [VARIANT]}), encoding="utf-8")
        return root, project

    def test_defaults_are_ordered_with_variant_last(self) -> None:
        root, project = self.fixture()
        self.assertEqual(artifact.relative_defaults(project, root, "sdkconfig.ci", VARIANT["overlay"]), ["sdkconfig.defaults", "sdkconfig.ci", "../../../config/sdkconfig/lcd-7.defaults", "../../../config/sdkconfig/rev3_x.defaults"])

    def test_lcd_validation_requires_exactly_one_expected_symbol(self) -> None:
        root, project = self.fixture()
        (project / "sdkconfig").write_text("CONFIG_BSP_LCD_TYPE_720_1280_7_INCH=y\n# CONFIG_BSP_LCD_TYPE_800_1280_8_INCH is not set\n", encoding="utf-8")
        artifact.validate_lcd_selection(project, VARIANT["kconfig"])
        (project / "sdkconfig").write_text("CONFIG_BSP_LCD_TYPE_720_1280_7_INCH=y\nCONFIG_BSP_LCD_TYPE_800_1280_8_INCH=y\n", encoding="utf-8")
        with self.assertRaises(artifact.ArtifactError):
            artifact.validate_lcd_selection(project, VARIANT["kconfig"])

    def test_flasher_schema_rejects_escaping_and_missing_files(self) -> None:
        root, project = self.fixture()
        args = project / "build" / "flasher_args.json"
        args.write_text(json.dumps({"write_flash_args": ["--flash_mode", "dio", "--flash_size", "32MB"], "flash_files": {"0x1000": "../outside.bin"}}), encoding="utf-8")
        with self.assertRaises(artifact.ArtifactError):
            artifact.load_flasher_args(project / "build")

    def test_flasher_schema_rejects_invalid_and_duplicate_normalized_offsets(self) -> None:
        root, project = self.fixture()
        args = project / "build" / "flasher_args.json"
        args.write_text(json.dumps({"write_flash_args": ["--flash_mode", "dio", "--flash_size", "32MB"], "flash_files": {"0xnothex": "demo.bin"}}), encoding="utf-8")
        with self.assertRaises(artifact.ArtifactError):
            artifact.load_flasher_args(project / "build")

    def test_flasher_write_options_reject_duplicates_and_non_32mb_size(self) -> None:
        root, project = self.fixture()
        args = project / "build" / "flasher_args.json"
        args.write_text(json.dumps({"write_flash_args": ["--flash_mode", "dio", "--flash-mode", "qio", "--flash_size", "32MB"], "flash_files": {"0x1000": "bootloader/bootloader.bin"}}), encoding="utf-8")
        with self.assertRaisesRegex(artifact.ArtifactError, "repeats write option"):
            artifact.load_flasher_args(project / "build")
        args.write_text(json.dumps({"write_flash_args": ["--flash-size", "16MB"], "flash_files": {"0x1000": "bootloader/bootloader.bin"}}), encoding="utf-8")
        with self.assertRaisesRegex(artifact.ArtifactError, "unsafe value"):
            artifact.load_flasher_args(project / "build")
        args.write_text(json.dumps({"write_flash_args": ["--flash_size", "32MB"], "flash_files": {"0x1000": "bootloader/bootloader.bin"}}), encoding="utf-8")
        self.assertEqual(artifact.load_flasher_args(project / "build")[0], ["--flash_size", "32MB"])
        args.write_text(json.dumps({"write_flash_args": ["--flash_mode", "dio", "--flash_size", "32MB"], "flash_files": {"0x1000": "demo.bin", "0x01000": "bootloader/bootloader.bin"}}), encoding="utf-8")
        with self.assertRaises(artifact.ArtifactError):
            artifact.load_flasher_args(project / "build")

    def test_fail_closed_target_and_safe_flash_bounds(self) -> None:
        root, project = self.fixture()
        sdkconfig = project / "sdkconfig"
        sdkconfig.write_text("CONFIG_ESP32P4_REV_MIN_100=y\nCONFIG_ESP32P4_SELECTS_REV_LESS_V3=y\n", encoding="utf-8")
        with self.assertRaisesRegex(artifact.ArtifactError, "target"):
            artifact.validate_build_config(sdkconfig, None, "rev1_3")
        args = project / "build" / "flasher_args.json"
        args.write_text(json.dumps({"write_flash_args": ["--flash_mode", "dio"], "flash_files": {"0x1000": "demo.bin"}}), encoding="utf-8")
        with self.assertRaisesRegex(artifact.ArtifactError, "flash_size"):
            artifact.load_flasher_args(project / "build")
        args.write_text(json.dumps({"write_flash_args": ["--erase-all", "--flash_size", "32MB"], "flash_files": {"0x1000": "demo.bin"}}), encoding="utf-8")
        with self.assertRaisesRegex(artifact.ArtifactError, "unsafe write option"):
            artifact.load_flasher_args(project / "build")
        (project / "build" / "demo.bin").write_bytes(b"")
        args.write_text(json.dumps({"write_flash_args": ["--flash_size", "32MB"], "flash_files": {"0x1000": "demo.bin"}}), encoding="utf-8")
        with self.assertRaisesRegex(artifact.ArtifactError, "empty"):
            artifact.load_flasher_args(project / "build")
        args.write_text(json.dumps({"write_flash_args": [], "flash_files": {}}), encoding="utf-8")
        with self.assertRaises(artifact.ArtifactError):
            artifact.load_flasher_args(project / "build")

    def test_packaging_writes_stable_relative_archive_contents(self) -> None:
        root, project = self.fixture()

        def fake_run(command: list[str], cwd: Path) -> None:
            self.assertEqual(command[:5], ["esptool.py", "--chip", "esp32p4", "merge_bin", "-o"])
            (cwd / "bin" / "merged-flash.bin").write_bytes(b"merged")

        with mock.patch.object(artifact, "run", side_effect=fake_run):
            archive = artifact.package_artifact(root, project, "v6.0.2", VARIANT, "0123456789abcdef")
        self.assertTrue(archive.is_file())
        with zipfile.ZipFile(archive) as output:
            names = output.namelist()
            self.assertIn("manifest.json", names)
            self.assertIn("SHA256SUMS", names)
            self.assertIn("flash.sh", names)
            self.assertIn("flash.bat", names)
            manifest = json.loads(output.read("manifest.json"))
            checksums = {path: digest for digest, path in (line.split("  ", 1) for line in output.read("SHA256SUMS").decode("utf-8").splitlines())}
            for name in names:
                if name != "SHA256SUMS":
                    self.assertIn(name, checksums)
                    self.assertEqual(checksums[name], artifact.sha256(Path(output.extract(name, root / "extracted"))))
        self.assertEqual(manifest["artifact_kind"], "source-built-example")
        self.assertEqual(manifest["schema_version"], 2)
        self.assertEqual(manifest["flash_size_bytes"], 32 * 1024 * 1024)
        self.assertFalse(manifest["factory_firmware"])
        self.assertEqual(manifest["variant"], "lcd-7")
        self.assertTrue(all(not Path(name).is_absolute() for name in names))
        self.assertNotIn(str(root), json.dumps(manifest))

    def test_main_builds_with_subprocess_list_and_package_guard(self) -> None:
        root, project = self.fixture()
        commands: list[list[str]] = []

        def fake_run(command: list[str], cwd: Path) -> None:
            commands.append(command)
            if command[0] == "idf.py":
                (project / "sdkconfig").write_text('CONFIG_IDF_TARGET="esp32p4"\nCONFIG_BSP_LCD_TYPE_720_1280_7_INCH=y\nCONFIG_ESP32P4_REV_MIN_300=y\n', encoding="utf-8")

        with mock.patch.object(artifact.Path, "cwd", return_value=project), mock.patch.object(artifact, "run", side_effect=fake_run), mock.patch.object(artifact, "verified_git_sha", return_value="head-sha"):
            self.assertEqual(artifact.main(["--repo-root", str(root), "--idf-version", "v5.5.5", "--display-variant", "lcd-7", "--variant-defaults", VARIANT["overlay"], "--sdkconfig-ci", "sdkconfig.ci", "--package-artifact", "false", "--expected-checkout-sha", "head-sha"]), 0)
        self.assertEqual(commands[0][0], "idf.py")
        self.assertIn("sdkconfig.defaults;sdkconfig.ci;../../../config/sdkconfig/lcd-7.defaults;../../../config/sdkconfig/rev3_x.defaults", commands[0][4])
        with mock.patch.object(artifact.Path, "cwd", return_value=project):
            with self.assertRaises(artifact.ArtifactError):
                artifact.main(["--repo-root", str(root), "--idf-version", "v6.0.2", "--display-variant", "shared", "--package-artifact", "true", "--expected-checkout-sha", "head-sha"])

    def test_maintained_firmware_uses_exact_project_defaults_and_rejects_impostors(self) -> None:
        root, project = self.maintained_fixture()
        commands: list[list[str]] = []

        def fake_run(command: list[str], cwd: Path) -> None:
            commands.append(command)
            if command[0] == "idf.py":
                (project / "sdkconfig.ci-lcd-7").write_text('CONFIG_IDF_TARGET="esp32p4"\nCONFIG_BSP_LCD_TYPE_720_1280_7_INCH=y\nCONFIG_ESP32P4_REV_MIN_300=y\n', encoding="utf-8")

        args = ["--repo-root", str(root), "--idf-version", "v5.5.5", "--display-variant", "lcd-7", "--revision-profile", "rev3_x", "--build-dir", "build/ci-lcd-7", "--sdkconfig", "sdkconfig.ci-lcd-7", "--package-artifact", "false", "--artifact-kind", "maintained-product-firmware", "--expected-checkout-sha", "head-sha"]
        with mock.patch.object(artifact.Path, "cwd", return_value=project), mock.patch.object(artifact, "run", side_effect=fake_run), mock.patch.object(artifact, "verified_git_sha", return_value="head-sha"):
            self.assertEqual(artifact.main(args), 0)
        self.assertIn("sdkconfig.defaults;sdkconfig.defaults.rev3_x;sdkconfig.defaults.lcd-7", commands[0][4])

        other_root, other_project = self.fixture()
        with mock.patch.object(artifact.Path, "cwd", return_value=other_project):
            with self.assertRaisesRegex(artifact.ArtifactError, "firmware/brookesia"):
                artifact.main(args[:2] + ["--idf-version", "v5.5.5", "--display-variant", "lcd-7", "--package-artifact", "false", "--artifact-kind", "maintained-product-firmware", "--expected-checkout-sha", "head-sha"])
        with mock.patch.object(artifact.Path, "cwd", return_value=project):
            with self.assertRaisesRegex(artifact.ArtifactError, "ESP-IDF v5.5.5"):
                artifact.main(args[:2] + ["--idf-version", "v6.0.2", "--display-variant", "lcd-7", "--package-artifact", "false", "--artifact-kind", "maintained-product-firmware", "--expected-checkout-sha", "head-sha"])
            with self.assertRaisesRegex(artifact.ArtifactError, "rev3_x"):
                artifact.main(args[:2] + ["--idf-version", "v5.5.5", "--display-variant", "lcd-7", "--revision-profile", "rev1_3", "--package-artifact", "false", "--artifact-kind", "maintained-product-firmware", "--expected-checkout-sha", "head-sha"])

    def test_git_head_is_authoritative_and_mismatch_fails(self) -> None:
        root, _ = self.fixture()
        result = mock.Mock(stdout="actual-head\n")
        with mock.patch.dict("os.environ", {"GITHUB_SHA": "merge-sha"}), mock.patch.object(artifact.subprocess, "run", return_value=result) as run:
            self.assertEqual(artifact.verified_git_sha(root, "actual-head"), "actual-head")
        self.assertEqual(run.call_args.args[0][:3], ["git", "-C", str(root)])
        with mock.patch.object(artifact.subprocess, "run", return_value=result):
            with self.assertRaises(artifact.ArtifactError):
                artifact.verified_git_sha(root, "different-head")


if __name__ == "__main__":
    unittest.main(verbosity=2)
