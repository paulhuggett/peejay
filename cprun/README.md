This module is used to generate the table of Unicode code point categories
that are used to parse tokens in Peejay.

  - WhiteSpace ::
      - \<TAB\>
      - \<VT\>
      - \<FF\>
      - \<SP\>
      - \<NBSP\>
      - \<BOM\>
      - \<USP\>

  - IdentifierName ::
      - IdentifierStart
      - IdentifierName
      - IdentifierPart

  - IdentifierStart ::
      - UnicodeLetter
      - $
      - _
      - \\ UnicodeEscapeSequence

  - IdentifierPart ::
      - IdentifierStart
      - UnicodeCombiningMark
      - UnicodeDigit
      - UnicodeConnectorPunctuation
      - \<ZWNJ\>
      - \<ZWJ\>

  - UnicodeLetter ::
      - any character in the Unicode categories “Uppercase letter (Lu)”, “Lowercase letter (Ll)”, “Titlecase letter (Lt)”, “Modifier letter (Lm)”, “Other letter (Lo)”, or “Letter number (Nl)”

  - UnicodeCombiningMark ::
      - any character in the Unicode categories “Non-spacing mark (Mn)” or “Spacing mark (Mc)”

  - UnicodeDigit ::
      - any character in the Unicode category “Decimal number (Nd)”

  - UnicodeConnectorPunctuation ::
      - any character in the Unicode category “Connector punctuation (Pc)”

  - UnicodeEscapeSequence ::
      - u HexDigit HexDigit HexDigit HexDigit

  - HexDigit ::
      - one of 0 1 2 3 4 5 6 7 8 9 a b c d e f A B C D E F

This table shows the Unicode code points that are assigned particular roles by the ECMAScript grammar rules. (Note that I've excluded '\' U+005C REVERSE SOLIDUS as used to prefix the UnicodeEscapeSequence rule.)

Code Point [Unicode Name]          | Formal Name | Grammar Rule
---------------------------------- | ----------- | ---------------
U+0009 [CHARACTER TABULATION]      | \<TAB\>     | Whitespace
U+000B [LINE TABULATION]           | \<VT\>      | Whitespace
U+000C [FORM FEED (FF)]            | \<FF\>      | Whitespace
U+0020 [SPACE]                     | \<SP\>      | Whitespace
U+0024 [DOLLAR SIGN]               |             | IdentifierStart
U+00A0 [NO-BREAK SPACE]            | \<NBSP\>    | Whitespace
U+005F [LOW LINE]                  |             | IdentifierStart
U+200C [ZERO WIDTH NON-JOINER]     | \<ZWNJ\>    | IdentifierPart
U+200D [ZERO WIDTH JOINER]         | \<ZWJ\>     | IdentifierPart
U+FEFF [ZERO WIDTH NO-BREAK SPACE] | \<BOM\>     | Whitespace

The table below shows the Unicode General Category values that are referenced by ECMAScript grammar rules.

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
