#!/usr/bin/python3
"""
This module is used to generate the table of Unicode code point categories
that are used to parse tokens in Peejay.

WhiteSpace ::<br>
&nbsp;&nbsp;\<TAB\><br>
&nbsp;&nbsp;\<VT\><br>
&nbsp;&nbsp;\<FF\><br>
&nbsp;&nbsp;\<SP\><br>
&nbsp;&nbsp;\<NBSP\><br>
&nbsp;&nbsp;\<BOM\><br>
&nbsp;&nbsp;\<USP\><br>

IdentifierName ::<br>
&nbsp;&nbsp;IdentifierStart<br>
&nbsp;&nbsp;IdentifierName<br>
&nbsp;&nbsp;IdentifierPart

IdentifierStart ::<br>
&nbsp;&nbsp;UnicodeLetter<br>
&nbsp;&nbsp;$<br>
&nbsp;&nbsp;_<br>
&nbsp;&nbsp;\\ UnicodeEscapeSequence

IdentifierPart ::<br>
&nbsp;&nbsp;IdentifierStart<br>
&nbsp;&nbsp;UnicodeCombiningMark<br>
&nbsp;&nbsp;UnicodeDigit<br>
&nbsp;&nbsp;UnicodeConnectorPunctuation<br>
&nbsp;&nbsp;\<ZWNJ\><br>
&nbsp;&nbsp;\<ZWJ\><br>

UnicodeLetter ::
  any character in the Unicode categories “Uppercase letter (Lu)”,
  “Lowercase letter (Ll)”, “Titlecase letter (Lt)”, “Modifier letter (Lm)”,
  “Other letter (Lo)”, or “Letter number (Nl)”.

UnicodeCombiningMark ::
  any character in the Unicode categories “Non-spacing mark (Mn)” or
  “Spacing mark (Mc)”

UnicodeDigit ::
  any character in the Unicode category “Decimal number (Nd)”

UnicodeConnectorPunctuation ::
  any character in the Unicode category “Connector punctuation (Pc)”

UnicodeEscapeSequence ::
  u HexDigit HexDigit HexDigit HexDigit

HexDigit ::
  one of 0 1 2 3 4 5 6 7 8 9 a b c d e f A B C D E F

This table shows the Unicode code points that are assigned particular roles by
the ECMAScript grammar rules. (Note that I've excluded '\' U+005C REVERSE
SOLIDUS as used to prefix the UnicodeEscapeSequence rule.)

Code Point [Unicode Name]          | Formal Name | Grammar Rule
---------------------------------- | ----------- | ---------------
U+0009 [CHARACTER TABULATION]      |             | Whitespace
U+000B [LINE TABULATION]           | \<VT\>      | Whitespace
U+000C [FORM FEED (FF)]            | \<FF\>      | Whitespace
U+0020 [SPACE]                     | \<SP\>      | Whitespace
U+0024 [DOLLAR SIGN]               |             | IdentifierStart
U+00A0 [NO-BREAK SPACE]            | \<NBSP\>    | Whitespace
U+005F [LOW LINE]                  |             | IdentifierStart
U+200C [ZERO WIDTH NON-JOINER]     | \<ZWNJ\>    | IdentifierPart
U+200D [ZERO WIDTH JOINER]         | \<ZWJ\>     | IdentifierPart
U+FEFF [ZERO WIDTH NO-BREAK SPACE] | \<BOM\>     | Whitespace

The table below shows the Unicode General Category values that are referenced by
ECMAScript grammar rules.

General Category               | Formal Name | Grammar Rule
------------------------------ | ----------- | ---------------
Spacing mark (Mc)              |             | IdentifierPart
Connector punctuation (Pc)     |             | IdentifierPart
Decimal number (Nd)            |             | IdentifierPart
Letter number (Nl)             |             | IdentifierStart
Lowercase letter (Ll)          |             | IdentifierStart
Modifier letter (Lm)           |             | IdentifierStart
Non-spacing mark (Mn)          |             | IdentifierPart
Space_Separator (Zs)           | \<USP\>     | Whitespace
Other_Letter (Lo)              |             | IdentifierStart
Titlecase letter (Lt)          |             | IdentifierStart
Uppercase letter (Lu)          |             | IdentifierStart
"""

from collections.abc import MutableSequence
from enum import Enum
from typing import Annotated, Generator, Optional, Sequence
import argparse
import pathlib

from unicode_data import CodePoint, CodePointValueDict, DbDict, GeneralCategory, MAX_CODE_POINT, read_unicode_data

CodePointBitsType = Annotated[int, 'The number of bits used to represent a code point']
CODE_POINT_BITS: CodePointBitsType = 21

RunLengthBitsType = Annotated[int, 'The number of bits used to represent a run length']
RUN_LENGTH_BITS: RunLengthBitsType = 9
MAX_RUN_LENGTH = pow(2, RUN_LENGTH_BITS) - 1

