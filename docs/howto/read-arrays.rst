Read an array into a container
==============================

:cpp:class:`peejay::rules::homogeneous_array_rule` parses a JSON array whose members all
have the same type. It appends one element to a container for each member, and pushes
a rule to parse the member's value into that element. This program reads an array of
objects into a ``std::vector``:

.. literalinclude:: ../examples/howto_rules_array.cpp
   :language: cpp
   :start-after: SPDX-License-Identifier: MIT

Output:

.. code-block:: text

   OK: http=80 ssh=22
   OK:
   too many array elements: a=1 b=2 c=3
   incomplete object: http=80

The template arguments
----------------------

``homogeneous_array_rule<Policies, Container, Rule, MaxElements>``

``Container``
   Any container with ``emplace_back()``, ``pop_back()``, and ``clear()``, such as
   ``std::vector``, ``std::deque``, ``std::list``, or ``peejay::arrayvec``. Its elements
   must be default-constructible.

``Rule``
   The rule that parses each member. It is constructed from a pointer to the new element.
   Here, each member is an object, so ``address_rule`` parses it. You can omit ``Rule``
   when the elements are ``bool``, an integer type, the policy's ``float_type``, or a
   string, as in ``homogeneous_array_rule<policies, std::vector<int>>``.

   To read an array of arrays, use another ``homogeneous_array_rule`` as ``Rule``:

   .. code-block:: cpp

      using row_rule = homogeneous_array_rule<policies, std::vector<int>>;
      using matrix_rule = homogeneous_array_rule<policies, std::vector<std::vector<int>>, row_rule>;

``MaxElements``
   The largest number of members, or 0 (the default) for no limit. A longer array fails
   with ``too many array elements``. An array also fails with this error if it has more members
   than the container can hold, so you don't need ``MaxElements`` to protect an ``arrayvec``.

The array rule can be the root of a context, as here, or the rule for a property of an
:cpp:class:`object_rule <peejay::rules::object_rule>`. For a property, list the array rule
in the object rule's ``UserRules``, as ``hosts_rule`` is in
:ref:`the tutorial <tutorial:read values into your own types>`.

What the container holds
------------------------

* The rule clears the container when the array starts, so the array replaces anything
  that was in it before.
* If the parse fails, the container holds the members that were complete. In the last
  line of the output, the second address is incomplete, so it was removed.
* A value that is not an array fails with ``unexpected object``, ``unexpected number``,
  or similar.
