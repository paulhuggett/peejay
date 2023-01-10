#!/usr/bin/python3

# IdentifierName ::
#   IdentifierStart
#   IdentifierName
#   IdentifierPart
# 
# IdentifierStart ::
#   UnicodeLetter
#   $
#   _
#   \ UnicodeEscapeSequence
# 
# IdentifierPart ::
#   IdentifierStart
#   UnicodeCombiningMark
#   UnicodeDigit
#   UnicodeConnectorPunctuation
#   <ZWNJ>
#   <ZWJ>
#
# UnicodeLetter ::
#   any character in the Unicode categories "Uppercase letter (Lu)",
#   "Lowercase letter (Ll)", "Titlecase letter (Lt)", "Modifier letter (Lm)",
#   "Other letter (Lo)", or "Letter number (Nl)".
# 
# UnicodeCombiningMark ::
#   any character in the Unicode categories "Non-spacing mark (Mn)" or
#   "Combining spacing mark (Mc)"
# 
# UnicodeDigit ::
#   any character in the Unicode category "Decimal number (Nd)"
# 
# UnicodeConnectorPunctuation ::
#   any character in the Unicode category "Connector punctuation (Pc)"
#
# UnicodeEscapeSequence ::
#   u HexDigit HexDigit HexDigit HexDigit
#
# HexDigit ::
#   one of 0 1 2 3 4 5 6 7 8 9 a b c d e f A B C D E F
#
# ECMAScript characters:
#
# Code Point          | Name                                | Formal Name | Usage
# ------------------- | ----------------------------------- | ----------- | -----
# \u0009              | Tab                                 | <TAB>       | Whitespace
# \u000B              | Vertical Tab                        | <VT>        | Whitespace
# \u000C              | Form Feed                           | <FF>        | Whitespace
# \u0020              | Space                               | <SP>        | Whitespace
# \u00A0              | No-break space                      | <NBSP>      | Whitespace
# \u200C              | Zero width non-joiner               | <ZWNJ>      | IdentifierPart
# \u200D              | Zero width joiner                   | <ZWJ>       | IdentifierPart
# \uFEFF              | Byte Order Mark                     | <BOM>       | Whitespace
# Other category "Zs" | Any other Unicode "space separator" | <USP>       | Whitespace

import csv
from enum import Enum
from typing import Annotated, Generator, NewType, Optional, TypedDict, Union
from collections.abc import Sequence, MutableSequence
import fractions

# The list of Unicode character categories which provides for the most general
# classification of a code point. Drawn from:
# https://www.unicode.org/reports/tr44/#General_Category_Values
Category = Enum ('Category', [
    'Uppercase_Letter',      # an uppercase letter
    'Lowercase_Letter',      # a lowercase letter
    'Titlecase_Letter',      # a digraphic character, with first part uppercase
    'Modifier_Letter',       # a modifier letter
    'Other_Letter',          # other letters, including syllables & ideographs
    'Nonspacing_Mark',       # non-spacing combining mark (zero advance width)
    'Spacing_Mark',          # spacing combining mark (positive advance width)
    'Enclosing_Mark',        # an enclosing combining mark
    'Decimal_Number',        # a decimal digit
    'Letter_Number',         # a letter-like numeric character
    'Other_Number',          # a numeric character of other type
    'Connector_Punctuation', # a connecting punctuation mark, like a tie
    'Dash_Punctuation',      # a dash or hyphen punctuation mark
    'Open_Punctuation',      # an opening punctuation mark (of a pair)
    'Close_Punctuation',     # a closing punctuation mark (of a pair)
    'Initial_Punctuation',   # an initial quotation mark
    'Final_Punctuation',     # a final quotation mark
    'Other_Punctuation',     # a punctuation mark of other type
    'Math_Symbol',           # a symbol of mathematical use
    'Currency_Symbol',       # a currency sign
    'Modifier_Symbol',       # a non-letter-like modifier symbol
    'Other_Symbol',          # a symbol of other type
    'Space_Separator',       # a space character (of various non-zero widths)
    'Line_Separator',        # U+2028 LINE SEPARATOR only
    'Paragraph_Separator',   # U+2029 PARAGRAPH SEPARATOR only
    'Control',               # a C0 or C1 control code
    'Format',                # a format control character
    'Surrogate',             # a surrogate code point
    'Private_Use',           # a private-use character
    'Unassigned',            # reserved unassigned code point or non-character
])

