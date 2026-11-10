Read a string as an enumeration
===============================

Many JSON documents use a string to choose one of a few options, such as
``"level": "warning"``. :cpp:class:`peejay::rules::enum_rule` matches such a string
against a list and stores the value that goes with it. Derive a rule from it and pass
the list to ``set_enumerators()`` in the constructor:

.. literalinclude:: ../examples/howto_rules_enum.cpp
   :language: cpp
   :start-after: SPDX-License-Identifier: MIT

Output:

.. code-block:: text

   net: level 2
   unrecognized enumerator

Some points to notice:

* The template arguments of ``enum_rule`` are the policy, the type of the value, and the
  largest number of enumerators.
* ``enum_rule``\ 's constructor takes a pointer to the variable that receives the value.
  It is protected, so you always derive your own rule.
* To use the rule for a property of an object, list it in the object rule's
  ``UserRules``, as ``logger_rule`` does. The object rule then uses ``level_rule`` for
  any property that points to a ``level``.
* A string that is not in the list fails with ``unrecognized enumerator``. The match is
  exact and case-sensitive.
* The strings are stored as ``std::u8string_view``, so they must outlive the rule.
  String literals do.
* The value type can also be an integer type, if you use the rule on its own or as the
  rule for an array's members. It can't be an integer type in an object rule's
  ``UserRules``, because the object rule already parses integers as numbers.
