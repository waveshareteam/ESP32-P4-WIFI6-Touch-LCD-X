#!/usr/bin/env python3
"""Discover first-party Arduino sketches and route the Arduino CI matrix."""

from __future__ import annotations

import argparse
import fnmatch
import json
import os
import subprocess
import sys
from pathlib import Path


CANONICAL_ROOT = Path("examples/arduino/examples")
DISPLAY_VARIANTS = (
    ("lcd-7", "7"),
    ("lcd-8", "8"),
    ("lcd-10-1", "101"),
)
GLOBAL_PATTERNS = (
    ".github/workflows/arduino-examples.yml",
    ".github/scripts/discover_arduino_examples.py",
    ".github/scripts/test_discover_arduino_examples.py",
    ".github/scripts/test_repository_policy.py",
    ".github/ci-routing-config.json",
    ".gitignore",
)
SHARED_PATTERNS = ("examples/arduino/libraries/**",)
ESP_IDF_ONLY_PATTERNS = (
    "examples/esp-idf/**",
    ".github/workflows/esp-idf-examples.yml",
    ".github/workflows/maintained-firmware.yml",
    ".github/scripts/discover_esp_idf_examples.py",
    ".github/scripts/test_discover_esp_idf_examples.py",
    ".github/scripts/build_esp_idf_artifact.py",
    ".github/scripts/test_build_esp_idf_artifact.py",
)
LIGHTWEIGHT_ONLY_PATTERNS = (
    ".gitattributes",
    ".github/markdown-audit-config.json",
    ".github/scripts/audit_markdown.py",
    ".github/scripts/check_markdown_links.py",
    ".github/scripts/test_audit_markdown.py",
    ".github/scripts/test_check_markdown_links.py",
    ".github/workflows/docs.yml",
    "Flash-CI-Firmware.cmd",
    "scripts/Flash-CI-Firmware.ps1",
)
DOCS_SUFFIXES = frozenset({".md"})
DOCS_FILENAMES = frozenset(
    {"changelog", "contributing", "copying", "license", "license.txt", "notice", "notice.txt", "readme", "security", "support"}
)
FIRMWARE_PREFIX = "firmware/"


class DiffScopeError(RuntimeError):
    """Raised when Git cannot provide a complete changed-file scope."""


def matches_any(path: str, patterns: tuple[str, ...]) -> bool:
    return any(fnmatch.fnmatchcase(path, pattern) for pattern in patterns)


def is_docs_only_path(path: str) -> bool:
    normalized = path.replace("\\", "/").strip("/")
    return Path(normalized).suffix.lower() in DOCS_SUFFIXES or Path(normalized).name.lower() in DOCS_FILENAMES


def is_ignored_path(path: str) -> bool:
    return matches_any(path, LIGHTWEIGHT_ONLY_PATTERNS)


def is_esp_idf_only_path(path: str) -> bool:
    return matches_any(path, ESP_IDF_ONLY_PATTERNS)


def is_firmware_path(path: str) -> bool:
    return path.lower().startswith(FIRMWARE_PREFIX)


def list_examples() -> list[str]:
    if not CANONICAL_ROOT.is_dir():
        return []
    examples: list[str] = []
    for sketch in CANONICAL_ROOT.rglob("*.ino"):
        if sketch.name != f"{sketch.parent.name}.ino":
            continue
        examples.append(sketch.parent.as_posix())
    return sorted(examples)


def normalize_example(value: str, known_examples: set[str]) -> str:
    value = value.strip().replace("\\", "/").strip("/")
    if not value or value == "all":
        return value
    if value in known_examples:
        return value
    matches = [example for example in known_examples if Path(example).name == value]
    return matches[0] if len(matches) == 1 else value


def discover_from_paths(paths: list[str], known_examples: set[str]) -> list[str]:
    """Select affected sketches. Unknown complete paths remain conservative."""

    selected: set[str] = set()
    for raw_path in paths:
        path = raw_path.replace("\\", "/").strip("/")
        if is_docs_only_path(path) or is_ignored_path(path) or is_firmware_path(path) or is_esp_idf_only_path(path):
            continue
        if matches_any(path, GLOBAL_PATTERNS) or matches_any(path, SHARED_PATTERNS):
            selected.update(known_examples)
            continue
        for example in known_examples:
            if path == example or path.startswith(example + "/"):
                selected.add(example)
                break
        else:
            # A source/configuration file under the Arduino surface is shared unless
            # it belongs to one of the individually-addressable sketches above.
            if path == CANONICAL_ROOT.as_posix() or path.startswith("examples/arduino/"):
                selected.update(known_examples)
            else:
                selected.update(known_examples)
    return sorted(selected)


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


def run_git(args: list[str]) -> list[str]:
    result = subprocess.run(["git", *args], check=True, text=True, stdout=subprocess.PIPE)
    return [field for field in result.stdout.split("\0") if field]


def changed_paths_from_git(base_ref: str | None, head_ref: str) -> list[str]:
    args = (["-c", "core.quotepath=false", "diff", "--name-status", "--find-renames", "-z", f"{base_ref}...{head_ref}"] if base_ref else ["-c", "core.quotepath=false", "diff-tree", "--root", "--no-commit-id", "--name-status", "-z", "-r", head_ref])
    return parse_name_status(run_git(args))


def build_matrix(selected: list[str], variant: str = "all") -> dict[str, list[dict[str, str]]]:
    choices = {slug: value for slug, value in DISPLAY_VARIANTS}
    if variant not in {"all", *choices}:
        raise ValueError(f"Unknown display variant: {variant}")
    variants = DISPLAY_VARIANTS if variant == "all" else ((variant, choices[variant]),)
    return {"include": [
        {"path": example, "name": Path(example).name, "display_variant": slug, "display_value": value}
        for example in selected for slug, value in variants
    ]}


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
    parser.add_argument("--variant", default="all", choices=("all", "lcd-7", "lcd-8", "lcd-10-1"))
    args = parser.parse_args(argv)
    known_examples = set(list_examples())
    if not known_examples:
        print("No first-party Arduino sketches were discovered.", file=sys.stderr)
        return 2
    requested = normalize_example(args.example, known_examples)
    changed_paths: list[str] = []
    if requested == "all":
        selected = sorted(known_examples)
    elif requested:
        if requested not in known_examples:
            print(f"Unknown Arduino sketch: {args.example}", file=sys.stderr)
            return 1
        selected = [requested]
    else:
        try:
            changed_paths = changed_paths_from_git(args.base_ref, args.head_ref)
        except (DiffScopeError, OSError, subprocess.CalledProcessError) as error:
            print(f"Unable to determine a complete changed-file scope: {error}", file=sys.stderr)
            return 2
        selected = discover_from_paths(changed_paths, known_examples)
    matrix = build_matrix(selected, args.variant)
    matrix_json = json.dumps(matrix, separators=(",", ":"))
    github_output("matrix", matrix_json)
    github_output("has_examples", "true" if matrix["include"] else "false")
    github_output("examples", ",".join(selected))
    github_output("docs_only", "true" if changed_paths and all(is_docs_only_path(path) for path in changed_paths) else "false")
    print(matrix_json)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
