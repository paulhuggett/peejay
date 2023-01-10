#!/usr/bin/python3
"""
This module is used to generate the table of Unicode code point categories
that are used to parse tokens in Peejay.
"""

from enum import Enum
from typing import Annotated, Generator, Optional
from collections.abc import MutableSequence

from unicode_data import CodePoint, CodePointValueDict, DbDict, GeneralCategory, read_unicode_data

MAX_CODE_POINT = 0x10FFFF

RunLengthBitsType = Annotated[int, 'The number of bits used to represent a run length']
RUN_LENGTH_BITS: RunLengthBitsType = 9
MAX_RUN_LENGTH = pow(2, RUN_LENGTH_BITS) - 1

MetaCategoryBitsType = Annotated[int, 'The number of bits used to represent a meta-category']
META_CATEGORY_BITS: MetaCategoryBitsType = 3
MAX_META_CATEGORY = pow(2, META_CATEGORY_BITS) - 1

CodePointBitsType = Annotated[int, 'The number of bits used to represent a code point']
CODE_POINT_BITS: CodePointBitsType = 20

class MetaCategory(Enum):
    letter = 0
    combining_mark = 1
    digit = 2
    connector_punctuation = 3
    space = 4

CATEGORY_TO_META: dict[GeneralCategory, MetaCategory] = {
    GeneralCategory.Uppercase_Letter     : MetaCategory.letter,
    GeneralCategory.Lowercase_Letter     : MetaCategory.letter,
    GeneralCategory.Titlecase_Letter     : MetaCategory.letter,
    GeneralCategory.Modifier_Letter      : MetaCategory.letter,
    GeneralCategory.Other_Letter         : MetaCategory.letter,
    GeneralCategory.Letter_Number        : MetaCategory.letter,
    GeneralCategory.Nonspacing_Mark      : MetaCategory.combining_mark,
    GeneralCategory.Spacing_Mark         : MetaCategory.combining_mark,
    GeneralCategory.Decimal_Number       : MetaCategory.digit,
    GeneralCategory.Connector_Punctuation: MetaCategory.connector_punctuation,
    GeneralCategory.Space_Separator      : MetaCategory.space,
}

class OutputRow:
    """An individual output row representing a run of unicode code points
    which all belong to the same meta-category."""

    def __init__(self, code_point: CodePoint, length: int, category: MetaCategory):
        assert code_point < pow(2,CODE_POINT_BITS)
        self.__code_point: CodePoint = code_point
        assert length < pow(2, RUN_LENGTH_BITS)
        self.__length: int = length
        assert category.value < pow(2, META_CATEGORY_BITS)
        self.__category: MetaCategory = category

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
        self.__cat: Optional[MetaCategory] = None

    def new_run(self, code_point: CodePoint, mc: Optional[MetaCategory]) -> None:
        """Start a new potential run of contiguous code points.

        :param code_point: The code point at which the run may start.
        :param mc: The meta-category associated with this code point or None.
        :return: None
        """

        self.__first = code_point
        self.__cat = mc
        self.__length = int(mc is not None)

    def extend_run (self, mc: Optional[MetaCategory]) -> bool:
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
            assert self.__cat.value <= MAX_META_CATEGORY
            assert CODE_POINT_BITS + RUN_LENGTH_BITS + META_CATEGORY_BITS <= 32
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
        mc: Optional[MetaCategory] = None
        v: Optional[CodePointValueDict] = db.get(code_point)
        if v is not None:
            mc = CATEGORY_TO_META.get(v['General_Category'])
            if state.extend_run(mc):
                continue
        state.end_run(code_runs)
        state.new_run(code_point, mc)
    state.end_run(code_runs)
    return code_runs

def main () -> None:
    db = read_unicode_data('./UnicodeData.txt')

    if False:
        for k, v in db.items():
            print('U+{0:04x} {1}'.format(k, v))

    if True:
        entries = code_run_array(db)

        print('''#include <array>
#include <cstdint>
enum class meta_category {''')
        print(',\n'.join(['  {0} = {1}'.format(x.name, x.value) for x in MetaCategory]))
        print('};')
        print('''struct cprun {{
  uint32_t code_point: {0};
  uint32_t length: {1};
  uint32_t cat: {2};
}};'''.format(CODE_POINT_BITS, RUN_LENGTH_BITS, META_CATEGORY_BITS))
        print('std::array<cprun, {0}> code_point_runs = {{{{'.format(len(entries)))
        for x in entries:
            print('  {0}'.format(x.as_str(db)))
        print('}};')

if __name__ == '__main__':
    main()
