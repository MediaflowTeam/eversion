#!/usr/bin/env python3

from __future__ import annotations

import re
import unittest
from pathlib import Path


ROOT_DIR = Path(__file__).resolve().parents[1]
CI_WORKFLOW = ROOT_DIR / ".github" / "workflows" / "ci.yml"
RELEASE_BRANCHING = ROOT_DIR / "docs" / "release-branching.md"
EXPECTED_BRANCHES = {"main", "dev", "release/**"}


def is_ignored(line: str) -> bool:
    """Returns True for blank lines and comment lines."""
    return not line.strip() or line.lstrip().startswith("#")


def indent_of(line: str) -> int:
    return len(line) - len(line.lstrip())


def unquote(value: str) -> str:
    if len(value) >= 2 and value[0] == value[-1] and value[0] in "\"'":
        return value[1:-1]
    return value


def top_level_blocks(workflow: str, key: str) -> list[list[str]]:
    """Returns each column-0 ``key:`` block, up to the next column-0 line."""
    lines = workflow.splitlines()
    blocks: list[list[str]] = []
    for index, line in enumerate(lines):
        if not re.match(rf"^{re.escape(key)}:", line):
            continue
        block = [line]
        for following in lines[index + 1 :]:
            if is_ignored(following):
                block.append(following)
                continue
            if not following[0].isspace():
                break
            block.append(following)
        blocks.append(block)
    return blocks


def child_blocks(block: list[str]) -> list[tuple[str, list[str]]]:
    """Returns each key nested directly below ``block[0]`` with its lines."""
    lines = block[1:]
    indents = [indent_of(line) for line in lines if not is_ignored(line)]
    if not indents:
        return []
    base = min(indents)
    children: list[tuple[str, list[str]]] = []
    current = ""
    for line in lines:
        if is_ignored(line):
            if current:
                children[-1][1].append(line)
            continue
        indent = indent_of(line)
        if indent == base:
            match = re.match(r"^\s*([A-Za-z0-9_-]+):[ \t]*(.*)$", line)
            if match:
                current = match.group(1)
                children.append((current, [line]))
            else:
                current = ""
        elif indent > base and current:
            children[-1][1].append(line)
        else:
            current = ""
    return children


def children_named(block: list[str], key: str) -> list[list[str]]:
    return [child for name, child in child_blocks(block) if name == key]


def sequence_items(block: list[str]) -> list[str]:
    """Returns the scalar items of a YAML sequence block such as ``branches:``."""
    key_indent = indent_of(block[0])
    items: list[str] = []
    for line in block[1:]:
        if is_ignored(line):
            continue
        if indent_of(line) <= key_indent:
            break
        match = re.match(r"^\s*-\s*(.*)$", line)
        if match:
            items.append(unquote(match.group(1).strip()))
    return items


def markdown_section(text: str, heading: str) -> str:
    """Returns a level-2 Markdown section, up to the next level-2 heading."""
    lines = text.splitlines()
    start = -1
    for index, line in enumerate(lines):
        if re.match(rf"^##\s+{re.escape(heading)}\s*$", line):
            start = index
            break
    if start < 0:
        return ""
    end = len(lines)
    for index in range(start + 1, len(lines)):
        if re.match(r"^##\s+", lines[index]):
            end = index
            break
    return "\n".join(lines[start:end])


class CiTriggersContractTest(unittest.TestCase):
    def setUp(self) -> None:
        self.workflow = CI_WORKFLOW.read_text(encoding="utf-8")
        self.docs = RELEASE_BRANCHING.read_text(encoding="utf-8")

    def on_block(self) -> list[str]:
        blocks = top_level_blocks(self.workflow, "on")
        self.assertEqual(
            len(blocks),
            1,
            f"Expected exactly one top-level 'on:' block, found {len(blocks)}.",
        )
        self.assertEqual(
            blocks[0][0].strip(),
            "on:",
            f"Expected the trigger block to start with 'on:', found "
            f"{blocks[0][0]!r}.",
        )
        return blocks[0]

    def event_branches(self, event: str) -> list[str]:
        events = children_named(self.on_block(), event)
        self.assertEqual(
            len(events),
            1,
            f"Expected exactly one {event!r} entry in the 'on:' block, found "
            f"{len(events)}.",
        )
        branch_blocks = children_named(events[0], "branches")
        self.assertEqual(
            len(branch_blocks),
            1,
            f"Expected exactly one 'branches:' list under {event!r}, found "
            f"{len(branch_blocks)}.",
        )
        return sequence_items(branch_blocks[0])

    def test_on_block_is_unique(self) -> None:
        self.on_block()

    def test_push_branches_match_contract(self) -> None:
        found = set(self.event_branches("push"))
        self.assertEqual(
            found,
            EXPECTED_BRANCHES,
            f"Expected push branches {sorted(EXPECTED_BRANCHES)!r}, found "
            f"{sorted(found)!r}.",
        )

    def test_pull_request_branches_match_contract(self) -> None:
        found = set(self.event_branches("pull_request"))
        self.assertEqual(
            found,
            EXPECTED_BRANCHES,
            f"Expected pull_request branches {sorted(EXPECTED_BRANCHES)!r}, "
            f"found {sorted(found)!r}.",
        )

    def test_manual_dispatch_is_enabled(self) -> None:
        events = children_named(self.on_block(), "workflow_dispatch")
        self.assertEqual(
            len(events),
            1,
            "Expected exactly one 'workflow_dispatch' entry in the 'on:' "
            f"block, found {len(events)}.",
        )

    def test_ci_policy_documents_triggers(self) -> None:
        section = markdown_section(self.docs, "CI Policy")
        self.assertTrue(
            section,
            "Expected a '## CI Policy' section in "
            f"{RELEASE_BRANCHING.relative_to(ROOT_DIR)}.",
        )
        branches = set(self.event_branches("push"))
        branches.update(self.event_branches("pull_request"))
        for branch in sorted(branches):
            expected = f"`{branch}`"
            self.assertIn(
                expected,
                section,
                f"Expected {expected} in the CI Policy section, but it is "
                "missing.",
            )
        self.assertIn(
            "`workflow_dispatch`",
            section,
            "Expected `workflow_dispatch` in the CI Policy section, but it is "
            "missing.",
        )


if __name__ == "__main__":
    unittest.main()
