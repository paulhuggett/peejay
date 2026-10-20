#!/usr/bin/env python3
# ===- scripts/run_jsontestsuite.py ----------------------------------------===//
# Copyright © 2026 Paul Bowen-Huggett
# SPDX-License-Identifier: MIT
# ===----------------------------------------------------------------------===//
"""Run pj2-dump over the JSONTestSuite parsing tests.

This mirrors the classification used by JSONTestSuite's run_tests.py:

  y_*  parsing must succeed
  n_*  parsing must fail
  i_*  result is implementation defined

A parser exit status of 0 means "parsing succeeded", 1 means "parsing failed",
and anything else (including death by a signal) is a crash. Results are
written to stdout; the exit status is non-zero if any test produced an
unexpected result, crashed, or timed out.
"""

import argparse
import collections
import os
import pathlib
import subprocess
import sys

EXPECTED_RESULT = "EXPECTED_RESULT"
SHOULD_HAVE_PASSED = "SHOULD_HAVE_PASSED"
SHOULD_HAVE_FAILED = "SHOULD_HAVE_FAILED"
IMPLEMENTATION_PASS = "IMPLEMENTATION_PASS"
IMPLEMENTATION_FAIL = "IMPLEMENTATION_FAIL"
CRASH = "CRASH"
TIMEOUT = "TIMEOUT"

DESCRIPTIONS = {
    EXPECTED_RESULT: "expected result",
    SHOULD_HAVE_PASSED: "parsing should have succeeded but failed",
    SHOULD_HAVE_FAILED: "parsing should have failed but succeeded",
    IMPLEMENTATION_PASS: "result undefined, parsing succeeded",
    IMPLEMENTATION_FAIL: "result undefined, parsing failed",
    CRASH: "parser crashed",
    TIMEOUT: "timeout",
}

# Statuses that indicate a problem with the parser.
FAILURES = (SHOULD_HAVE_PASSED, SHOULD_HAVE_FAILED, CRASH, TIMEOUT)


def classify(filename: str, exit_status: int) -> str:
    """Returns the status of a test given its name and the parser's exit status."""

    if exit_status not in (0, 1):
        return CRASH
    passed = exit_status == 0
    if filename.startswith("y_"):
        return EXPECTED_RESULT if passed else SHOULD_HAVE_PASSED
    if filename.startswith("n_"):
        return SHOULD_HAVE_FAILED if passed else EXPECTED_RESULT
    return IMPLEMENTATION_PASS if passed else IMPLEMENTATION_FAIL


def is_inside_project_tree(inp:Union[pathlib.Path, str]) -> bool:
    inp = pathlib.Path(inp).resolve()
    ok = False
    project_tree = pathlib.Path(__file__).parents[1]
    for root, dirs, _ in project_tree.walk(on_error=print):
        r = pathlib.Path(root).resolve()
        a = any(r / d == inp for d in dirs)
        ok = ok or any(r / d == inp for d in dirs)
    return ok

class CheckPathAction(argparse.Action):
    def __init__(self, option_strings, dest, nargs=None, **kwargs):
        if nargs is not None:
            raise ValueError("nargs not allowed")
        super().__init__(option_strings, dest, **kwargs)
    def __call__(self, parser, namespace, values, option_string=None):
        if not is_inside_project_tree(os.path.dirname(values)):
            raise ValueError("parser must be inside the project tree")
        setattr(namespace, self.dest, values)

def run_test(parser: pathlib.Path, path: pathlib.Path, timeout: float) -> str:
    """Runs the parser on a single test file and returns its status."""

    try:
        status = subprocess.call(
            [str(parser), str(path)],
            stdin=subprocess.DEVNULL,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            timeout=timeout,
        )
    except subprocess.TimeoutExpired:
        return TIMEOUT
    return classify(path.name, status)


def main() -> int:
    """Runs every test and reports the results. Returns the process exit status."""

    arg_parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    arg_parser.add_argument("parser", type=pathlib.Path, action=CheckPathAction, help="path to the pj2-dump executable")
    arg_parser.add_argument(
        "tests", type=pathlib.Path, help="path to the JSONTestSuite test_parsing directory"
    )
    arg_parser.add_argument(
        "--timeout", type=float, default=5.0, help="per-test timeout in seconds (default: 5)"
    )
    arg_parser.add_argument(
        "-v", "--verbose", action="store_true", help="also list tests that gave the expected result"
    )
    args = arg_parser.parse_args()

    if not args.tests.is_dir():
        print(f"Error: '{args.tests}' is not a directory", file=sys.stderr)
        return 2

    paths = sorted(args.tests.glob("*.json"))
    if not paths:
        print(f"Error: no tests found in '{args.tests}'", file=sys.stderr)
        return 2

    counts: collections.Counter[str] = collections.Counter()
    for path in paths:
        status = run_test(args.parser, path, args.timeout)
        counts[status] += 1
        if status != EXPECTED_RESULT or args.verbose:
            print(f"{status}\t{path.name}")

    print()
    print(f"{len(paths)} tests:")
    for status, description in DESCRIPTIONS.items():
        print(f"  {counts[status]:4}  {description}")

    return 1 if any(counts[s] for s in FAILURES) else 0


if __name__ == "__main__":
    sys.exit(main())
