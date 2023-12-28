#!/usr/bin/python3
# ===- cprun/parse.py -----------------------------------------------------===//
# *                             *
# *  _ __   __ _ _ __ ___  ___  *
# * | '_ \ / _` | '__/ __|/ _ \ *
# * | |_) | (_| | |  \__ \  __/ *
# * | .__/ \__,_|_|  |___/\___| *
# * |_|                         *
# ===----------------------------------------------------------------------===//
#  Distributed under the Apache License v2.0.
#  See <https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT>.
#  SPDX-License-Identifier: Apache-2.0
# ===----------------------------------------------------------------------===//
"""
This module is used to generate the table of Unicode code point categories
that are used to parse tokens in Peejay.
"""

from collections.abc import MutableSequence
from enum import Enum
from typing import Annotated, Generator, Optional, Sequence
import argparse
import pathlib

from unicode_data import CodePoint, CodePointValueDict, DbDict, GeneralCategory,\
                         MAX_CODE_POINT, read_unicode_data

CodePointBitsType = Annotated[
    int, 'The number of bits used to represent a code point']
CODE_POINT_BITS: CodePointBitsType = 21

RunLengthBitsType = Annotated[
    int, 'The number of bits used to represent a run length']
RUN_LENGTH_BITS: RunLengthBitsType = 9
MAX_RUN_LENGTH = pow(2, RUN_LENGTH_BITS) - 1

RuleBitsType = Annotated[int, 'The number of bits used to represent a rule']
RULE_BITS: RuleBitsType = 2
MAX_RULE = pow(2, RULE_BITS) - 1


class GrammarRule(Enum):
    """The roles that each known code-point may serve."""
    WHITESPACE = 0b00
    IDENTIFIER_START = 0b01
    IDENTIFIER_PART = 0b11


def rule_name (rule: GrammarRule) -> str:
    """Converts a GrammerRule name using the Python naming convention (all
    upper-case) to the PJ C++ convention of using snake-case."""

    rule_to_str_map = {
        GrammarRule.WHITESPACE: 'whitespace',
        GrammarRule.IDENTIFIER_START: 'identifier_start',
        GrammarRule.IDENTIFIER_PART: 'identifier_part'
    }
    return rule_to_str_map[rule]


CATEGORY_TO_GRAMMAR_RULE: dict[GeneralCategory, GrammarRule] = {
    GeneralCategory.Spacing_Mark: GrammarRule.IDENTIFIER_PART,
    GeneralCategory.Connector_Punctuation: GrammarRule.IDENTIFIER_PART,
    GeneralCategory.Decimal_Number: GrammarRule.IDENTIFIER_PART,
    GeneralCategory.Letter_Number: GrammarRule.IDENTIFIER_START,
    GeneralCategory.Lowercase_Letter: GrammarRule.IDENTIFIER_START,
    GeneralCategory.Modifier_Letter: GrammarRule.IDENTIFIER_START,
    GeneralCategory.Nonspacing_Mark: GrammarRule.IDENTIFIER_PART,
    GeneralCategory.Space_Separator: GrammarRule.WHITESPACE,
    GeneralCategory.Other_Letter: GrammarRule.IDENTIFIER_START,
    GeneralCategory.Titlecase_Letter: GrammarRule.IDENTIFIER_START,
    GeneralCategory.Uppercase_Letter: GrammarRule.IDENTIFIER_START,
}


class OutputRow:
    """An individual output row representing a run of Unicode code points
    which all belong to the same rule."""

    def __init__(self, code_point: CodePoint, length: int,
                 category: GrammarRule):
        assert code_point < pow(2, CODE_POINT_BITS)
        self.__code_point: CodePoint = code_point
        assert length < pow(2, RUN_LENGTH_BITS)
        self.__length: int = length
        assert category.value < pow(2, RULE_BITS)
        self.__category: GrammarRule = category

    def as_str(self, database: DbDict) -> str:
        name = database[self.__code_point]['Name']
        return f'{{ 0x{self.__code_point:04x}, {self.__length}, {self.__category.value} }}, // {name} ({rule_name (self.__category)})'


class CRAState:
    """A class used to model the state of code_run_array() as it processes
    each unicode code point in sequence."""

    def __init__(self) -> None:
        self.__first: CodePoint = CodePoint(0)
        self.__length: int = 0
        self.__max_length: int = 0
        self.__rule: Optional[GrammarRule] = None

    def new_run(self, code_point: CodePoint,
                rule: Optional[GrammarRule]) -> None:
        """Start a new potential run of contiguous code points.

        :param code_point: The code point at which the run may start.
        :param rule: The rule associated with this code point or None.
        :return: None
        """

        self.__first = code_point
        self.__rule = rule
        self.__length = int(rule is not None)

    def extend_run(self, rule: Optional[GrammarRule]) -> bool:
        """Adds a code point to the current run or indicates that a new run
        should be started.

        :param rule:
        :return: True if the current run was extended or False if a new run
                 should be started.
        """

        if self.__rule is not None and rule == self.__rule:
            if self.__length > MAX_RUN_LENGTH:
                return False
            self.__length += 1
            self.__max_length = max(self.__max_length, self.__length)
            return True
        return False

    def end_run(
            self,
            result: MutableSequence[OutputRow]) -> MutableSequence[OutputRow]:
        """Call at the end of a contiguous run of entries. If there are new
        values, a new Entry will be appended to the 'result' sequence.

        :param result: The sequence to which a new entry may be appended.
        :return: The value 'result'.
        """

        if self.__length > 0 and self.__rule is not None:
            assert self.__first <= MAX_CODE_POINT
            assert self.__length <= MAX_RUN_LENGTH
            assert self.__rule.value <= MAX_RULE
            result.append(OutputRow(self.__first, self.__length, self.__rule))
        return result

    def max_run(self) -> int:
        """The length of the longest contiguous run that has been encountered."""
        return self.__max_length


