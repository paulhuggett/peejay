#!/usr/bin/python3
"""This module decodes the contents of UnicodeData.txt (from from
<https://www.unicode.org/Public/zipped/15.0.0/UCD.zip>.

The primary export is the function read_unicode_data() which will read the 
file and return a dictionary containing the fully decoded contents."""

import csv
from enum import Enum
from typing import Annotated, NewType, Optional, TypedDict, Union
from collections.abc import Sequence
import fractions

__all__ = [
    'BidiClass', 
    'CodePoint', 
    'CodePointValueDict', 
    'DbDict', 
    'Decomposition', 
    'FormattingFlag', 
    'GeneralCategory', 
    'NumericTypeEnum', 
    'read_unicode_data', 
]
__author__ = 'Paul Bowen-Huggett'

# The list of Unicode character categories which provides for the most general
# classification of a code point. Drawn from:
# https://www.unicode.org/reports/tr44/#General_Category_Values
GeneralCategory = Enum(
    'GeneralCategory', 
    [
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
    ]
)

# A table which converts from the abbreviated General_Category property value
# alias to the Category enumeration.
GENERAL_CATEGORY_ABBR_TO_ENUM = {
    'Lu': GeneralCategory.Uppercase_Letter,
    'Ll': GeneralCategory.Lowercase_Letter,
    'Lt': GeneralCategory.Titlecase_Letter,
    'Lm': GeneralCategory.Modifier_Letter,
    'Lo': GeneralCategory.Other_Number,
    'Mn': GeneralCategory.Nonspacing_Mark,
    'Mc': GeneralCategory.Spacing_Mark,
    'Me': GeneralCategory.Enclosing_Mark,
    'Nd': GeneralCategory.Decimal_Number,
    'Nl': GeneralCategory.Letter_Number,
    'No': GeneralCategory.Other_Number,
    'Pc': GeneralCategory.Connector_Punctuation,
    'Pd': GeneralCategory.Dash_Punctuation,
    'Ps': GeneralCategory.Open_Punctuation,
    'Pe': GeneralCategory.Close_Punctuation,
    'Pi': GeneralCategory.Initial_Punctuation,
    'Pf': GeneralCategory.Final_Punctuation,
    'Po': GeneralCategory.Other_Punctuation,
    'Sm': GeneralCategory.Math_Symbol,
    'Sc': GeneralCategory.Currency_Symbol,
    'Sk': GeneralCategory.Modifier_Symbol,
    'So': GeneralCategory.Other_Symbol,
    'Zs': GeneralCategory.Space_Separator,
    'Zl': GeneralCategory.Line_Separator,
    'Zp': GeneralCategory.Paragraph_Separator,
    'Cc': GeneralCategory.Control,
    'Cf': GeneralCategory.Format,
    'Cs': GeneralCategory.Surrogate,
    'Co': GeneralCategory.Private_Use,
    'Cn': GeneralCategory.Unassigned,
}

# https://www.unicode.org/reports/tr44/#Bidi_Class_Values
BidiClass = Enum(
    'BidiClass',
    [
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
    ]
)

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
FormattingFlag = Enum(
    'FormattingFlag',
    [
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
    ]
)

# A dictionary which maps from each of the compatibility formatting tags
# enclosed in angle brackets (<...>) to the corresponding FormattingFlag
# enumeration value.
COMPATIBILITY_FORMATTING_FLAGS = dict([('<' + x.name + '>', x)
                                       for x in FormattingFlag])

# A CodePoint is an integer in the range 0..0x10FFFF
CodePoint = NewType('CodePoint', int)

class Decomposition:
    """Describes how to decompose a code point.

    :param formatting: A canonical mapping is indicated by formatting=None; for compatibility mappings FormattingFlag is used. 
    :param mappings: The code point to which this code point should be decomposed.
    """

    def __init__(self, formatting: Optional[FormattingFlag], mappings:Sequence[CodePoint]) -> None:
        self.formatting = formatting
        self.mappings = mappings

    def __str__(self) -> str:
        return '{0}, {1}'.format(self.formatting, self.mappings)

NumericTypeEnum = Enum('NumericTypeEnum', ['Decimal', 'Digit', 'Numeric'])
CodePointValueDict = TypedDict(
    'CodePointValueDict',
    {
        'Name': str,
        'General_Category': GeneralCategory,
        'Canonical_Combining_Class': str, # TODO: interpret properly
        'Bidi_Class': BidiClass,
        'Decomposition': Decomposition,
        'Numeric_Type': Optional[NumericTypeEnum],
        'Numeric_Value' : Union[None, int, fractions.Fraction],
        'Bidi_Mirrored': bool,
        'Simple_Uppercase_Mapping': Optional[CodePoint],
        'Simple_Lowercase_Mapping': Optional[CodePoint],
        'Simple_Titlecase_Mapping': Optional[CodePoint],
    }
)

def yn_field(x: str) -> bool:
    """Converts the input string to boolean. The input must be either 'Y'
    (True) or 'N' (False).

    :param x: A string which should be either empty or contain 'Y' or 'N'.
    :return: True if the input is 'Y', False if it is 'N'.
    """

    return {'Y': True, 'N': False}[x]

