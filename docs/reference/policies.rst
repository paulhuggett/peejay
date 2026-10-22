Policies
========

A policy is a type that configures the parser at compile time. The parser uses the
policy named by its backend's nested ``policies`` type.

The ``policy`` concept
----------------------

.. code-block:: cpp

   #include "peejay/concepts.hpp"   // Also included by "peejay/json.hpp".

   template <typename Policy> concept policy = /* see below */;

A type ``P`` satisfies ``policy`` when it has the following members. The values are
normally ``static constexpr`` data members.

.. list-table::
   :header-rows: 1
   :widths: 25 30 45

   * - Member
     - Type
     - Meaning
   * - ``max_length``
     - unsigned integral
     - The maximum length of a string, in UTF-8 code units.
   * - ``max_stack_depth``
     - integral, at least 1
     - The maximum nesting depth of values.
   * - ``pos_tracking``
     - ``bool``
     - Whether the parser counts lines.
   * - ``float_type``
     - floating-point type, or :cpp:struct:`peejay::no_float_type`
     - The type passed to ``float_value()``.
   * - ``integer_type``
     - signed integral type
     - The type passed to ``integer_value()``.
   * - ``max_significant_digits``
     - unsigned integral (optional)
     - The number of significant digits of a number that the parser keeps.

``default_policies``
--------------------

.. code-block:: cpp

   struct default_policies {
     static constexpr std::size_t max_length = 64;
     static constexpr std::size_t max_stack_depth = 8;
     static constexpr bool pos_tracking = true;
     using float_type = double;
     using integer_type = int;
   };

These defaults keep the parser small. Most programs that read JSON from other sources
need larger limits. To change a value, derive from ``default_policies`` and redefine
it. See :doc:`../howto/configure-policies`.

Members
-------

``max_length``
^^^^^^^^^^^^^^

The longest string, measured in UTF-8 code units after escape sequences have been
decoded, that the parser accepts as an object key or a string value. A longer string
causes :cpp:enumerator:`peejay::error::string_too_long`. The parser contains a buffer of
exactly this many ``char8_t``.

``max_stack_depth``
^^^^^^^^^^^^^^^^^^^

The maximum depth to which values may be nested. The top-level value is at depth 1,
and each member of an array or object is one level deeper than the array or object
that contains it. Every value counts, including the innermost one. With the default
of 8:

* ``[[[[[[[1]]]]]]]`` is accepted: 7 arrays, with the number 1 at depth 8.
* ``[[[[[[[[]]]]]]]]`` is accepted: 8 arrays, the innermost of them empty.
* ``[[[[[[[[1]]]]]]]]`` is rejected, because the number 1 is at depth 9.

Input that nests too deeply causes :cpp:enumerator:`peejay::error::nesting_too_deep`.
The parser's stack holds one entry for each level, plus one more for its own use, so
its size depends only on this value. The DOM backend also keeps a stack of
``max_stack_depth`` elements.

``pos_tracking``
^^^^^^^^^^^^^^^^

When ``true``, the parser counts lines, and :cpp:func:`peejay::parser::pos` returns a
:cpp:struct:`peejay::coord` with a ``line`` member. When ``false``, the coordinate is
an empty type, and the counting code is compiled out.

``integer_type``
^^^^^^^^^^^^^^^^

The signed integer type of the values passed to ``integer_value()``. Any standard
signed integer type of up to 64 bits can be used, and ``__int128`` where the compiler
provides it. A number is passed as an integer only when it has no fraction or
exponent and fits in this type. See :ref:`reference/input-format:numbers`.

``float_type``
^^^^^^^^^^^^^^

The type of the values passed to ``float_value()``: ``float``, ``double``, or
``long double`` (radix 2, with a significand of at most 64 bits). Numbers are
converted to the nearest value of this type, with ties going to the even value.

Use ``peejay::no_float_type`` to disable floating point. A number that is not passed
as an integer then causes :cpp:enumerator:`peejay::error::number_out_of_range`, and
the backend does not need a ``float_value()`` function.

.. _max-significant-digits:

``max_significant_digits``
^^^^^^^^^^^^^^^^^^^^^^^^^^

Optional. The parser stores a number's significant digits in a fixed-size buffer.
By default, it keeps 38 digits (39 when ``integer_type`` is 128 bits), which is
enough to hold every value of ``integer_type`` exactly and to convert almost any
number correctly. Digits beyond the limit are noted but not stored.

In the very rare case that the discarded digits decide which way the result rounds,
the parser reports :cpp:enumerator:`peejay::error::number_out_of_range` rather than
risk an incorrect result. If your input contains numbers with very long significands,
raise ``max_significant_digits``. Each extra digit makes the parser slightly larger.
Values below the default have no effect.