# A table which converts from the abbreviated General_Category property value
# alias to the Category enumeration.
GENERAL_CATEGORY_ABBR_TO_ENUM = {
    'Lu': Category.Uppercase_Letter,
    'Ll': Category.Lowercase_Letter,
    'Lt': Category.Titlecase_Letter,
    'Lm': Category.Modifier_Letter,
    'Lo': Category.Other_Number,
    'Mn': Category.Nonspacing_Mark,
    'Mc': Category.Spacing_Mark,
    'Me': Category.Enclosing_Mark,
    'Nd': Category.Decimal_Number,
    'Nl': Category.Letter_Number,
    'No': Category.Other_Number,
    'Pc': Category.Connector_Punctuation,
    'Pd': Category.Dash_Punctuation,
    'Ps': Category.Open_Punctuation,
    'Pe': Category.Close_Punctuation,
    'Pi': Category.Initial_Punctuation,
    'Pf': Category.Final_Punctuation,
    'Po': Category.Other_Punctuation,
    'Sm': Category.Math_Symbol,
    'Sc': Category.Currency_Symbol,
    'Sk': Category.Modifier_Symbol,
    'So': Category.Other_Symbol,
    'Zs': Category.Space_Separator,
    'Zl': Category.Line_Separator,
    'Zp': Category.Paragraph_Separator,
    'Cc': Category.Control,
    'Cf': Category.Format,
    'Cs': Category.Surrogate,
    'Co': Category.Private_Use,
    'Cn': Category.Unassigned,
}

# https://www.unicode.org/reports/tr44/#Bidi_Class_Values
BidiClass = Enum('BidiClass', [
    'Left_To_Right',           # any strong left-to-right character
    'Right_To_Left',           # any strong right-to-left (non-Arabic-type) character
    'Arabic_Letter',           # any strong right-to-left (Arabic-type) character
    'European_Number',         # any ASCII digit or Eastern Arabic-Indic digit
    'European_Separator',      # plus and minus signs
    'European_Terminator',     # a terminator in a numeric format context, includes currency signs
    'Arabic_Number',           # any Arabic-Indic digit
    'Common_Separator',        # commas, colons, and slashes
    'Nonspacing_Mark',         # any nonspacing mark
    'Boundary_Neutral',        # most format characters, control codes, or noncharacters
    'Paragraph_Separator',     # various newline characters
    'Segment_Separator',       # various segment-related control codes
    'White_Space',             # spaces
    'Other_Neutral',           # most other symbols and punctuation marks
    'Left_To_Right_Embedding', # U+202A: the LR embedding control
    'Left_To_Right_Override',  # U+202D: the LR override control
    'Right_To_Left_Embedding', # U+202B: the RL embedding control
    'Right_To_Left_Override',  # U+202E: the RL override control
    'Pop_Directional_Format',  # U+202C: terminates an embedding or override control
    'Left_To_Right_Isolate',   # U+2066: the LR isolate control
    'Right_To_Left_Isolate',   # U+2067: the RL isolate control
    'First_Strong_Isolate',    # U+2068: the first strong isolate control
    'Pop_Directional_Isolate', # U+2069: terminates an isolate control
])