def code_point(x: str) -> CodePoint:
    """Decodes a sequence of hexadecimal digits as a Unicode code point value.

    :param x: A string which should contain a sequence of hexadecimal 
              characters.
    :return: The value of x as a CodePoint.
    """

    return CodePoint(int(x, 16))

def opt_code_point(x: str) -> Optional[CodePoint]:
    """Returns None or an integer given an empty string or a sequence of
    hexadecimal digits which represent a Unicode code point value.

    :param x: A string which should be empty or contain a sequence of
              hexadecimal characters.
    :return: None if x is empty or the value of x as a CodePoint.
    """

    return None if len(x) == 0 else code_point(x)

def decode_decomposition(cp:CodePoint, cell: str) -> Decomposition:
    """Decodes the Decomposition_Type/Decomposition_Mapping fields from
    UnicodeData.txt.

    :param cp: The code point number being decoded.
    :param cell: A string which follows the conventions of the Character
                 Decomposition Mapping section of Unicode TR#44.
    :return: An instance of Decomposition which contains the information
             needed to decompose the code point."""

    if len(cell) == 0:
        return Decomposition(None, [cp])
    parts = cell.split(' ')
    formatting: Optional[FormattingFlag] = None
    if parts[0].startswith('<'):
        assert parts[0].endswith('>')
        formatting = COMPATIBILITY_FORMATTING_FLAGS[parts[0]]
        parts = parts[1:]
    return Decomposition(formatting, [code_point(p) for p in parts])

def get_numeric_type(row:Sequence[str]) -> Optional[NumericTypeEnum]:
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

def get_numeric_value_decimal(row:Sequence[str]) -> int:
    """If the character has the property value Numeric_Type=Decimal, then
    the Numeric_Value of that digit is represented with an integer value
    (limited to the range 0..9) in fields 6, 7, and 8.

    :param row: A sequence of values from one row of the UnicodeData.txt file.
    :return: The Numeric_Value field of a record with Numeric_Type=Decimal.
    """

    result = int(row[6])
    assert result == int(row[7]) and result == int(row[8])
    return result

def get_numeric_value_digit(row:Sequence[str]) -> int:
    """If the character has the property value Numeric_Type=Digit, then the
    Numeric_Value of that digit is represented with an integer value
    (limited to the range 0..9) in fields 7 and 8, and field 6 is null.
    This covers digits that need special handling, such as the compatibility
    superscript digits.

    :param row: A sequence of values from one row of the UnicodeData.txt file.
    :return: The Numeric_Value field of a record with Numeric_Type=Digit.
    """

    result = int(row[7])
    assert int(row[7]) == int(row[8])
    return result

def get_numeric_value_numeric(row:Sequence[str]) -> fractions.Fraction:
    """If the character has the property value Numeric_Type=Numeric, then the
    Numeric_Value of that character is represented with a positive or negative
    integer or rational number in this field, and fields 6 and 7 are null.
    This includes fractions such as, for example, "1/5" for U+2155 VULGAR FRACTION ONE FIFTH.

    :param row: A sequence of values from one row of the UnicodeData.txt file.
    :return: The Numeric_Value field of a record with Numeric_Type=Numeric.
    """

    assert len(row[6]) == 0 and len(row[7]) == 0
    return fractions.Fraction(row[8])

def code_point_value(row:Sequence[str]) -> CodePointValueDict:
    """Takes a row from the UnicodeData.txt file (representing an individual
    code point) and returns a CodePointValue dict which contains the properties
    associated with that code point.

    :param row: A sequence of values from one row of the UnicodeData.txt file.
    :return: A dictionary which contains information from the UnicodeData.txt
             row.
    """

    assert len(row) == 15
    numeric_type = get_numeric_type(row)
    numeric_value: Union[None, int, fractions.Fraction] = {
        None: (lambda _: None),
        NumericTypeEnum.Decimal: get_numeric_value_decimal,
        NumericTypeEnum.Digit: get_numeric_value_digit,
        NumericTypeEnum.Numeric: get_numeric_value_numeric
    }[numeric_type](row)
    simple_uppercase_mapping = opt_code_point(row[12])
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
        'Decomposition': decode_decomposition(code_point(row[0]), row[5]),
        # https://www.unicode.org/reports/tr44/#Numeric_Type
        'Numeric_Type': numeric_type,
        'Numeric_Value': numeric_value,
        # https://www.unicode.org/reports/tr44/#Bidi_Mirrored
        'Bidi_Mirrored': yn_field(row[9]),
        # https://www.unicode.org/reports/tr44/#Simple_Uppercase_Mapping
        'Simple_Uppercase_Mapping': simple_uppercase_mapping,
        # https://www.unicode.org/reports/tr44/#Simple_Lowercase_Mapping
        'Simple_Lowercase_Mapping': opt_code_point(row[13]),
        # https://www.unicode.org/reports/tr44/#Simple_Titlecase_Mapping
        'Simple_Titlecase_Mapping': opt_code_point(row[14]) if len(row[14]) > 0 else simple_uppercase_mapping,
    }

DbDict = Annotated[
    dict[CodePoint, CodePointValueDict],
    'A dictionary showing mapping a Unicode code point to its properties from UnicodeData.txt'
]

def read_unicode_data(uncode_data_path: str) -> DbDict:
  with open(uncode_data_path) as udb:
    return dict([
      (code_point(row[0]), code_point_value(row))
      for row in csv.reader(udb, delimiter=';')
    ])
