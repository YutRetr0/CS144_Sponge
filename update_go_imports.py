#!/usr/bin/env python3
"""
Utility to rewrite Go imports for the SN test packages.

Rules:
- Replace imports of "git.woa.com/sn/servergroup/apitest/test" with
  sntest "sn/test/internal/testing" (dot imports keep the dot).
- Remove imports of the formatter/matcher packages from the same module and
  ensure sntest "sn/test/internal/testing" is imported instead.
- Update usages of apitest/test/formatter/matcher aliases to sntest.
"""

import argparse
from pathlib import Path
import re
from typing import Iterable, List, Optional, Set, Tuple

TEST_PATH = "git.woa.com/sn/servergroup/apitest/test"
FORMATTER_PATH = "git.woa.com/sn/servergroup/apitest/formatter"
MATCHER_PATH = "git.woa.com/sn/servergroup/apitest/matcher"
NEW_PATH = "sn/test/internal/testing"
NEW_ALIAS = "sntest"
DEFAULT_IMPORT_INDENT = "\t"


def _default_alias(import_path: str) -> str:
    return import_path.rstrip("/").split("/")[-1]


def _parse_import_entry(line: str) -> Optional[Tuple[str, str, str, str]]:
    """Return (indent, alias, path, rest) for an import line, or None if not matched."""
    match = re.match(r'^(\s*)(?:(?P<alias>[.\w-]+)\s+)?\"(?P<path>[^\"]+)\"(?P<rest>.*)$', line)
    if not match:
        return None
    indent = match.group(1)
    alias = match.group("alias")
    path = match.group("path")
    rest = match.group("rest") or ""
    return indent, alias, path, rest


def _process_import_block(lines: List[str]) -> Tuple[List[str], Set[str], bool, bool]:
    """Process a multi-line import block starting with 'import (' and ending with ')'.

    Returns (new_lines, aliases_to_replace, changed, has_sntest).
    """
    if len(lines) < 2:
        return lines, set(), False, False

    header, footer = lines[0], lines[-1]
    body = lines[1:-1]
    new_body: List[str] = []
    aliases_to_replace: Set[str] = set()
    ensure_sntest = False
    found_sntest = False
    added_sntest = False
    changed = False
    default_indent = None

    for entry in body:
        parsed = _parse_import_entry(entry.strip("\n"))
        if not parsed:
            new_body.append(entry)
            continue

        indent, alias, path, rest = parsed
        default_indent = default_indent or indent
        effective_alias = alias or _default_alias(path)

        if path == TEST_PATH:
            if alias == ".":
                new_body.append(f'{indent}. "{NEW_PATH}"{rest}')
            else:
                ensure_sntest = True
                aliases_to_replace.add(effective_alias)
                new_body.append(f'{indent}{NEW_ALIAS} "{NEW_PATH}"{rest}')
                added_sntest = True
            changed = True
            continue

        if path in {FORMATTER_PATH, MATCHER_PATH}:
            if alias != ".":
                ensure_sntest = True
                aliases_to_replace.add(effective_alias)
            changed = True
            # drop the line
            continue

        if path == NEW_PATH and (alias == NEW_ALIAS or effective_alias == NEW_ALIAS):
            found_sntest = True

        new_body.append(entry)

    if ensure_sntest and not (found_sntest or added_sntest):
        indent = default_indent if default_indent is not None else DEFAULT_IMPORT_INDENT
        new_body.append(f'{indent}{NEW_ALIAS} "{NEW_PATH}"')
        changed = True

    has_sntest = found_sntest or added_sntest
    return [header, *new_body, footer], aliases_to_replace, changed, has_sntest