RuleBitsType = Annotated[int, 'The number of bits used to represent a rule']
RULE_BITS: RuleBitsType = 2
MAX_RULE = pow(2, RULE_BITS) - 1

class GrammarRule(Enum):
    whitespace = 0
    identifier_start = 1
    identifier_part = 2

CATEGORY_TO_GRAMMAR_RULE: dict[GeneralCategory, GrammarRule] = {
    GeneralCategory.Spacing_Mark         : GrammarRule.identifier_part,
    GeneralCategory.Connector_Punctuation: GrammarRule.identifier_part,
    GeneralCategory.Decimal_Number       : GrammarRule.identifier_part,
    GeneralCategory.Letter_Number        : GrammarRule.identifier_start,
    GeneralCategory.Lowercase_Letter     : GrammarRule.identifier_start,
    GeneralCategory.Modifier_Letter      : GrammarRule.identifier_start,
    GeneralCategory.Nonspacing_Mark      : GrammarRule.identifier_part,
    GeneralCategory.Space_Separator      : GrammarRule.whitespace,
    GeneralCategory.Other_Letter         : GrammarRule.identifier_start,
    GeneralCategory.Titlecase_Letter     : GrammarRule.identifier_start,
    GeneralCategory.Uppercase_Letter     : GrammarRule.identifier_start,
}

class OutputRow:
    """An individual output row representing a run of unicode code points
    which all belong to the same rule."""

    def __init__(self, code_point: CodePoint, length: int, category: GrammarRule):
        assert code_point < pow(2,CODE_POINT_BITS)
        self.__code_point: CodePoint = code_point
        assert length < pow(2, RUN_LENGTH_BITS)
        self.__length: int = length
        assert category.value < pow(2, RULE_BITS)
        self.__category: GrammarRule = category

    def as_str (self, db: DbDict) -> str:
        return '{{ 0x{0:04x}, {1}, {2} }}, // {3} ({4})'\
            .format(self.__code_point, self.__length, self.__category.value,
                    db[self.__code_point]['Name'], self.__category.name)

class CRAState:
    """A class used to model the state of code_run_array() as it processes
    each unicode code point in sequence."""

    def __init__(self) -> None:
        self.__first: CodePoint = CodePoint(0)
        self.__length: int = 0
        self.__max_length: int = 0
        self.__cat: Optional[GrammarRule] = None

    def new_run(self, code_point: CodePoint, mc: Optional[GrammarRule]) -> None:
        """Start a new potential run of contiguous code points.

        :param code_point: The code point at which the run may start.
        :param mc: The rule associated with this code point or None.
        :return: None
        """

        self.__first = code_point
        self.__cat = mc
        self.__length = int(mc is not None)

    def extend_run (self, mc: Optional[GrammarRule]) -> bool:
        """Adds a code point to the current run or indicates that a new run
        should be started.

        :param mc:
        :return: True if the current run was extended or False if a new run
                 should be started.
        """

        if self.__cat is not None and mc == self.__cat:
            if self.__length > MAX_RUN_LENGTH:
                return False
            self.__length += 1
            self.__max_length = max (self.__max_length, self.__length)
            return True
        return False

    def end_run (self, result: MutableSequence[OutputRow]) -> MutableSequence[OutputRow]:
        """Call at the end of a contiguous run of entries. If there are new
        values, a new Entry will be appended to the 'result' sequence.

        :param result: The sequence to which a new entry may be appended.
        :return: The value 'result'.
        """

        if self.__length > 0 and self.__cat is not None:
            assert self.__first <= MAX_CODE_POINT
            assert self.__length <= MAX_RUN_LENGTH
            assert self.__cat.value <= MAX_RULE
            result.append(OutputRow(self.__first, self.__length, self.__cat))
        return result

    def max_run (self) -> int:
        """The length of the longest contiguous run that has been encountered."""
        return self.__max_length

def all_code_points() -> Generator[CodePoint, None, None]:
    """A generator which yields all of the valid Unicode code points in
    sequence."""

    code_point = CodePoint(0)
    while code_point <= MAX_CODE_POINT:
        yield code_point
        code_point = CodePoint(code_point + 1)

def code_run_array(db: DbDict) -> list[OutputRow]:
    """Produces an array of code runs from the Unicode database dictionary.

    :param db: The Unicode database dictionary.
    :return: An array of code point runs.
    """

    state = CRAState()
    code_runs: list[OutputRow] = list()
    for code_point in all_code_points():
        mc: Optional[GrammarRule] = None
        v: Optional[CodePointValueDict] = db.get(code_point)
        if v is not None:
            mc = CATEGORY_TO_GRAMMAR_RULE.get(v['General_Category'])
            if state.extend_run(mc):
                continue
        state.end_run(code_runs)
        state.new_run(code_point, mc)
    state.end_run(code_runs)
    return code_runs