# A table which converts from the abbreviated Bidi_Class property value
# alias to the BidiClass enumeration.
BIDI_CLASS_ABBR_TO_ENUM = {
    'L': BidiClass.Left_To_Right,
    'R': BidiClass.Right_To_Left,
    'AL': BidiClass.Arabic_Letter,
    'EN': BidiClass.European_Number,
    'ES': BidiClass.European_Separator,
    'ET': BidiClass.European_Terminator,
    'AN': BidiClass.Arabic_Number,
    'CS': BidiClass.Common_Separator,
    'NSM': BidiClass.Nonspacing_Mark,
    'BN': BidiClass.Boundary_Neutral,
    'B': BidiClass.Paragraph_Separator,
    'S': BidiClass.Segment_Separator,
    'WS': BidiClass.White_Space,
    'ON': BidiClass.Other_Neutral,
    'LRE': BidiClass.Left_To_Right_Embedding,
    'LRO': BidiClass.Left_To_Right_Override,
    'RLE': BidiClass.Right_To_Left_Embedding,
    'RLO': BidiClass.Right_To_Left_Override,
    'PDF': BidiClass.Pop_Directional_Format,
    'LRI': BidiClass.Left_To_Right_Isolate,
    'RLI': BidiClass.Right_To_Left_Isolate,
    'FSI': BidiClass.First_Strong_Isolate,
    'PDI': BidiClass.Pop_Directional_Isolate
}

# The collection of "Compatibility Formatting Tags" from 
# https://www.unicode.org/reports/tr44/#Formatting_Tags_Table
FormattingFlag = Enum('FormattingFlag', [
    'font',     # Font variant (for example, a blackletter form)
    'noBreak',  # No-break version of a space or hyphen
    'initial',  # Initial presentation form (Arabic)
    'medial',   # Medial presentation form (Arabic)
    'final',    # Final presentation form (Arabic)
    'isolated', # Isolated presentation form (Arabic)
    'circle',   # Encircled form
    'super',    # Superscript form
    'sub',      # Subscript form
    'vertical', # Vertical layout presentation form
    'wide',     # Wide (or zenkaku) compatibility character
    'narrow',   # Narrow (or hankaku) compatibility character
    'small',    # Small variant form (CNS compatibility)
    'square',   # CJK squared font variant
    'fraction', # Vulgar fraction form
    'compat',   # Otherwise unspecified compatibility character
])
# A dictionary which mapps from each of the compatibility formatting tags 
# enclosed in angle brackets (<...>) to the corresponding FormattingFlag 
# enumeration value.
COMPATIBILITY_FORMATTING_FLAGS = dict([('<' + x.name + '>', x) 
                                       for x in FormattingFlag])

CodePoint = NewType('CodePoint', int)

class Decomposition:
    def __init__(self, formatting: Optional[FormattingFlag], mappings:Sequence[CodePoint]) -> None:
        self.formatting = formatting
        self.mappings = mappings

    def __str__(self) -> str:
        return '{0}, {1}'.format(self.formatting, self.mappings)

class MetaCategory(Enum):
    letter = 0
    combining_mark = 1
    digit = 2
    connector_punctuation = 3
    space = 4

CATEGORY_TO_META: dict[Category, MetaCategory] = {
    Category.Uppercase_Letter     : MetaCategory.letter,
    Category.Lowercase_Letter     : MetaCategory.letter,
    Category.Titlecase_Letter     : MetaCategory.letter,
    Category.Modifier_Letter      : MetaCategory.letter,
    Category.Other_Letter         : MetaCategory.letter,
    Category.Letter_Number        : MetaCategory.letter,
    Category.Nonspacing_Mark      : MetaCategory.combining_mark,
    Category.Spacing_Mark         : MetaCategory.combining_mark,
    Category.Decimal_Number       : MetaCategory.digit,
    Category.Connector_Punctuation: MetaCategory.connector_punctuation,
    Category.Space_Separator      : MetaCategory.space,
}

NumericTypeEnum = Enum('NumericTypeEnum', ['Decimal', 'Digit', 'Numeric'])
NumericValueType = Union[None, int, fractions.Fraction]
CodePointValueDict = TypedDict('CodePointValueDict', {
    'Name': str,
    'General_Category': Category,
    'Canonical_Combining_Class': str, # TODO: interpret properly
    'Bidi_Class': BidiClass,
    'Decomposition': Decomposition,
    'Numeric_Type': Optional[NumericTypeEnum],
    'Numeric_Value' : NumericValueType,
    'Bidi_Mirrored': bool,
    'Simple_Uppercase_Mapping': Optional[CodePoint],
    'Simple_Lowercase_Mapping': Optional[CodePoint],
    'Simple_Titlecase_Mapping': Optional[CodePoint],
})

