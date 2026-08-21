#!/usr/bin/env python3
"""Tests for the repository-local Markdown link checker."""

from __future__ import annotations

import contextlib
import io
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock


sys.path.insert(0, str(Path(__file__).resolve().parent))
import check_markdown_links as links  # noqa: E402


class MarkdownLinkTests(unittest.TestCase):
    def test_extracts_markdown_images_and_html_targets(self) -> None:
        targets = links.extract_targets(
            "[guide](docs/guide.md) ![board](assets/board.png) "
            '<a href="README_ZH.md">中文</a>'
        )
        self.assertEqual(
            targets,
            ["docs/guide.md", "assets/board.png", "README_ZH.md"],
        )

    def test_ignores_external_and_same_page_targets(self) -> None:
        self.assertIsNone(links.local_path("https://example.com/docs"))
        self.assertIsNone(links.local_path("mailto:support@example.com"))
        self.assertIsNone(links.local_path("#configuration"))

    def test_validates_existing_missing_escape_and_root_relative_paths(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir).resolve()
            docs = root / "docs"
            docs.mkdir()
            (root / "README_ZH.md").write_text("# 中文\n", encoding="utf-8")
            source = docs / "guide.md"
            source.write_text(
                "[ok](../README_ZH.md) [missing](missing.md) "
                "[escape](../../outside.md) [root](/README.md)\n",
                encoding="utf-8",
            )
            failures = links.validate_file(root, source)
            self.assertEqual(len(failures), 3)
            self.assertTrue(all(failure.startswith("docs/guide.md:") for failure in failures))

    def test_tracked_file_enumeration_is_nul_safe_and_excludes_embedded_trees(self) -> None:
        completed = subprocess.CompletedProcess(
            ["git", "ls-files"],
            0,
            stdout=(
                b"README.md\0docs/Guide with space.md\0"
                b"examples/esp-idf/demo/components/upstream/README.md\0"
                b"examples/arduino/libraries/lvgl/README.md\0"
                b"docs/libraries-guide.md\0"
            ),
        )
        root = Path("repository").resolve()
        with mock.patch.object(links.subprocess, "run", return_value=completed):
            selected = links.tracked_first_party_files(root)
        self.assertEqual(
            [path.relative_to(root).as_posix() for path in selected],
            ["README.md", "docs/Guide with space.md", "docs/libraries-guide.md"],
        )

    def test_tracked_mode_invocation(self) -> None:
        old_cwd = Path.cwd()
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            source = root / "README.md"
            source.write_text("# Repository\n", encoding="utf-8")
            try:
                os.chdir(root)
                with mock.patch.object(links, "tracked_first_party_files", return_value=[source]), contextlib.redirect_stdout(io.StringIO()):
                    self.assertEqual(links.main(["--tracked-first-party"]), 0)
            finally:
                os.chdir(old_cwd)

    def test_workflow_runs_tests_and_tracked_mode(self) -> None:
        workflow = Path(".github/workflows/docs.yml").read_text(encoding="utf-8")
        self.assertIn("test_check_markdown_links.py", workflow)
        self.assertIn("check_markdown_links.py --tracked-first-party", workflow)
        self.assertIn("uses: actions/checkout@v7", workflow)
        self.assertNotIn("uses: actions/checkout@v4", workflow)


if __name__ == "__main__":
    unittest.main(verbosity=2)
