#!/usr/bin/env python3
"""Discover and expand first-party ESP-IDF examples for CI."""

from __future__ import annotations

import argparse
import fnmatch
import json
import os
import re
import subprocess
import sys
from pathlib import Path
from typing import Any


CANONICAL_ROOT = Path("examples/esp-idf")
REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
MANIFEST_PATH = REPOSITORY_ROOT / "config/display-variants.json"
DEFAULT_IDF_VERSIONS = ("v5.5.5", "v6.0.2")
SDKCONFIG_CI_BY_EXAMPLE = {
    "examples/esp-idf/04_wifistation": "sdkconfig.ci",
    "examples/esp-idf/05_sdmmc": "sdkconfig.ci",
}
GLOBAL_EXAMPLE_PATTERNS = (
    ".github/workflows/esp-idf-examples.yml",
    ".github/scripts/discover_esp_idf_examples.py",
    ".github/scripts/test_discover_esp_idf_examples.py",
    ".github/scripts/build_esp_idf_artifact.py",
    ".github/scripts/test_build_esp_idf_artifact.py",
    ".github/scripts/test_repository_policy.py",
    ".github/workflows/maintained-firmware.yml",
    "config/display-variants.json",
    "config/sdkconfig/rev1_3.defaults",
    "config/sdkconfig/rev3_x.defaults",
    "config/*",
    "config/**/*",
)
SHARED_CI_SAFETY_INPUTS = frozenset(
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
DOCS_ONLY_SUFFIXES = frozenset({".md"})
DOCS_MEDIA_SUFFIXES = frozenset({".gif", ".jpeg", ".jpg", ".pdf", ".png", ".svg", ".webp"})
DOCS_ONLY_FILENAMES = frozenset(
    {"changelog", "contributing", "copying", "license", "license.txt", "notice", "notice.txt", "readme", "security", "support"}
)
LIGHTWEIGHT_ONLY_PATHS = frozenset(
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
ARDUINO_ONLY_PATTERNS = (
    "examples/arduino/**",
    ".github/workflows/arduino-examples.yml",
    ".github/scripts/discover_arduino_examples.py",
    ".github/scripts/test_discover_arduino_examples.py",
)
FIRMWARE_SOURCE_SUFFIXES = frozenset({".c", ".cc", ".cpp", ".h", ".hpp", ".py", ".s"})
FIRMWARE_CONFIG_SUFFIXES = frozenset({".cmake", ".csv", ".json", ".toml", ".yaml", ".yml"})
FIRMWARE_CONFIG_FILENAMES = frozenset({"cmakelists.txt", "kconfig", "kconfig.projbuild", "partitions.csv", "sdkconfig"})
FIRMWARE_ARCHIVE_SUFFIXES = frozenset({".7z", ".gz", ".rar", ".tar", ".tgz", ".xz", ".zip"})
MAINTAINED_FIRMWARE_ROOT = "firmware/brookesia"


class DiffScopeError(RuntimeError):
    """Raised when CI cannot establish a complete changed-file scope."""


class ManifestError(RuntimeError):
    """Raised when the display-variant contract is malformed."""


def _require_string(value: Any, field: str) -> str:
    if not isinstance(value, str) or not value:
        raise ManifestError(f"display manifest field {field!r} must be a non-empty string")
    return value


def load_display_manifest(path: Path = MANIFEST_PATH) -> dict[str, Any]:
    """Load the small CI-only display contract and reject ambiguous input."""

    try:
        raw = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise ManifestError(f"unable to load display manifest {path}: {error}") from error
    if not isinstance(raw, dict) or set(raw) != {"display_examples", "variants"}:
        raise ManifestError("display manifest must contain only display_examples and variants")
    display_examples = raw["display_examples"]
    variants = raw["variants"]
    if not isinstance(display_examples, list) or not display_examples or not all(isinstance(item, str) and item for item in display_examples):
        raise ManifestError("display_examples must be a non-empty list of paths")
    if len(set(display_examples)) != len(display_examples):
        raise ManifestError("display_examples contains duplicates")
    if not isinstance(variants, list) or len(variants) != 3:
        raise ManifestError("display manifest must define exactly three variants")
    required_fields = {"slug", "product", "label", "overlay", "kconfig", "resolution", "panel"}
    normalized_variants: list[dict[str, str]] = []
    for variant in variants:
        if not isinstance(variant, dict) or set(variant) != required_fields:
            raise ManifestError(f"each variant must contain exactly {sorted(required_fields)}")
        normalized_variants.append({field: _require_string(variant[field], field) for field in required_fields})
    slugs = [variant["slug"] for variant in normalized_variants]
    overlays = [variant["overlay"] for variant in normalized_variants]
    kconfigs = [variant["kconfig"] for variant in normalized_variants]
    if len(set(slugs)) != len(slugs) or len(set(overlays)) != len(overlays) or len(set(kconfigs)) != len(kconfigs):
        raise ManifestError("variant slugs, overlays, and Kconfig symbols must each be unique")
    if set(slugs) != {"lcd-7", "lcd-8", "lcd-10-1"}:
        raise ManifestError("display manifest must define lcd-7, lcd-8, and lcd-10-1")
    return {"display_examples": tuple(display_examples), "variants": tuple(normalized_variants)}


def validate_manifest_against_repository(manifest: dict[str, Any], known_examples: set[str]) -> None:
    missing_examples = sorted(set(manifest["display_examples"]) - known_examples)
    if missing_examples:
        raise ManifestError(f"display manifest references unknown examples: {', '.join(missing_examples)}")
    for variant in manifest["variants"]:
        if not (REPOSITORY_ROOT / variant["overlay"]).is_file():
            raise ManifestError(f"display variant overlay is missing: {variant['overlay']}")


def run_git(args: list[str]) -> list[str]:
    result = subprocess.run(["git", *args], check=True, text=True, stdout=subprocess.PIPE)
    return [field for field in result.stdout.split("\0") if field]


def is_docs_only_path(path: str) -> bool:
    lowered = path.lower()
    suffix = Path(path).suffix.lower()
    name = Path(path).name.lower()
    if suffix in DOCS_ONLY_SUFFIXES or name in DOCS_ONLY_FILENAMES:
        return True
    if lowered.startswith(("assets/", "docs/", "schematic/")) and suffix in DOCS_MEDIA_SUFFIXES:
        return True
    if "/docs/" in lowered and suffix in DOCS_MEDIA_SUFFIXES:
        return True
    return lowered.startswith(".github/") and ("pull_request_template" in lowered or lowered.startswith(".github/issue_template/"))


def is_lightweight_only_path(path: str) -> bool:
    return path.lower() in LIGHTWEIGHT_ONLY_PATHS


def is_arduino_only_path(path: str) -> bool:
    return any(fnmatch.fnmatchcase(path, pattern) for pattern in ARDUINO_ONLY_PATTERNS)


def is_firmware_path(path: str) -> bool:
    return path.lower().startswith("firmware/")


def classify_firmware_path(path: str) -> str | None:
    if not is_firmware_path(path):
        return None
    lowered = path.lower()
    name, suffix = Path(lowered).name, Path(lowered).suffix
    if suffix == ".md" or name in DOCS_ONLY_FILENAMES:
        return "markdown"
    if suffix == ".bin":
        return "binary"
    if suffix in FIRMWARE_ARCHIVE_SUFFIXES:
        return "archive"
    if suffix in FIRMWARE_SOURCE_SUFFIXES:
        return "source"
    if suffix in FIRMWARE_CONFIG_SUFFIXES or name in FIRMWARE_CONFIG_FILENAMES or name.startswith("sdkconfig."):
        return "config"
    return "other"


def is_maintained_firmware_path(path: str) -> bool:
    normalized = path.replace("\\", "/").strip("/").lower()
    return normalized == MAINTAINED_FIRMWARE_ROOT or normalized.startswith(MAINTAINED_FIRMWARE_ROOT + "/")


def is_firmware_documentation_path(path: str) -> bool:
    normalized = path.replace("\\", "/").strip("/").lower()
    return is_docs_only_path(path) or f"{MAINTAINED_FIRMWARE_ROOT}/docs/" in normalized


def firmware_build_impact(paths: list[str]) -> bool:
    """Route only maintained firmware source/config/resources to the product build."""
    for path in paths:
        lowered = path.lower()
        if is_firmware_documentation_path(path) or is_lightweight_only_path(path) or is_arduino_only_path(path):
            continue
        if is_maintained_firmware_path(path):
            if classify_firmware_path(path) in {"source", "config", "other"}:
                return True
            continue
        if is_firmware_path(path):
            continue
        if lowered in SHARED_CI_SAFETY_INPUTS:
            return True
        if lowered.startswith(("config/", ".github/workflows/")):
            return True
    return False


def is_project(path: Path) -> bool:
    return (path / "CMakeLists.txt").is_file() and (path / "main").is_dir()


def discover_roots() -> list[Path]:
    return [CANONICAL_ROOT] if CANONICAL_ROOT.is_dir() else []


def list_examples() -> list[str]:
    return sorted(path.as_posix() for root in discover_roots() for path in root.iterdir() if path.is_dir() and is_project(path))


def normalize_example(value: str, known_examples: set[str]) -> str:
    value = value.strip().replace("\\", "/").strip("/")
    if not value or value == "all":
        return value
    normalized = Path(value).as_posix()
    if normalized in known_examples:
        return normalized
    matches = [example for example in known_examples if Path(example).name == value]
    return matches[0] if len(matches) == 1 else normalized


def _overlay_variant(path: str, manifest: dict[str, Any]) -> str | None:
    for variant in manifest["variants"]:
        if path == variant["overlay"]:
            return variant["slug"]
    return None


def discover_selection_from_paths(paths: list[str], known_examples: set[str], manifest: dict[str, Any]) -> tuple[list[str], str]:
    """Return selected projects plus all/a single display overlay routing intent."""

    selected: set[str] = set()
    display_examples = set(manifest["display_examples"])
    overlay_slugs: set[str] = set()
    force_all_variants = False
    for changed_path in paths:
        changed_path = changed_path.replace("\\", "/").strip("/")
        if is_docs_only_path(changed_path) or is_lightweight_only_path(changed_path) or is_firmware_path(changed_path) or is_arduino_only_path(changed_path):
            continue
        overlay_slug = _overlay_variant(changed_path, manifest)
        if overlay_slug:
            selected.update(display_examples)
            overlay_slugs.add(overlay_slug)
            continue
        if any(fnmatch.fnmatch(changed_path, pattern) for pattern in GLOBAL_EXAMPLE_PATTERNS):
            selected.update(known_examples)
            force_all_variants = True
            continue
        for example in known_examples:
            if changed_path == example or changed_path.startswith(example + "/"):
                selected.add(example)
                force_all_variants = True
                break
        else:
            if any(changed_path == root.as_posix() or changed_path.startswith(root.as_posix() + "/") for root in discover_roots()):
                selected.update(known_examples)
            else:
                selected.update(known_examples)
            force_all_variants = True
    return sorted(selected), next(iter(overlay_slugs)) if not force_all_variants and len(overlay_slugs) == 1 else "all"


def discover_from_paths(paths: list[str], known_examples: set[str]) -> list[str]:
    """Compatibility wrapper used by routing tests that only need project paths."""

    return discover_selection_from_paths(paths, known_examples, load_display_manifest())[0]


def parse_name_status(lines: list[str]) -> list[str]:
    paths: list[str] = []
    index = 0
    while index < len(lines):
        status = lines[index]
        index += 1
        if status.startswith(("R", "C")):
            if index + 1 >= len(lines):
                raise DiffScopeError(f"Malformed rename/copy record: {status}")
            paths.extend(lines[index:index + 2])
            index += 2
        else:
            if index >= len(lines):
                raise DiffScopeError(f"Malformed changed-file record: {status}")
            paths.append(lines[index])
            index += 1
    normalized = [path.replace("\\", "/").strip("/") for path in paths if path.strip("/")]
    if not normalized:
        raise DiffScopeError("Git returned an empty changed-file scope")
    return normalized


def changed_paths_from_git(base_ref: str | None, head_ref: str) -> list[str]:
    diff_args = (["-c", "core.quotepath=false", "diff", "--name-status", "--find-renames", "-z", f"{base_ref}...{head_ref}"] if base_ref else ["-c", "core.quotepath=false", "diff-tree", "--root", "--no-commit-id", "--name-status", "-z", "-r", head_ref])
    return parse_name_status(run_git(diff_args))


def build_matrix(selected: list[str], manifest: dict[str, Any] | None = None, variant: str = "all", artifact_sha: str = "") -> dict[str, list[dict[str, Any]]]:
    manifest = manifest or load_display_manifest()
    display_examples = set(manifest["display_examples"])
    variants = tuple(manifest["variants"])
    valid_variants = {"all", "shared", *(entry["slug"] for entry in variants)}
    if variant not in valid_variants:
        raise ValueError(f"Unknown display variant: {variant}")
    include: list[dict[str, Any]] = []
    for example in selected:
        if example in display_examples:
            applicable = () if variant == "shared" else (variants if variant == "all" else tuple(entry for entry in variants if entry["slug"] == variant))
            for display_variant in applicable:
                for idf_version in DEFAULT_IDF_VERSIONS:
                    row = {"example": example, "example_name": Path(example).name, "idf_version": idf_version, "display_variant": display_variant["slug"], "product": display_variant["product"], "variant_label": display_variant["label"], "variant_defaults": display_variant["overlay"], "revision_profile": "rev3_x", "sdkconfig_ci": SDKCONFIG_CI_BY_EXAMPLE.get(example, ""), "package_artifact": "true" if idf_version == "v6.0.2" else "false"}
                    if row["package_artifact"] == "true" and artifact_sha:
                        row["artifact_name"] = f"{display_variant['product']}-{Path(example).name}-rev3_x-{idf_version.lstrip('v')}-{artifact_sha[:12]}"
                    include.append(row)
        elif variant in {"all", "shared"}:
            for idf_version in DEFAULT_IDF_VERSIONS:
                include.append({"example": example, "idf_version": idf_version, "display_variant": "shared", "variant_label": "Shared defaults", "variant_defaults": "", "revision_profile": "rev3_x", "sdkconfig_ci": SDKCONFIG_CI_BY_EXAMPLE.get(example, ""), "package_artifact": "false"})
    return {"include": include}


def github_output(name: str, value: str) -> None:
    output_path = os.environ.get("GITHUB_OUTPUT")
    if output_path:
        with open(output_path, "a", encoding="utf-8") as output:
            output.write(f"{name}={value}\n")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--base-ref")
    parser.add_argument("--head-ref", default="HEAD")
    parser.add_argument("--example", default="")
    parser.add_argument("--variant", default="all", choices=("all", "shared", "lcd-7", "lcd-8", "lcd-10-1"))
    parser.add_argument("--artifact-sha", default="")
    args = parser.parse_args(argv)
    known_examples = set(list_examples())
    if not known_examples:
        print("No first-party ESP-IDF examples were discovered.", file=sys.stderr)
        return 2
    try:
        manifest = load_display_manifest()
        validate_manifest_against_repository(manifest, known_examples)
    except ManifestError as error:
        print(f"Invalid display variant manifest: {error}", file=sys.stderr)
        return 2
    requested_example = normalize_example(args.example, known_examples)
    changed_paths: list[str] = []
    routing_variant = args.variant
    if requested_example == "all":
        selected = sorted(known_examples)
    elif requested_example:
        if requested_example not in known_examples:
            print(f"Unknown ESP-IDF example: {args.example}", file=sys.stderr)
            return 1
        selected = [requested_example]
    else:
        try:
            changed_paths = changed_paths_from_git(args.base_ref, args.head_ref)
        except (DiffScopeError, OSError, subprocess.CalledProcessError) as error:
            print(f"Unable to determine a complete changed-file scope: {error}", file=sys.stderr)
            return 2
        selected, discovered_variant = discover_selection_from_paths(changed_paths, known_examples, manifest)
        if args.variant == "all":
            routing_variant = discovered_variant
    if args.artifact_sha and not re.fullmatch(r"[0-9a-fA-F]{40}", args.artifact_sha):
        print("Artifact SHA must be a complete 40-character commit SHA.", file=sys.stderr)
        return 2
    matrix = build_matrix(selected, manifest, routing_variant, args.artifact_sha)
    matrix_json = json.dumps(matrix, separators=(",", ":"))
    has_examples = "true" if matrix["include"] else "false"
    docs_only = "true" if changed_paths and all(is_docs_only_path(path) for path in changed_paths) else "false"
    firmware_kinds = sorted({kind for path in changed_paths if (kind := classify_firmware_path(path)) is not None})
    github_output("matrix", matrix_json)
    github_output("has_examples", has_examples)
    github_output("examples", ",".join(selected))
    github_output("docs_only", docs_only)
    github_output("firmware_touched", "true" if firmware_kinds else "false")
    github_output("firmware_kinds", ",".join(firmware_kinds))
    github_output("release_review", "true" if {"archive", "binary"}.intersection(firmware_kinds) else "false")
    github_output("firmware_build_impact", "true" if firmware_build_impact(changed_paths) else "false")
    print(matrix_json)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