def yn_field (x: str) -> bool:
    """Converts the input string to boolean. The input must be either 'Y'
    (True) or 'N' (False).

    :param x: A string which should be either empty or contain 'Y' or 'N'.
    :return: True if the input is 'Y', False if it is 'N'.
    """

    return {'Y': True, 'N': False}[x]

def opt_code_point (x: str) -> Optional[CodePoint]:
    """Returns None or an integer given an empty string or a sequence of
    hexadecimal digits which represent a Unicode code point value.

    :param x: A string which should be empty or contain a sequence of
              hexadecimal characters.
    :return: None if x is empty or the value of x as an integer.
    """

    return None if len(x) == 0 else CodePoint(int(x, 16))

def decode_decomposition (code_point:CodePoint, cell: str) -> Decomposition:
    """Decodes the Decomposition_Type/Decomposition_Mapping fields from 
    UnicodeData.txt.
    
    :param code_point: The code point number being decoded.
    :param cell: A string which follows the conventions of the Character 
                 Decomposition Mapping section of Unicode TR#44.
    :return: An instance of Decomposition which contains the information
             needed to decompose the code point."""

    if len(cell) == 0:
        return Decomposition(None, [code_point])
    parts = cell.split(' ')
    formatting: Optional[FormattingFlag] = None
    if parts[0].startswith('<'):
        assert parts[0].endswith('>')
        formatting = COMPATIBILITY_FORMATTING_FLAGS[parts[0]]
        parts = parts[1:]
    return Decomposition(formatting, [CodePoint(int(p, 16)) for p in parts])

def numeric_type(row:Sequence[str]) -> Optional[NumericTypeEnum]:
    """Numeric_Type is extracted as follows. If fields 6, 7, and 8 in 
    UnicodeData.txt are all non-empty, then Numeric_Type=Decimal. Otherwise, if
    fields 7 and 8 are both non-empty, then Numeric_Type=Digit. Otherwise, if
    field 8 is non-empty, then Numeric_Type=Numeric. For characters  listed in
    the Unihan data files, Numeric_Type=Numeric for characters that have 
    kPrimaryNumeric, kAccountingNumeric, or kOtherNumeric tags. The default
    value is Numeric_Type=None.

    See <https://www.unicode.org/reports/tr44/#Derived_Extracted>.

    :param row: A sequence of values from one row of the UnicodeData.txt file.
    :return: The Numeric_Type field of a record.
    """

    if len(row[6]) > 0 and len(row[7]) > 0 and len(row[8]) > 0:
        return NumericTypeEnum.Decimal
    if len(row[7]) > 0 and len(row[8]) > 0:
        return NumericTypeEnum.Digit
    if len(row[8]) > 0:
        return NumericTypeEnum.Numeric
    return None

def numeric_value_decimal(row:Sequence[str]) -> int:
    """If the character has the property value Numeric_Type=Decimal, then 
    the Numeric_Value of that digit is represented with an integer value
    (limited to the range 0..9) in fields 6, 7, and 8.

    :param row: A sequence of values from one row of the UnicodeData.txt file.
    :return: The Numeric_Value field of a record with Numeric_Type=Decimal.
    """

    Numeric_Value = int(row[6])
    assert Numeric_Value == int(row[7]) and Numeric_Value == int(row[8])
    return Numeric_Value

def numeric_value_digit(row:Sequence[str]) -> int:
    """If the character has the property value Numeric_Type=Digit, then the
    Numeric_Value of that digit is represented with an integer value 
    (limited to the range 0..9) in fields 7 and 8, and field 6 is null. 
    This covers digits that need special handling, such as the compatibility 
    superscript digits.

    :param row: A sequence of values from one row of the UnicodeData.txt file.
    :return: The Numeric_Value field of a record with Numeric_Type=Digit.
    """

    Numeric_Value = int(row[7])
    assert int(row[7]) == int(row[8])
    return Numeric_Value

