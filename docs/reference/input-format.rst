Accepted input
==============

peejay accepts JSON text as defined by
`RFC 8259 <https://www.rfc-editor.org/rfc/rfc8259>`_ and
`ECMA-404 <https://ecma-international.org/publications-and-standards/standards/ecma-404/>`_,
subject to the limits set by the policy. This page describes how it handles each part
of the grammar, and the places where the standard leaves the choice to the parser.

Documents
---------

* The input must be exactly one JSON value of any type, optionally surrounded by
  whitespace. An empty document is an error, and so is anything other than whitespace
  after the value.
* Whitespace is space, horizontal tab, line feed, and carriage return. Nothing else
  counts as whitespace.
* A UTF-8 byte-order mark is not accepted.
* Comments, trailing commas, single-quoted strings, unquoted keys, ``NaN``, and
  ``Infinity`` are not accepted.

Encoding
--------

The input is a sequence of UTF-8 code units (``char8_t``). Outside strings, only ASCII
characters are valid. Inside strings, the parser checks the characters it interprets,
but copies other bytes to the backend unchanged, without checking that they form
valid UTF-8. See :doc:`../howto/validate-utf8`.

Strings
-------

* All of the escapes defined by JSON are supported: ``\"``, ``\\``, ``\/``, ``\b``,
  ``\f``, ``\n``, ``\r``, ``\t``, and ``\uXXXX``. The hexadecimal digits can be upper or
  lower case.
* ``\uXXXX`` escapes are decoded as UTF-16 and converted to UTF-8. A character outside
  the Basic Multilingual Plane must be written as a surrogate pair, such as
  ``😀``. An unpaired surrogate is an error.
* Unescaped control characters (U+0000 to U+001F) are errors.
* The backend receives the decoded string, which may contain U+0000.
* Object keys follow the same rules as string values.
* The length of each decoded string is limited by ``max_length``.

Numbers
-------

Numbers must match the JSON grammar: an optional minus sign, an integer part with no
leading zeros, an optional fraction, and an optional exponent. A leading ``+``, a
leading ``.``, and leading zeros are not accepted.

The parser chooses which backend function to call as follows:

#. A number written without a fraction or an exponent, whose value fits in the
   policy's ``integer_type``, is passed to ``integer_value()``. ``-0`` is passed as the
   integer 0.
#. Any other number is passed to ``float_value()``, converted to the nearest
   ``float_type`` value (rounding ties to even). This includes integers that are too
   large for ``integer_type``, and numbers such as ``1.0`` and ``1e2`` that are whole
   but are written with a fraction or an exponent.
#. If floating point is disabled (``float_type`` is ``no_float_type``), a number that
   would be passed to ``float_value()`` causes ``number_out_of_range`` instead.

Floating-point conversion is correctly rounded. A number too large for ``float_type``
(for example, ``1e400`` for ``double``) causes ``number_out_of_range``. A number too
small to represent becomes zero.

Objects
-------

* Keys must be strings.
* The parser reports duplicate keys to the backend like any other key. The backend
  decides what to do with them. (The DOM backend keeps the last value.)
* Members are reported in the order they appear in the input.

Limits
------

The policy sets these limits:

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - Limit
     - Error when exceeded
   * - String length (``max_length``)
     - ``string_too_long``
   * - Nesting depth (``max_stack_depth``)
     - ``nesting_too_deep``
   * - Number range (``integer_type``, ``float_type``)
     - ``number_out_of_range``

There is no limit on the total length of the input, on the number of members of an
array or object, or on the number of documents parsed in sequence. The DOM backend
adds its own limits on the number of members. See :ref:`reference/backends:dom
policies`.
