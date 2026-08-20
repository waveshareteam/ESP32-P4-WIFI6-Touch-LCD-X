#!/usr/bin/env python3
"""Build one ESP-IDF CI row and optionally package a source-built artifact."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
import zipfile
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


LCD_SYMBOLS = (
    "CONFIG_BSP_LCD_TYPE_720_1280_7_INCH",
    "CONFIG_BSP_LCD_TYPE_800_1280_8_INCH",
    "CONFIG_BSP_LCD_TYPE_800_1280_10_1_INCH",
)
REVISION_PROFILES = {
    "rev1_3": {
        "defaults": "config/sdkconfig/rev1_3.defaults",
        "min": "1.0",
        "max_exclusive": "3.0",
        "less_v3": "y",
        "minimum": "CONFIG_ESP32P4_REV_MIN_100",
    },
    "rev3_x": {
        "defaults": "config/sdkconfig/rev3_x.defaults",
        "min": "3.0",
        "max_exclusive": None,
        "less_v3": None,
        "minimum": "CONFIG_ESP32P4_REV_MIN_300",
    },
}
MAINTAINED_FIRMWARE_PATH = Path("firmware/brookesia")
MAINTAINED_FIRMWARE_IDF_VERSION = "v5.5.5"


class ArtifactError(RuntimeError):
    """Raised when build output cannot be safely packaged."""


def run(command: list[str], cwd: Path) -> None:
    subprocess.run(command, cwd=cwd, check=True)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def relative_defaults(project: Path, repo_root: Path, sdkconfig_ci: str, variant_defaults: str, revision_profile: str = "rev3_x") -> list[str]:
    defaults: list[str] = []
    if (project / "sdkconfig.defaults").is_file():
        defaults.append("sdkconfig.defaults")
    if sdkconfig_ci:
        candidate = project / sdkconfig_ci
        if not candidate.is_file():
            raise ArtifactError(f"sdkconfig.ci overlay is missing: {candidate}")
        defaults.append(sdkconfig_ci)
    if variant_defaults:
        overlay = repo_root / variant_defaults
        if not overlay.is_file():
            raise ArtifactError(f"display overlay is missing: {overlay}")
        defaults.append(os.path.relpath(overlay, project).replace("\\", "/"))
    profile = REVISION_PROFILES.get(revision_profile)
    if profile is None:
        raise ArtifactError(f"unknown revision profile: {revision_profile}")
    overlay = repo_root / profile["defaults"]
    if not overlay.is_file():
        raise ArtifactError(f"revision overlay is missing: {overlay}")
    defaults.append(os.path.relpath(overlay, project).replace("\\", "/"))
    return defaults


def maintained_firmware_defaults(project: Path, display_variant: str, revision_profile: str) -> list[str]:
    if revision_profile != "rev3_x":
        raise ArtifactError("maintained firmware only supports rev3_x")
    defaults = ["sdkconfig.defaults", "sdkconfig.defaults.rev3_x", f"sdkconfig.defaults.{display_variant}"]
    for name in defaults:
        if not (project / name).is_file():
            raise ArtifactError(f"maintained firmware defaults are missing: {project / name}")
    return defaults


def is_maintained_firmware_project(project: Path, repo_root: Path) -> bool:
    return project == (repo_root / MAINTAINED_FIRMWARE_PATH).resolve()


def validate_build_config(sdkconfig: Path, expected_symbol: str | None, revision_profile: str) -> None:
    if not sdkconfig.is_file():
        raise ArtifactError(f"generated sdkconfig is missing: {sdkconfig}")
    values = dict(
        line.split("=", 1)
        for line in sdkconfig.read_text(encoding="utf-8").splitlines()
        if line.startswith("CONFIG_") and "=" in line
    )
    enabled = [symbol for symbol in LCD_SYMBOLS if values.get(symbol) == "y"]
    if expected_symbol is not None and enabled != [expected_symbol]:
        raise ArtifactError(f"LCD variant mismatch: expected {expected_symbol}=y, got {enabled or 'no enabled LCD type'}")
    if values.get("CONFIG_IDF_TARGET") != '"esp32p4"':
        raise ArtifactError("generated sdkconfig target is not esp32p4")
    profile = REVISION_PROFILES.get(revision_profile)
    if profile is None:
        raise ArtifactError(f"unknown revision profile: {revision_profile}")
    if values.get(profile["minimum"]) != "y":
        raise ArtifactError(f"generated sdkconfig does not select {revision_profile}")
    less_v3 = values.get("CONFIG_ESP32P4_SELECTS_REV_LESS_V3")
    if profile["less_v3"] == "y" and less_v3 != "y":
        raise ArtifactError("generated sdkconfig does not select pre-v3 silicon")
    if profile["less_v3"] is None and less_v3 == "y":
        raise ArtifactError("generated sdkconfig incorrectly selects pre-v3 silicon")


def validate_lcd_selection(project: Path, expected_symbol: str) -> None:
    """Backward-compatible LCD-only wrapper used by older unit tests."""
    sdkconfig = project / "sdkconfig"
    if not sdkconfig.is_file():
        raise ArtifactError(f"generated sdkconfig is missing: {sdkconfig}")
    values = dict(line.split("=", 1) for line in sdkconfig.read_text(encoding="utf-8").splitlines() if line.startswith("CONFIG_BSP_LCD_TYPE_") and "=" in line)
    enabled = [symbol for symbol in LCD_SYMBOLS if values.get(symbol) == "y"]
    if enabled != [expected_symbol]:
        raise ArtifactError(f"LCD variant mismatch: expected {expected_symbol}=y, got {enabled or 'no enabled LCD type'}")


def load_variant(repo_root: Path, slug: str) -> dict[str, str]:
    if slug == "shared":
        return {}
    try:
        manifest = json.loads((repo_root / "config/display-variants.json").read_text(encoding="utf-8"))
        variants = manifest["variants"]
    except (OSError, json.JSONDecodeError, KeyError, TypeError) as error:
        raise ArtifactError(f"invalid display variant manifest: {error}") from error
    matches = [entry for entry in variants if isinstance(entry, dict) and entry.get("slug") == slug]
    if len(matches) != 1:
        raise ArtifactError(f"unknown display variant: {slug}")
    required = {"slug", "product", "label", "overlay", "kconfig", "resolution", "panel"}
    if set(matches[0]) != required or not all(isinstance(matches[0][key], str) and matches[0][key] for key in required):
        raise ArtifactError(f"invalid display variant metadata for {slug}")
    return matches[0]


def load_flasher_args(build_dir: Path) -> tuple[list[str], list[tuple[str, Path]]]:
    path = build_dir / "flasher_args.json"
    try:
        raw = json.loads(path.read_text(encoding="utf-8"))
        write_args = raw["write_flash_args"]
        flash_files = raw["flash_files"]
    except (OSError, json.JSONDecodeError, KeyError, TypeError) as error:
        raise ArtifactError(f"invalid flasher_args.json: {error}") from error
    if not isinstance(write_args, list) or not all(isinstance(arg, str) and arg for arg in write_args):
        raise ArtifactError("flasher_args.json write_flash_args must be a non-empty string list")
    option_aliases = {
        "--flash_mode": "flash_mode",
        "--flash-mode": "flash_mode",
        "--flash_freq": "flash_freq",
        "--flash-freq": "flash_freq",
        "--flash_size": "flash_size",
        "--flash-size": "flash_size",
    }
    valid_values = {
        "flash_mode": {"qio", "qout", "dio", "dout"},
        "flash_freq": {"20m", "26m", "40m", "80m"},
        "flash_size": {"32MB"},
    }
    seen_options: set[str] = set()
    index = 0
    while index < len(write_args):
        option = write_args[index]
        if not option.startswith("--"):
            raise ArtifactError(f"flasher_args.json has a non-option write argument: {option}")
        if option not in option_aliases:
            raise ArtifactError(f"flasher_args.json has an unsafe write option: {option}")
        normalized_option = option_aliases[option]
        if normalized_option in seen_options:
            raise ArtifactError(f"flasher_args.json repeats write option: {option}")
        if index + 1 >= len(write_args) or write_args[index + 1] not in valid_values[normalized_option]:
            raise ArtifactError(f"flasher_args.json has an unsafe value for {option}")
        seen_options.add(normalized_option)
        index += 2
    if "flash_size" not in seen_options:
        raise ArtifactError("flasher_args.json must explicitly set --flash_size 32MB")
    if not isinstance(flash_files, dict) or not flash_files:
        raise ArtifactError("flasher_args.json flash_files must be a non-empty object")
    entries: list[tuple[str, Path]] = []
    normalized_offsets: set[str] = set()
    for offset, file_name in flash_files.items():
        if not isinstance(offset, str) or not offset.startswith("0x") or not isinstance(file_name, str):
            raise ArtifactError("flasher_args.json flash_files contains an invalid offset or path")
        try:
            normalized_offset = f"0x{int(offset, 16):x}"
        except ValueError as error:
            raise ArtifactError(f"flasher_args.json contains a non-hex offset: {offset}") from error
        if normalized_offset in normalized_offsets:
            raise ArtifactError(f"flasher_args.json contains duplicate normalized offset: {normalized_offset}")
        normalized_offsets.add(normalized_offset)
        source = (build_dir / file_name).resolve()
        if not source.is_relative_to(build_dir.resolve()) or not source.is_file():
            raise ArtifactError(f"flasher_args.json references missing or escaping file: {file_name}")
        size = source.stat().st_size
        if size <= 0:
            raise ArtifactError(f"flasher_args.json references an empty flash file: {file_name}")
        if int(normalized_offset, 16) + size > 32 * 1024 * 1024:
            raise ArtifactError(f"flasher_args.json flash range exceeds 32MiB: {file_name}")
        entries.append((normalized_offset, source))
    entries.sort(key=lambda item: int(item[0], 16))
    for (left_offset, left), (right_offset, _right) in zip(entries, entries[1:]):
        if int(left_offset, 16) + left.stat().st_size > int(right_offset, 16):
            raise ArtifactError("flasher_args.json flash files overlap")
    return write_args, entries


def verified_git_sha(repo_root: Path, expected_checkout_sha: str) -> str:
    result = subprocess.run(["git", "-C", str(repo_root), "rev-parse", "HEAD"], check=True, text=True, stdout=subprocess.PIPE)
    actual = result.stdout.strip()
    if not actual:
        raise ArtifactError("git returned an empty checkout SHA")
    if actual != expected_checkout_sha:
        raise ArtifactError(f"checkout SHA mismatch: expected {expected_checkout_sha}, got {actual}")
    return actual


def package_artifact(repo_root: Path, project: Path, idf_version: str, variant: dict[str, str], git_sha: str, revision_profile: str = "rev3_x", build_dir: Path | None = None, artifact_kind: str = "source-built-example") -> Path:
    build_dir = (build_dir or project / "build").resolve()
    write_args, flash_files = load_flasher_args(build_dir)
    stage = build_dir / "ci-artifact"
    if stage.exists():
        shutil.rmtree(stage)
    bin_dir = stage / "bin"
    bin_dir.mkdir(parents=True)
    packed_files: list[dict[str, Any]] = []
    flash_args: list[str] = [*write_args]
    for offset, source in flash_files:
        destination = bin_dir / f"{int(offset, 16):08x}-{source.name}"
        shutil.copy2(source, destination)
        relative = destination.relative_to(stage).as_posix()
        flash_args.extend((offset, relative))
        packed_files.append({"offset": offset, "path": relative, "size": destination.stat().st_size, "sha256": sha256(destination)})
    for left, right in zip(packed_files, packed_files[1:]):
        if int(left["offset"], 16) + int(left["size"]) > int(right["offset"], 16):
            raise ArtifactError("flash files overlap")
    flash_args_path = stage / "flash_args"
    flash_args_path.write_text("\n".join(flash_args) + "\n", encoding="utf-8")
    merged = bin_dir / "merged-flash.bin"
    run(["esptool.py", "--chip", "esp32p4", "merge_bin", "-o", "bin/merged-flash.bin", "@flash_args"], stage)
    merged_size = merged.stat().st_size
    if merged_size <= 0 or merged_size > 32 * 1024 * 1024:
        raise ArtifactError("merged flash image must be non-empty and no larger than 32MiB")
    merged_image = {"path": "bin/merged-flash.bin", "size": merged_size, "sha256": sha256(merged)}
    example = project.relative_to(repo_root).as_posix()
    command = ["esptool.py", "--chip", "esp32p4", "--baud", "460800", "write_flash", "@flash_args"]
    manifest = {
        "schema_version": 2,
        "flash_size_bytes": 32 * 1024 * 1024,
        "artifact_kind": artifact_kind,
        "factory_firmware": False,
        "framework": "ESP-IDF",
        "idf_version": idf_version,
        "revision_profile": revision_profile,
        "revision_bounds": {"min": REVISION_PROFILES[revision_profile]["min"], "max_exclusive": REVISION_PROFILES[revision_profile]["max_exclusive"]},
        "target": "esp32p4",
        "example": example,
        "variant": variant["slug"],
        "product": variant["product"],
        "resolution": variant["resolution"],
        "panel": variant["panel"],
        "kconfig": variant["kconfig"],
        "git_sha": git_sha,
        "timestamp_utc": datetime.now(timezone.utc).replace(microsecond=0).isoformat(),
        "baud": 460800,
        "offsets": {entry["offset"]: entry["path"] for entry in packed_files},
        "files": packed_files,
        "merged_image": merged_image,
        "flash_command": " ".join(command),
    }
    (stage / "manifest.json").write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    (stage / "flash.sh").write_text("#!/usr/bin/env sh\nset -eu\nesptool.py --chip esp32p4 --baud 460800 write_flash @flash_args\n", encoding="utf-8")
    (stage / "flash.bat").write_text("@echo off\r\nesptool.py --chip esp32p4 --baud 460800 write_flash @flash_args\r\n", encoding="utf-8")
    checksum_files = sorted(path.relative_to(stage).as_posix() for path in stage.rglob("*") if path.is_file())
    (stage / "SHA256SUMS").write_text("".join(f"{sha256(stage / item)}  {item}\n" for item in checksum_files), encoding="utf-8")
    output_dir = repo_root / "ci-artifacts"
    output_dir.mkdir(exist_ok=True)
    archive = output_dir / f"{variant['product']}-{project.name}-{revision_profile}-{idf_version.lstrip('v')}-{git_sha[:12]}.zip"
    with zipfile.ZipFile(archive, "w", compression=zipfile.ZIP_DEFLATED) as output:
        for source in sorted(stage.rglob("*")):
            if source.is_file():
                output.write(source, source.relative_to(stage).as_posix())
    return archive


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo-root", required=True)
    parser.add_argument("--idf-version", required=True)
    parser.add_argument("--display-variant", required=True)
    parser.add_argument("--variant-defaults", default="")
    parser.add_argument("--sdkconfig-ci", default="")
    parser.add_argument("--package-artifact", choices=("true", "false"), required=True)
    parser.add_argument("--revision-profile", choices=tuple(REVISION_PROFILES), default="rev3_x")
    parser.add_argument("--build-dir", default="build")
    parser.add_argument("--sdkconfig", default="sdkconfig")
    parser.add_argument("--artifact-kind", choices=("source-built-example", "maintained-product-firmware"), default="source-built-example")
    parser.add_argument("--expected-checkout-sha", required=True)
    args = parser.parse_args(argv)
    project = Path.cwd().resolve()
    repo_root = Path(args.repo_root).resolve()
    variant = load_variant(repo_root, args.display_variant)
    maintained_project = is_maintained_firmware_project(project, repo_root)
    if args.artifact_kind == "maintained-product-firmware":
        if not maintained_project:
            raise ArtifactError("maintained-product-firmware is reserved for firmware/brookesia")
        if args.idf_version != MAINTAINED_FIRMWARE_IDF_VERSION:
            raise ArtifactError("maintained firmware requires ESP-IDF v5.5.5")
        if args.revision_profile != "rev3_x":
            raise ArtifactError("maintained firmware only supports rev3_x")
        if args.sdkconfig_ci or args.variant_defaults:
            raise ArtifactError("maintained firmware must use its project defaults")
    if args.package_artifact == "true" and not variant:
        raise ArtifactError("only display variants may package artifacts")
    actual_sha = verified_git_sha(repo_root, args.expected_checkout_sha)
    defaults = (
        maintained_firmware_defaults(project, args.display_variant, args.revision_profile)
        if maintained_project and args.artifact_kind == "maintained-product-firmware"
        else relative_defaults(project, repo_root, args.sdkconfig_ci, args.variant_defaults, args.revision_profile)
    )
    build_dir = (project / args.build_dir).resolve()
    sdkconfig = (project / args.sdkconfig).resolve()
    command = ["idf.py", "-B", str(build_dir), f"-DSDKCONFIG={sdkconfig}"]
    if defaults:
        command.append(f"-DSDKCONFIG_DEFAULTS={';'.join(defaults)}")
    command.append("build")
    run(command, project)
    if variant and not maintained_project:
        if args.variant_defaults != variant["overlay"]:
            raise ArtifactError("matrix overlay does not match display variant metadata")
    validate_build_config(sdkconfig, variant.get("kconfig"), args.revision_profile)
    if args.package_artifact == "true":
        print(package_artifact(repo_root, project, args.idf_version, variant, actual_sha, args.revision_profile, build_dir, args.artifact_kind))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (ArtifactError, subprocess.CalledProcessError) as error:
        print(f"CI artifact build failed: {error}", file=sys.stderr)
        raise SystemExit(1)