def numeric_value_numeric (row:Sequence[str]) -> fractions.Fraction:
    """If the character has the property value Numeric_Type=Numeric, then the
    Numeric_Value of that character is represented with a positive or negative
    integer or rational number in this field, and fields 6 and 7 are null. 
    This includes fractions such as, for example, "1/5" for U+2155 VULGAR FRACTION ONE FIFTH.

    :param row: A sequence of values from one row of the UnicodeData.txt file.
    :return: The Numeric_Value field of a record with Numeric_Type=Numeric.
    """

    assert len(row[6]) == 0 and len(row[7]) == 0
    return fractions.Fraction(row[8])

def code_point_value (row:Sequence[str]) -> CodePointValueDict:
    """Takes a row from the UnicodeData.txt file (representing an individual 
    code point) and returns a CodePointValue dict which contains the properties
    associated with that code point.
    
    :param row: A sequence of values from one row of the UnicodeData.txt file.
    :return: A dictionary which contains information from the UnicodeData.txt 
             row.
    """

    assert len (row) == 15
    Numeric_Type = numeric_type (row)
    Numeric_Value: Union[None, int, fractions.Fraction] = {
        None: (lambda _: None),
        NumericTypeEnum.Decimal: numeric_value_decimal,
        NumericTypeEnum.Digit: numeric_value_digit,
        NumericTypeEnum.Numeric: numeric_value_numeric
    }[Numeric_Type](row)
    Simple_Uppercase_Mapping = opt_code_point(row[12])
    return {
        # https://www.unicode.org/reports/tr44/#Name
        'Name': row[1],
        # https://www.unicode.org/reports/tr44/#General_Category
        'General_Category': GENERAL_CATEGORY_ABBR_TO_ENUM[row[2]],
        # https://www.unicode.org/reports/tr44/#Canonical_Combining_Class
        'Canonical_Combining_Class': row[3],
        # https://www.unicode.org/reports/tr44/#Bidi_Class
        'Bidi_Class': BIDI_CLASS_ABBR_TO_ENUM[row[4]],
        # https://www.unicode.org/reports/tr44/#Decomposition_Type
        'Decomposition': decode_decomposition(CodePoint(int(row[0], 16)), row[5]),
        # https://www.unicode.org/reports/tr44/#Numeric_Type
        'Numeric_Type': Numeric_Type,
        'Numeric_Value': Numeric_Value,
        # https://www.unicode.org/reports/tr44/#Bidi_Mirrored
        'Bidi_Mirrored': yn_field (row[9]),
        # https://www.unicode.org/reports/tr44/#Simple_Uppercase_Mapping
        'Simple_Uppercase_Mapping': Simple_Uppercase_Mapping,
        # https://www.unicode.org/reports/tr44/#Simple_Lowercase_Mapping
        'Simple_Lowercase_Mapping': opt_code_point(row[13]),
        # https://www.unicode.org/reports/tr44/#Simple_Titlecase_Mapping
        'Simple_Titlecase_Mapping': opt_code_point(row[14]) if len(row[14]) > 0 else Simple_Uppercase_Mapping,
    }

CodePointBitsType = Annotated[int, 'The number of bits used to represent a code point']
CODE_POINT_BITS: CodePointBitsType = 20

MAX_CODE_POINT = 0x10FFFF

RunLengthBitsType = Annotated[int, 'The number of bits used to represent a run length']
RUN_LENGTH_BITS: RunLengthBitsType = 9
MAX_RUN_LENGTH = pow(2, RUN_LENGTH_BITS) - 1

MetaCategoryBitsType = Annotated[int, 'The number of bits used to represent a meta-category']
META_CATEGORY_BITS: MetaCategoryBitsType = 3
MAX_META_CATEGORY = pow(2, META_CATEGORY_BITS) - 1

DbDict = dict[CodePoint, CodePointValueDict]

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

def read_unicode_data(uncode_data_path: str) -> DbDict:
  with open(uncode_data_path) as udb:
    return dict([(CodePoint(int(row[0], 16)), code_point_value(row))
                 for row in csv.reader(udb, delimiter=';')])

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
