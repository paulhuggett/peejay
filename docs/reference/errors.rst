Errors
======

.. code-block:: cpp

   #include "peejay/error.hpp"   // Also included by "peejay/json.hpp".

   namespace peejay {
     enum class error : int;
     class error_category;
     std::error_code make_error_code(error e) noexcept;
   }

The parser reports errors as ``std::error_code`` values. The errors it detects itself
are the values of ``peejay::error``. Their category is ``peejay::error_category``,
whose ``name()`` is ``"PJ JSON Parser"``. ``peejay::error`` is registered with
``std::is_error_code_enum``, so its values convert to ``std::error_code``
automatically, and you can compare them directly:

.. code-block:: cpp

   if (p.last_error() == peejay::error::string_too_long) { /* ... */ }

``peejay::error`` values
------------------------

Each example is a complete input that causes the error.

.. list-table::
   :header-rows: 1
   :widths: 25 45 30

   * - Value
     - Meaning
     - Example input
   * - ``none``
     - No error. This value is never reported.
     -
   * - ``bad_unicode_code_point``
     - A string contains an unescaped control character (U+0000 to U+001F), or a
       ``\u`` escape that is an unpaired UTF-16 surrogate.
     - ``"\ud800"``, or a tab inside quotes
   * - ``expected_array_member``
     - Something other than ``,`` or ``]`` follows an array member, or the input
       ends inside an array.
     - ``[1 2]``, ``[1``
   * - ``expected_close_quote``
     - The input ends inside a string.
     - ``"abc``
   * - ``expected_colon``
     - Something other than ``:`` follows an object key.
     - ``{"a" 1}``
   * - ``expected_digits``
     - A minus sign is not followed by a digit.
     - ``-``, ``-a``
   * - ``expected_object_key``
     - An object key is not a string, or a comma is followed by ``}``.
     - ``{1:2}``, ``{"a":1,}``
   * - ``expected_object_member``
     - Something other than ``,`` or ``}`` follows an object member, or the input
       ends inside an object.
     - ``{"a":1 "b":2}``, ``{``
   * - ``expected_token``
     - A value is missing: the input is empty or only whitespace, a value is not
       allowed here, or a comma is followed by ``]``.
     - (empty input), ``]``, ``[1,]``
   * - ``invalid_escape_char``
     - A backslash in a string is followed by a character other than
       ``" \ / b f n r t u``.
     - ``"\x"``
   * - ``invalid_hex_char``
     - A ``\u`` escape contains a character that is not a hexadecimal digit.
     - ``"\u12g4"``
   * - ``nesting_too_deep``
     - Values are nested more deeply than ``max_stack_depth`` allows.
     - ``[[[[[[[[1]]]]]]]]`` with the default policy
   * - ``number_out_of_range``
     - A number is too large for ``float_type``, is not an integer when floating point
       is disabled, cannot be rounded reliably (see
       :ref:`max_significant_digits <max-significant-digits>`), or is incomplete at the end of
       the input.
     - ``1e400``, ``1.``
   * - ``unexpected_extra_input``
     - Something other than whitespace follows the top-level value.
     - ``1 2``, ``truex``, ``01``
   * - ``unrecognized_token``
     - A keyword is misspelled or incomplete, or a number has a ``.`` or exponent with
       no digits after it.
     - ``nul``, ``tru``, ``1.x``, ``1e+x``
   * - ``string_too_long``
     - A string is longer than ``max_length``.
     - A 65-byte string with the default policy

Errors from backends
--------------------

When a backend function returns a non-empty ``std::error_code``, the parser stops and
records that value unchanged. ``last_error()`` therefore returns an error in the
backend's own category, not the category of ``peejay::error``. To check which kind of
error you have, compare it with the enumeration values you expect, or compare
``err.category()`` with the backend's category object.

The rules have their own errors: see :ref:`reference/errors:rules errors`.

The DOM backend's errors are values of ``peejay::dom::dom_error``, in category
``peejay::dom::dom_error_category`` (``"PJ JSON DOM"``):

.. list-table::
   :header-rows: 1
   :widths: 35 65

   * - Value
     - Meaning
   * - ``none``
     - No error. This value is never reported.
   * - ``too_many_array_members``
     - An array has more than ``max_array_size`` members.
   * - ``too_many_object_members``
     - An object has more than ``max_object_size`` members.

Rules errors
------------

.. code-block:: cpp

   #include "peejay/rules/errors.hpp"   // Also included by the other rules headers.

   namespace peejay::rules {
     enum class error : int;
     class error_category;
     std::error_code make_error_code(error e) noexcept;
   }

The :doc:`rules <rules>` report their own errors as values of ``peejay::rules::error``,
in a category whose ``name()`` is ``"peejay rule"``. Like ``peejay::error``, the enumeration
is registered with ``std::is_error_code_enum``.

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Value
     - Meaning
   * - ``none``
     - No error. This value is never reported.
   * - ``unexpected_null``
     - A ``null`` value reached a rule that does not accept one.
   * - ``unexpected_boolean``
     - ``true`` or ``false`` reached a rule that does not accept a boolean.
   * - ``unexpected_number``
     - A number reached a rule that does not accept one. Also reported by
       ``integer_rule`` for a number that the parser passes as floating point, such as
       ``1.5``, ``1e2``, or an integer too large for the policy's ``integer_type``.
   * - ``unexpected_string``
     - A string reached a rule that does not accept one.
   * - ``unexpected_array``
     - An array reached a rule that does not accept one.
   * - ``unexpected_end_array``
     - The end of an array reached a rule that was not parsing an array.
   * - ``unexpected_object``
     - An object reached a rule that does not accept one.
   * - ``unexpected_object_key``
     - An object key reached a rule that was not parsing an object.
   * - ``unexpected_end_object``
     - The end of an object reached a rule that was not parsing an object.
   * - ``unrecognized_property``
     - An object has a key that is not one of the ``object_rule``\ 's properties.
   * - ``incomplete_object``
     - An object ended without one of the ``object_rule``\ 's properties.
   * - ``too_many_array_elements``
     - An array has more members than the ``homogeneous_array_rule``\ 's ``MaxElements``.
   * - ``unrecognized_enumerator``
     - A string is not one of the ``enum_rule``\ 's enumerators.
   * - ``no_active_rule``
     - Input reached a context whose parse had already ended. A context parses only one
       document.

The rules also report three of the parser's errors:

* ``peejay::error::number_out_of_range``: an ``integer_rule`` received an integer that its
  type can't represent, such as ``256`` for ``std::uint8_t`` or ``-1`` for an unsigned type.
* ``peejay::error::string_too_long``: a ``string_rule`` received a string longer than the
  capacity of its ``arrayvec``.
* ``peejay::error::nesting_too_deep``: there was no room on the context's stack for a rule.
