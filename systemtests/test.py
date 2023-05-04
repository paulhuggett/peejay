#!/usr/bin/python3
# ===- systemtests/test.py ------------------------------------------------===//
# *  _            _    *
# * | |_ ___  ___| |_  *
# * | __/ _ \/ __| __| *
# * | ||  __/\__ \ |_  *
# *  \__\___||___/\__| *
# *                    *
# ===----------------------------------------------------------------------===//
#
#  Distributed under the Apache License v2.0.
#  See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
#  for license information.
#  SPDX-License-Identifier: Apache-2.0
#
# ===----------------------------------------------------------------------===//
from subprocess import run
from os import walk
from os.path import join, split, splitext
from pathlib import Path
from typing import Generator
import sys
import argparse

EXIT_SUCCESS = 0
EXIT_FAILURE = 1

def hidden (name:str) -> bool:
    return name.startswith(".")

def enumerate_files (base_path:Path, ext:str) -> Generator[Path, None, None]:
    for root, dirs, files in walk(base_path):
        # Skip 'hidden' directories.
        dirs[:] = [d for d in dirs if not hidden(d)]
        # ... and hidden files.
        files[:] = [f for f in files if not hidden(f)]
        for name in files:
            if splitext(name)[1] == ext:
                yield Path(join(root, name))

def run_tests (test_dir:Path, pj_check:Path, extension:str, exit_code:int,
               verbose:bool) -> list[bool]:
    """
    Walks a directory hierarchy starting at test_dir and running pj-check on
    each file that has the extension given by 'extension'.

    :param test_dir: The directory tree containing the test inputs.
    :param pj_check: The pj-check executable path.
    :param extension: The file extension used by the tests to be parsed.
    :param exit_code: The expected exit code from pj-check. The test fails is
                      a different result is produced.
    :param verbose:  If true, verbose output is written to stdout.
    :result: A list of booleans. Each member is true for each passing test and
             false for each failing test.
    """

    results = list()
    for p in enumerate_files(test_dir, extension):
        if verbose:
            print(p)
        res = run([pj_check, p],
                  capture_output = True,
                  timeout = 5, # timeout in seconds
                  close_fds = True,
                  universal_newlines = True)
        if verbose:
            if len(res.stdout) > 0:
                print(res.stdout)
            if len(res.stderr) > 0:
                print(res.stderr)
        results.append(res.returncode == exit_code)
        if not results[-1]:
            print('**FAIL** "{0}"'.format (p))
    return results

TESTS = [
    (".json", EXIT_SUCCESS),
    (".json5", EXIT_SUCCESS),
    (".js", EXIT_FAILURE),
    (".txt", EXIT_FAILURE)
]

if __name__ == "__main__":

    parser = argparse.ArgumentParser(prog = sys.argv[0],
                                     description = 'Runs the JSON5 test suite')
    parser.set_defaults(emit_header=True)
    parser.add_argument('path', help='The path of the pj-check executable', type=Path)
    parser.add_argument('--test-dir', help='The directory containing the JSON5 test suite files', type=Path, default=Path(join(split(__file__)[0], "json5-tests")))
    parser.add_argument('-v', '--verbose', help='Produce verbose output', action='store_true')

    args = parser.parse_args()
    if not args.path.is_file():
        print ('"{0}" is not a file: path argument must be the path of the pj-check executable'.format (args.path), file=sys.stderr)
        sys.exit(EXIT_FAILURE)
    test_dir = args.test_dir
    if not test_dir.is_dir():
        print ("test-dir must be a directory", file=sys.stderr)
        sys.exit(EXIT_FAILURE)
    pj_check = args.path
    # Run the tests producing a list of the results lists.
    results = [run_tests (test_dir, pj_check, ext, expected, args.verbose) for ext,expected in TESTS]
    # Flatten the list.
    flat_results = [item for sublist in results for item in sublist]
    failures = flat_results.count(False)
    print("PASSES: {0}, FAILURES={1}".format(flat_results.count(True), failures))
    sys.exit(EXIT_SUCCESS if failures == 0 else EXIT_FAILURE)
