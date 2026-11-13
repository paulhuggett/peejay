Read an object with optional properties
=======================================

An :cpp:class:`peejay::rules::object_rule` has two lists of properties. Every
**required** property must be present in the object. An **optional** property may be
present or absent. This program reads a server's settings, where ``host`` and ``port``
are required, and ``timeout`` and ``verbose`` are optional:

.. literalinclude:: ../examples/howto_rules_optional.cpp
   :language: cpp
   :start-after: SPDX-License-Identifier: MIT

Output:

.. code-block:: text

   alpha:80 timeout=5 verbose=true
   beta:443 timeout=30 verbose=false
   incomplete object
   unrecognized property

The template arguments
----------------------

``object_rule<Policies, RequiredElements, OptionalElements, UserRules...>``

``RequiredElements``
   The largest number of required properties.

``OptionalElements``
   The largest number of optional properties. It defaults to 0, so a rule with only
   required properties can leave it out, as in ``object_rule<policies, 2>``. If the rule
   has ``UserRules``, give it explicitly, even when it is 0, as in
   ``object_rule<policies, 2, 0, hosts_rule>``.

Either list may be shorter than its maximum. A rule with no required properties, such
as ``object_rule<policies, 0, 3>``, accepts an empty object.

Passing the two lists
---------------------

Pass the required properties and then the optional properties to ``object_rule``'s
constructor. Each list has its own pair of braces, so the whole argument has three
levels of braces:

.. code-block:: cpp

   : object_rule{{{u8"host"sv, &s->host}, {u8"port"sv, &s->port}},  // required
                 {{u8"timeout"sv, &s->timeout}}}                     // optional

Without the braces around each list, as in ``object_rule{{u8"host"sv, &s->host},
{u8"port"sv, &s->port}}``, there is only one list and every property in it is required.

Instead of initializer lists, the constructor also accepts two ranges, such as two
``std::array``\ s, or two iterator and sentinel pairs. ``set_properties()`` takes the same
arguments, if you would rather set the properties in the body of your constructor.

Some points to notice
---------------------

* An optional property that is absent leaves its variable unchanged. Give the variable
  its default value before the parse: here, the member initializers of ``server`` do
  that.
* An object that lacks a required property fails with ``incomplete object``, as the
  third document does.
* A key that is in neither list fails with ``unrecognized property``, as the fourth
  document does.
* An optional property is not the same as a property whose value may be ``null``. To
  accept ``null``, write a rule for the value, such as the ``optional_int_rule`` in
  :doc:`write-a-rule`.
