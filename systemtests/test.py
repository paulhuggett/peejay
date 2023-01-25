#!/usr/bin/python3
from subprocess import run
from os import walk
from os.path import join, split, splitext
from pathlib import Path
from typing import Generator
import sys

EXIT_SUCCESS = 0
EXIT_FAILURE = 1

def enumerate_files (base_path:Path, ext:str) -> Generator[Path, None, None]:
    for root, dirs, files in walk(base_path):
        # Skip 'hidden' directories.
        dirs[:] = [d for d in dirs if not d.startswith(".")]
        # ... and hidden files.
        files[:] = [f for f in files if not f.startswith(".")]
        for name in files:
            if splitext(name)[1] == ext:
                yield Path(join(root, name))

def run_tests (test_dir:Path, pj_check:Path, extension:str, exit_code:int) -> list[bool]:
    verbose = False
    results = list()
    for p in enumerate_files(test_dir, extension):
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
            print("**FAIL**")
    return results

TESTS = [(".json", EXIT_SUCCESS), (".json5", EXIT_SUCCESS), (".js", EXIT_FAILURE), (".txt", EXIT_FAILURE)]

if __name__ == "__main__":
    test_dir = Path(join(split(__file__)[0], "json5-tests"))
    pj_check = Path("../build_mac/tools/check/Debug/pj-check")
    # Run the tests producing a list of the results lists.
    results = [run_tests (test_dir, pj_check, ext, expected) for ext,expected in TESTS]
    # Flatten the list.
    flat_results = [item for sublist in results for item in sublist]
    failures = flat_results.count(False)
    print("PASSES: {0}, FAILURES={1}".format(flat_results.count(True), failures))
    sys.exit(EXIT_SUCCESS if failures == 0 else EXIT_FAILURE)
