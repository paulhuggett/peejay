Read a document without allocating memory
=========================================

The parser and a rules context never allocate memory: their sizes are fixed when the
program is compiled. ``std::string`` and ``std::vector`` do allocate, though. To read a
document with no allocation at all, hold strings and arrays in
:ref:`peejay::arrayvec <rules-arrayvec>`, a container that has a fixed capacity
and keeps its elements inside itself.

.. literalinclude:: ../examples/howto_rules_fixed.cpp
   :language: cpp
   :start-after: SPDX-License-Identifier: MIT

Output:

.. code-block:: text

   t-01: 21 -4 19
   string too long
   too many array elements
   number out of range

Strings
-------

``string_rule<policies, peejay::arrayvec<char8_t, N>>`` stores a string of up to ``N``
UTF-8 code units. A longer string fails with ``string too long``, and the arrayvec is left
unchanged. ``object_rule`` doesn't handle arrayvec strings itself, so list the string
rule in its ``UserRules``, as ``sensor_rule`` does.

``N`` counts code units, not characters, so a character outside ASCII uses two to four of
them. The policy's ``max_length`` limits every string as well, whatever the capacity of
the arrayvec.

Arrays
------

A ``homogeneous_array_rule`` can append to an arrayvec. Always set its ``MaxElements``
argument to no more than the arrayvec's capacity. The rule then rejects a longer array
with ``too many array elements`` before the arrayvec is full. Without the limit, a long
array would overrun the arrayvec, because ``arrayvec::emplace_back()`` does not check that
there is room.

The size of a context
---------------------

A context holds its stack of rules inside itself. The size of the stack is computed when
the program is compiled, from the rules that can be active at the same time, and is
available as ``context<...>::stack_bytes``. For a grammar that is not recursive, the stack
is exactly large enough. See :ref:`reference/rules:stack size`.