def all_code_points() -> Generator[CodePoint, None, None]:
    """A generator which yields all of the valid Unicode code points in
    sequence."""

    code_point = CodePoint(0)
    while code_point <= MAX_CODE_POINT:
        yield code_point
        code_point = CodePoint(code_point + 1)


def code_run_array(database: DbDict) -> list[OutputRow]:
    """Produces an array of code runs from the Unicode database dictionary.

    :param database: The Unicode database dictionary.
    :return: An array of code point runs.
    """

    state = CRAState()
    code_runs: list[OutputRow] = []
    for code_point in all_code_points():
        rule: Optional[GrammarRule] = None
        value: Optional[CodePointValueDict] = database.get(code_point)
        if value is not None:
            rule = CATEGORY_TO_GRAMMAR_RULE.get(value['General_Category'])
            if state.extend_run(rule):
                continue
        state.end_run(code_runs)
        state.new_run(code_point, rule)
    state.end_run(code_runs)
    return code_runs


def patch_special_code_points(database: DbDict) -> DbDict:
    """The ECMAScript grammar rules assigns meaning to some individual Unicode
    code points as well as to entire categories of code point. This function
    ensures that the individual code points belong to categories that will in
    turn be mapped to the correct grammar_rule value.

    :param database: The Unicode database dictionary.
    :return: The Unicode database dictionary.
    """

    # Check that the GeneralCategory enumerations will be eventually mapped to
    # the grammar_rule value we expect.
    assert CATEGORY_TO_GRAMMAR_RULE[
        GeneralCategory.Space_Separator] == GrammarRule.WHITESPACE
    assert CATEGORY_TO_GRAMMAR_RULE[
        GeneralCategory.Other_Letter] == GrammarRule.IDENTIFIER_START
    assert CATEGORY_TO_GRAMMAR_RULE[
        GeneralCategory.Spacing_Mark] == GrammarRule.IDENTIFIER_PART
    special_code_points = {
        CodePoint(0x0009): GeneralCategory.Space_Separator,
        CodePoint(0x000A): GeneralCategory.Space_Separator,
        CodePoint(0x000B): GeneralCategory.Space_Separator,
        CodePoint(0x000C): GeneralCategory.Space_Separator,
        CodePoint(0x000D): GeneralCategory.Space_Separator,
        CodePoint(0x0020): GeneralCategory.Space_Separator,
        CodePoint(0x0024): GeneralCategory.Other_Letter,
        CodePoint(0x005F): GeneralCategory.Other_Letter,
        CodePoint(0x00A0): GeneralCategory.Space_Separator,
        CodePoint(0x200C): GeneralCategory.Spacing_Mark,
        CodePoint(0x200D): GeneralCategory.Spacing_Mark,
        CodePoint(0xFEFF): GeneralCategory.Space_Separator
    }
    for code_point, cat in special_code_points.items():
        database[code_point]['General_Category'] = cat
    return database


def dump_database(database: DbDict) -> None:
    for key, value in database.items():
        print(f'U+{key:04x} {value}')


def emit_header(database: DbDict, entries: Sequence[OutputRow], include_guard: str) -> None:
    """Emits a C++ header file which declares the array variable along with the
    necessary types.

    :param database: The Unicode database dictionary.
    :param entries: A sequence of OutputRow instances sorted by code point.
    :param include_guard: The name of the header file include guard to be used.
    :return: None
    """

    print('// This file was auto-generated. DO NOT EDIT!')
    print(f'#ifndef {include_guard}')
    print(f'#define {include_guard}')
    print('''#include <array>
#include <cstdint>
namespace peejay {
enum class grammar_rule : std::uint8_t {''')
    print(',\n'.join([f'  {rule_name(x)} = 0b{x.value:0>2b}' for x in GrammarRule]))
    print(f'''}};
constexpr auto idmask = 0b01U;
struct cprun {{
  std::uint_least32_t code_point: {CODE_POINT_BITS};
  std::uint_least32_t length: {RUN_LENGTH_BITS};
  std::uint_least32_t rule: {RULE_BITS};
}};''')
    assert CODE_POINT_BITS + RUN_LENGTH_BITS + RULE_BITS <= 32

    print(f'inline std::array<cprun, {len(entries)}> const code_point_runs = {{{{')
    for entry in entries:
        print(f'  {entry.as_str(database)}')
    print('}};')
    print('\n} // end namespace peejay')
    print(f'#endif // {include_guard}')


def main():
    """The program's entry point."""

    parser = argparse.ArgumentParser(
        prog='ProgramName',
        description=
        'Generates C++ source and header files which map Unicode code points to ECMAScript grammar rules'
    )
    parser.set_defaults(emit_header=True)
    parser.add_argument('-u',
                        '--unicode-data',
                        help='the path of the UnicodeData.txt file',
                        default='./UnicodeData.txt',
                        type=pathlib.Path)
    parser.add_argument('--include-guard',
                        help='the name of header file include guard macro',
                        default='CPRUN_HPP')

    group = parser.add_mutually_exclusive_group()
    group.add_argument('-d',
                       '--dump',
                       help='dump the Unicode code point database',
                       action='store_true')

    args = parser.parse_args()
    database = read_unicode_data(args.unicode_data)
    if args.dump:
        dump_database(database)
    else:
        database = patch_special_code_points(database)
        entries = code_run_array(database)
        emit_header(database, entries, args.include_guard)

if __name__ == '__main__':
    main ()
