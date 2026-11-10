Write your own rule
===================

The rules that peejay provides cover booleans, numbers, strings, objects with a fixed
set of properties, arrays of one type, and enumerations. For anything else, write a rule
of your own. This program defines two:

* ``optional_int_rule`` accepts an integer or ``null``, and stores it in a
  ``std::optional<int>``.
* ``point_rule`` accepts an array of exactly two numbers, such as ``[1.5, -2]``, and stores
  them in a ``point``.

.. literalinclude:: ../examples/howto_rules_custom.cpp
   :language: cpp
   :start-after: SPDX-License-Identifier: MIT

Output (the text of the last message depends on the standard library):

.. code-block:: text

   home at (1.5, -2) layer 3
   work at (0, 4) layer none
   too many array elements
   Invalid argument

The parts of a rule
-------------------

Derive from :cpp:class:`peejay::rules::rule`
   ``rule`` has a virtual function for each kind of event: ``integer_value()``,
   ``null_value()``, ``begin_array()``, ``element()``, and so on. By default, each rejects
   its event with an error such as ``unexpected null``. Override the functions for the
   events that your rule accepts. Each receives a pointer to the context first.

Define ``name()`` and ``value_type``
   ``name()`` returns a name for the rule. ``value_type`` is the type that the rule fills
   in. A rule needs ``value_type``, and a constructor that takes a ``value_type*``, to be
   the root of a context or one of an object rule's ``UserRules``.

Hold pointers, not values
   A rule must be trivially destructible, because it is discarded without running its
   destructor. Store a pointer to the object that the rule fills in, as both rules here do.

Pop the rule when its value is complete
   ``this->pop(ctx)`` removes the rule from the stack and returns success, so the next
   event goes to the rule underneath. Don't use the rule after popping it.

Push a rule to parse a nested value
   ``point_rule`` doesn't parse the numbers itself. In ``element()``, which is called
   before each member of an array, it pushes a ``float_rule`` to parse the member into
   ``x`` or ``y``. The ``float_rule`` pops itself once it has stored the number, and the next
   event, ``element()`` or ``end_array()``, goes back to ``point_rule``.

List the rules you push in ``children``
   The context computes the size of its stack from the ``children`` of each rule, so a rule
   that pushes must list everything it pushes. ``push<T>()`` takes ``this`` as its first
   argument so that it can check, at compile time, that ``T`` is listed.

Return errors
   Return an empty ``std::error_code`` to continue, or any other value to stop the parse.
   You can return one of the :ref:`rules errors <reference/errors:rules errors>`, a
   standard error such as ``std::errc::invalid_argument``, or an error code of your own
   (see :doc:`stop-the-parse-early`).

Recursive grammars
------------------

A rule can push itself, which is how you parse a structure of unbounded depth, such as
a tree. List the rule in its own ``children``:

.. code-block:: cpp

   struct tree {
     std::vector<tree> children;
   };

   // Accepts nested arrays, such as [[], [[]]].
   class tree_rule final : public peejay::rules::rule<policies> {
   public:
     using value_type = tree;
     using children = peejay::type_list::type_list<tree_rule>;

     explicit tree_rule(tree* const t) noexcept : tree_{t} {}
     char const* name() const noexcept override { return "tree"; }

     std::error_code begin_array(context_base* const) override { return {}; }
     std::error_code element(context_base* const ctx) override {
       return push<tree_rule>(this, ctx, &tree_->children.emplace_back());
     }
     std::error_code end_array(context_base* const ctx) override { return this->pop(ctx); }

   private:
     tree* tree_;
   };

For a recursive grammar, the context's stack has room for the policy's
``max_stack_depth`` rules, which is also the deepest nesting that the parser accepts.

A rule derived from ``object_rule`` can't list itself in its own ``UserRules``. To parse
an object that contains objects of the same type, derive the rule from ``rule`` and
handle ``key()`` yourself.

Tidy up after a failed parse
----------------------------

If the parse fails while your rule is on the stack, the rule's ``abandon()`` function is
called before the rule is discarded. Its argument is ``true`` if a rule that this rule
pushed was also still active. ``homogeneous_array_rule`` uses it to remove the element
that it was parsing. If your rule builds a value in stages, override ``abandon()`` to
remove a partial value.
