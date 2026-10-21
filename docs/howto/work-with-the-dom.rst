Work with the DOM
=================

The DOM backend, :cpp:class:`peejay::dom::dom`, turns a document into a tree of
:cpp:class:`peejay::dom::element` objects. This guide shows how to visit every element
in the tree, and how to raise the limits on the size of arrays and objects.

Visit every element
-------------------

Each element holds exactly one value. To find out which type of value, call
``holds<T>()`` or ``get_if<T>()``, where ``T`` is one of the element's nested type
aliases:

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Alias
     - Type
   * - ``element::null``
     - :cpp:struct:`peejay::dom::null`, an empty type.
   * - ``element::boolean``
     - ``bool``
   * - ``element::integer_type``
     - The policy's ``integer_type``.
   * - ``element::float_type``
     - The policy's ``float_type``. Not available when it is ``no_float_type``.
   * - ``element::string``
     - ``std::u8string``
   * - ``element::array``
     - ``std::vector<element>``
   * - ``element::object``
     - ``std::unordered_map<std::u8string, element, ...>``

This program prints a description of any document:

.. literalinclude:: ../examples/howto_walk_dom.cpp
   :language: cpp
   :start-after: SPDX-License-Identifier: MIT

Output (the order of object members may differ):

.. code-block:: text

   object (3 members)
     "ratio":
       float 0.5
     "tags":
       array (2 members)
         string "x"
         null
     "id":
       integer 7

Raise the size limits
---------------------

The DOM backend takes a second policy, which limits the number of members in each
array and object. The defaults, in :cpp:struct:`peejay::dom::default_dom_policies`,
are 50 of each. A document that exceeds a limit fails with
:cpp:enumerator:`peejay::dom::dom_error::too_many_array_members` or
:cpp:enumerator:`peejay::dom::dom_error::too_many_object_members`. To raise the limits,
write a type with two unsigned members, as ``dom_policies`` does in the example
above, and pass it as the second template argument:
``peejay::dom::dom<policies, dom_policies>``.

The parser policy (the first template argument) still limits the length of strings
and the depth of nesting.

Things to know about the DOM
----------------------------

* Objects don't keep their members in document order.
* If an object has the same key more than once, the last value wins.
* Elements can be moved but not copied.
* The DOM allocates memory with the standard containers, so it can throw
  ``std::bad_alloc``. The parser itself never allocates.
* Floating-point elements compare equal when they differ by only a tiny relative
  amount, which absorbs rounding differences.
* To build an element yourself, for example to compare it with one from a parse, use
  ``element::make<T>(args...)``. ``==`` compares the type and the value, and compares
  arrays and objects member by member:

  .. code-block:: cpp

     auto const seven = element::make<element::integer_type>(7);
     auto const empty_array = element::make<element::array>();
     bool const is_seven = (some_element == seven);