def _process_single_import(
    line: str, has_sntest_import: bool
) -> Tuple[Optional[str], Set[str], bool, bool]:
    """Process a single-line import statement."""
    parsed = _parse_import_entry(line.strip("\n").replace("import ", "", 1))
    if not parsed:
        return line, set(), False, False

    indent_match = re.match(r"^(\s*)import\s+", line)
    indent = indent_match.group(1) if indent_match else ""
    alias, path, rest = parsed[1:]
    aliases_to_replace: Set[str] = set()
    has_sntest_here = False

    if path == NEW_PATH and (alias == NEW_ALIAS or _default_alias(path) == NEW_ALIAS):
        return line, aliases_to_replace, False, True

    if path == TEST_PATH:
        if alias == ".":
            return f'{indent}import . "{NEW_PATH}"{rest}', aliases_to_replace, True, False
        aliases_to_replace.add(alias or _default_alias(path))
        return (
            f'{indent}import {NEW_ALIAS} "{NEW_PATH}"{rest}',
            aliases_to_replace,
            True,
            True,
        )

    if path in {FORMATTER_PATH, MATCHER_PATH}:
        if alias != ".":
            aliases_to_replace.add(alias or _default_alias(path))
        if has_sntest_import:
            return None, aliases_to_replace, True, False
        return (
            f'{indent}import {NEW_ALIAS} "{NEW_PATH}"{rest}',
            aliases_to_replace,
            True,
            True,
        )

    return line, aliases_to_replace, False, has_sntest_here


def _replace_usages(content: str, aliases: Iterable[str]) -> Tuple[str, bool]:
    changed = False
    for alias in aliases:
        if alias in {".", NEW_ALIAS}:
            continue
        pattern = re.compile(rf"\b{re.escape(alias)}\.")
        new_content, replacements = pattern.subn(f"{NEW_ALIAS}.", content)
        if replacements:
            changed = True
            content = new_content
    return content, changed


def process_file(path: Path) -> bool:
    original = path.read_text()
    lines = original.splitlines()
    new_lines: List[str] = []
    idx = 0
    aliases_to_replace: Set[str] = set()
    has_sntest_import = False

    while idx < len(lines):
        line = lines[idx]
        stripped = line.lstrip()
        if stripped.startswith("import ("):
            block: List[str] = [line]
            idx += 1
            found_closing = False
            while idx < len(lines):
                block.append(lines[idx])
                if lines[idx].strip().endswith(")"):
                    found_closing = True
                    break
                idx += 1
            if not found_closing:
                new_lines.extend(block)
                idx = len(lines)
                continue
            processed, aliases, _, block_has_sntest = _process_import_block(block)
            aliases_to_replace.update(aliases)
            has_sntest_import = has_sntest_import or block_has_sntest
            new_lines.extend(processed)
        elif stripped.startswith("import "):
            processed_line, aliases, _, line_has_sntest = _process_single_import(
                line, has_sntest_import
            )
            aliases_to_replace.update(aliases)
            has_sntest_import = has_sntest_import or line_has_sntest
            if processed_line is not None:
                new_lines.append(processed_line)
        else:
            new_lines.append(line)
        idx += 1

    updated_content = "\n".join(new_lines)
    if original.endswith("\n"):
        updated_content += "\n"
    updated_content, _ = _replace_usages(updated_content, aliases_to_replace)

    if updated_content != original:
        path.write_text(updated_content)
        return True
    return False


def iter_go_files(target: Path) -> Iterable[Path]:
    if target.is_file() and target.suffix == ".go":
        yield target
    elif target.is_dir():
        for file in target.rglob("*.go"):
            yield file


def main() -> None:
    parser = argparse.ArgumentParser(description="Rewrite Go imports for SN test packages.")
    parser.add_argument(
        "target",
        type=Path,
        help="Path to a Go file or directory containing Go files to update.",
    )
    args = parser.parse_args()
    target = args.target.resolve()

    if not target.exists():
        raise SystemExit(f"Target path does not exist: {target}")

    changed_files = []
    for go_file in iter_go_files(target):
        if process_file(go_file):
            changed_files.append(go_file)

    if changed_files:
        print("Updated imports in:")
        for file in changed_files:
            print(f" - {file}")
    else:
        print("No changes were necessary.")


if __name__ == "__main__":
    main()
