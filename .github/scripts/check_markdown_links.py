#!/usr/bin/env python3
"""Validate repository-local links in Markdown documentation."""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path
from urllib.parse import unquote, urlsplit


MARKDOWN_LINK = re.compile(r"!?\[[^\]]*]\(([^)]+)\)")
HTML_LINK = re.compile(r"""(?:href|src)\s*=\s*["']([^"']+)["']""", re.IGNORECASE)
EXTERNAL_SCHEMES = {"data", "http", "https", "mailto", "tel"}


def extract_targets(markdown: str) -> list[str]:
    targets = [match.group(1).strip() for match in MARKDOWN_LINK.finditer(markdown)]
    targets.extend(match.group(1).strip() for match in HTML_LINK.finditer(markdown))
    return targets


def local_path(target: str) -> str | None:
    if target.startswith("#") or target.startswith("//"):
        return None

    parsed = urlsplit(target.strip("<>"))
    if parsed.scheme.lower() in EXTERNAL_SCHEMES:
        return None
    if parsed.scheme or parsed.netloc or not parsed.path:
        return None

    return unquote(parsed.path)


def validate_file(root: Path, source: Path) -> list[str]:
    failures: list[str] = []
    markdown = source.read_text(encoding="utf-8")
    try:
        source_name = source.resolve().relative_to(root).as_posix()
    except ValueError:
        source_name = source.as_posix()

    for target in extract_targets(markdown):
        path = local_path(target)
        if path is None:
            continue

        if path.startswith("/"):
            failures.append(
                f"{source_name}: root-relative target is not a repository-local link: {target}"
            )
            continue

        candidate = (source.parent / path).resolve()
        try:
            candidate.relative_to(root)
        except ValueError:
            failures.append(f"{source_name}: target escapes repository root: {target}")
            continue

        if not candidate.exists():
            failures.append(f"{source_name}: missing local target {target}")

    return failures


def tracked_first_party_files(root: Path) -> list[Path]:
    result = subprocess.run(
        ["git", "ls-files", "-z", "--", "*.md"],
        cwd=root,
        check=True,
        stdout=subprocess.PIPE,
    )
    paths = [
        Path(raw.decode("utf-8"))
        for raw in result.stdout.split(b"\0")
        if raw
    ]
    return [
        root / path
        for path in paths
        if "components" not in path.parts
        and path.parts[:3] != ("examples", "arduino", "libraries")
    ]


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("files", nargs="*", type=Path)
    parser.add_argument(
        "--tracked-first-party",
        action="store_true",
        help="Validate every tracked Markdown file outside embedded component and library trees.",
    )
    args = parser.parse_args(argv)

    root = Path.cwd().resolve()
    failures: list[str] = []

    if args.tracked_first_party and args.files:
        parser.error("provide file paths or --tracked-first-party, not both")

    try:
        sources = tracked_first_party_files(root) if args.tracked_first_party else args.files
    except (OSError, subprocess.CalledProcessError, UnicodeDecodeError) as error:
        print(f"Unable to enumerate tracked Markdown files: {error}", file=sys.stderr)
        return 2

    if not sources:
        parser.error("no Markdown files were selected")

    for source in sources:
        if not source.is_file():
            try:
                source_name = source.resolve().relative_to(root).as_posix()
            except ValueError:
                source_name = source.as_posix()
            failures.append(f"{source_name}: Markdown file does not exist")
            continue
        failures.extend(validate_file(root, source))

    if failures:
        print("\n".join(failures), file=sys.stderr)
        return 1

    print(f"Validated local links in {len(sources)} Markdown files.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
