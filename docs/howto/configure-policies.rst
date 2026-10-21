Change the parser's limits and types
====================================

The parser's limits and number types are set at compile time by a **policy**: a type
that the backend names as its nested ``policies`` type. To change them, derive a type
from :cpp:struct:`peejay::default_policies` and redefine only the members that you
want to change.

.. literalinclude:: ../examples/howto_policies.cpp
   :language: cpp
   :start-after: SPDX-License-Identifier: MIT

Output:

.. code-block:: text

   OK
   OK
   number out of range

How to pass a policy to the parser
----------------------------------

The parser always takes its policy from the backend:

* The bundled backends take the policy as their first template argument:
  ``peejay::null<my_policies>`` and ``peejay::dom::dom<my_policies>``.
* In your own backend, declare ``using policies = my_policies;``.

Choosing values
---------------

``max_length``
   The longest string, in UTF-8 code units (bytes), that the parser accepts. It
   applies to object keys and string values, and counts each escape sequence after it
   is decoded, so ``"é"`` counts as 2 bytes. The parser holds a buffer of this
   size, so the parser object grows by one byte for each unit you add.

``max_stack_depth``
   The number of entries in the parse stack. The parser uses some of the entries for
   its own bookkeeping, so the deepest nesting it accepts is a little less than this
   value. With the default of 8, it accepts six levels of nested arrays. Choose a
   value a few larger than the deepest nesting you expect.

``integer_type``
   The signed integer type passed to ``integer_value()``. The default is ``int``. For
   general-purpose input, prefer ``std::int64_t``, because JSON written by other
   programs often contains values beyond 32 bits. An integer that does not fit is
   passed to ``float_value()`` instead.

``float_type``
   ``float``, ``double``, or ``long double``. Set it to ``peejay::no_float_type`` to
   reject every number that is not an integer. The backend then needs no
   ``float_value()`` function.

``pos_tracking``
   Set to ``false`` if you don't need line numbers in error messages. The parser
   becomes slightly smaller and faster.

``max_significant_digits``
   Optional. The number of significant digits of a number that the parser keeps. You
   rarely need to set this: see :ref:`max_significant_digits <max-significant-digits>`.

:doc:`../reference/policies` describes each member precisely.

.. tip::

   The DOM backend has a second policy that limits the number of members in each
   array and object. See :doc:`work-with-the-dom`.