def patch_special_code_points (db: DbDict) -> DbDict:
    """The ECMAScript grammar rules assigns meaning to some individual Unicode
    code points as well as to entire categories of code point. This function
    ensures that the individual code points belong to categories that will in
    turn be mapped to the correct grammar_rule value.

    :param db: The Unicode database dictionary.
    :return: The Unicode database dictionary.
    """

    # Check that the GeneralCategory enumerations will be eventually mapped to
    # the grammar_rule value we expect.
    assert CATEGORY_TO_GRAMMAR_RULE[GeneralCategory.Space_Separator] == GrammarRule.whitespace
    assert CATEGORY_TO_GRAMMAR_RULE[GeneralCategory.Other_Letter] == GrammarRule.identifier_start
    assert CATEGORY_TO_GRAMMAR_RULE[GeneralCategory.Spacing_Mark] == GrammarRule.identifier_part
    special_code_points = {
        CodePoint(0x0009): GeneralCategory.Space_Separator,
        CodePoint(0x000B): GeneralCategory.Space_Separator,
        CodePoint(0x000C): GeneralCategory.Space_Separator,
        CodePoint(0x0020): GeneralCategory.Space_Separator,
        CodePoint(0x0024): GeneralCategory.Other_Letter,
        CodePoint(0x005F): GeneralCategory.Other_Letter,
        CodePoint(0x00A0): GeneralCategory.Space_Separator,
        CodePoint(0x200C): GeneralCategory.Spacing_Mark,
        CodePoint(0x200D): GeneralCategory.Spacing_Mark,
        CodePoint(0xFEFF): GeneralCategory.Space_Separator
    }
    for cp, cat in special_code_points.items():
        db[cp]['General_Category'] = cat
    return db

def dump_db(db: DbDict) -> None:
    for k, v in db.items():
        print('U+{0:04x} {1}'.format(k, v))

def emit_header(db: DbDict, entries:Sequence[OutputRow], include_guard: str) -> None:
    """Emits a C++ header file which declares the array variable along with the
    necessary types.

    :param db: The unicode database dictionary.
    :param entries: A sequence of OutputRow instances sorted by code point.
    :param include_guard: The name of the header file include guard to be used.
    :return: None
    """

    entries = code_run_array(db)

    print('#ifndef {0}'.format (include_guard))
    print('#define {0}'.format (include_guard))
    print('''
#include <array>
#include <cstdint>

enum class grammar_rule {''')
    print(',\n'.join(['  {0} = {1}'.format(x.name, x.value) for x in GrammarRule]))
    print('''}};

struct cprun {{
  uint_least32_t code_point: {0};
  uint_least32_t length: {1};
  uint_least32_t rule: {2};
}};
'''.format(CODE_POINT_BITS, RUN_LENGTH_BITS, RULE_BITS))
    assert CODE_POINT_BITS + RUN_LENGTH_BITS + RULE_BITS <= 32

    print('extern std::array<cprun, {0}> code_point_runs;\n'.format(len(entries)))
    print('#endif // {0}'.format (include_guard))

def emit_source (db: DbDict, entries:Sequence[OutputRow], header_file:pathlib.Path) -> None:
    """Emits a C++ source file which defines of the array variable. The members
    are a sequence of cprun instances sorted in code point order.

    :param db: The unicode database dictionary.
    :param entries: A sequence of OutputRow instances sorted by code point.
    :param header_file: The path of the header file to be included by this cpp
                        file.
    :result: None
    """

    print('#include "{0}"'.format(header_file))
    print('std::array<cprun, {0}> code_point_runs = {{{{'.format(len(entries)))
    for x in entries:
        print('  {0}'.format(x.as_str(db)))
    print('}};')

if __name__ == '__main__':
    parser = argparse.ArgumentParser(prog = 'ProgramName',
                                     description = 'Generates C++ source and header files which map Unicode code points to ECMAScript grammar rules')
    parser.set_defaults(emit_header=True)
    parser.add_argument('-u', '--unicode-data', help='the path of the UnicodeData.txt file', default='./UnicodeData.txt')
    parser.add_argument('-f', '--header-file', help='the name of the cprun header file', default='cprun.hpp', type=pathlib.Path)
    parser.add_argument('--include-guard', help='the name of header file include guard macro', default='CPRUN_HPP')

    group = parser.add_mutually_exclusive_group()
    group.add_argument('--hpp', help='emit the header file', action='store_true', dest='emit_header')
    group.add_argument('-c', '--cpp', help='emit the source file', action='store_false', dest='emit_header')
    group.add_argument('-d', '--dump', help='dump the Unicode code point database', action='store_true')

    args = parser.parse_args()
    db = read_unicode_data(args.unicode_data)
    if args.dump:
        dump_db(db)
    else:
        db = patch_special_code_points(db)
        entries = code_run_array(db)
        emit_header(db, entries, args.include_guard) if args.emit_header else emit_source(db, entries, args.header_file)
