Rules
=====

.. code-block:: cpp

   #include "peejay/rules/non_terminal_rule.hpp"   // Includes all of the headers below.
   #include "peejay/rules/terminals.hpp"           // bool_rule, integer_rule, float_rule, string_rule
   #include "peejay/rules/rule.hpp"                // rule, context, context_base, backend
   #include "peejay/rules/errors.hpp"              // error, error_category

   namespace peejay::rules {
     template <policies Policies, is_rule<Policies> Root> class context;
     template <policies Policies> class context_base;
     template <policies Policies> class backend;
     template <policies Policies> class rule;

     template <policies Policies> class bool_rule;
     template <policies Policies, integer_target T = Policies::integer_type> class integer_rule;
     template <policies Policies> using uinteger_rule = /* see below */;
     template <policies Policies> class float_rule;
     template <policies Policies, string_target String = std::u8string> class string_rule;

     template <policies Policies, std::size_t Elements, user_rule<Policies>... UserRules> class object_rule;
     template <policies Policies, emplace_back_container Container, typename Rule = /* see below */,
               std::size_t MaxElements = 0>
     class homogeneous_array_rule;
     template <policies Policies, enum_target T, std::size_t Elements> class enum_rule;

     enum class error : int;
   }

The rules sub-library reads a JSON document directly into your own types, without
building a DOM and without allocating memory. You describe the shape of the document
with **rules**. Each rule parses one JSON value and stores it in an object that you
supply. The library provides rules for the built-in types, for objects with a fixed set
of properties, for arrays, and for strings that name enumerators. You can also write your
own rules. :ref:`tutorial:read values into your own types` introduces them.

How a parse works
-----------------

A :cpp:class:`context <peejay::rules::context>` holds a stack of rules, and it can make a
parser whose backend passes every event to the rule at the top of that stack. When the
context is constructed, the stack holds one rule: the **root** rule, which parses the
top-level value.

* A rule handles the events it expects by overriding the corresponding virtual member
  function of :cpp:class:`peejay::rules::rule`. Any other event produces one of the
  :ref:`peejay::rules::error <reference/rules:errors>` values, which stops the parse.
* To parse a nested value, a rule **pushes** another rule. That rule then receives the
  events for the nested value.
* When its value is complete, a rule **pops** itself. The rule that pushed it receives the
  following events.

For example, when the root is an :cpp:class:`object_rule <peejay::rules::object_rule>`
for the object ``{"port": 80}``, the parse goes like this:

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Event
     - What happens
   * - ``begin_object()``
     - The object rule accepts it.
   * - ``key("port")``
     - The object rule finds the property and pushes an ``integer_rule`` that writes to
       the property's variable.
   * - ``integer_value(80)``
     - The ``integer_rule`` stores 80 and pops itself.
   * - ``end_object()``
     - The object rule checks that it has seen every property and pops itself. The
       stack is now empty.

A rule writes each value as soon as it is parsed. There is no separate result: after
``eof()``, the objects you supplied hold the document.

.. _rules-context:

``context``
-----------

.. code-block:: cpp

   template <policies Policies, is_rule<Policies> Root>
   class context final : public context_base<Policies>;

``Policies``
   The parser policy. See :doc:`policies`. Every rule in the context must use the same
   policy.

``Root``
   The rule that parses the top-level value. It must derive from ``rule<Policies>`` and
   have a member type ``value_type``.

``template <typename... Args> explicit context(Args&&... args)``
   Constructs the context, and constructs a ``Root`` on its stack from ``args``. The
   argument is normally a pointer to the object that receives the document.

``static constexpr std::size_t stack_bytes``
   The size of the buffer that holds the stack. See
   :ref:`reference/rules:stack size`.

``auto make_parser()``
   *Returns:* a ``peejay::parser<backend<Policies>>`` whose backend passes events to the
   rule at the top of this context's stack. The parser refers to the context, so the
   context must outlive it.

``template <typename... Args> context& reset(Args&&... args)``
   *Requires:* ``std::constructible_from<Root, Args...>`` is true.

   Puts the context back into the state of a newly constructed one, so that it can parse
   another document. Any rules still on the stack are abandoned (see
   :ref:`reference/rules:when a parse fails`) and discarded. Then a new ``Root`` is
   constructed on the stack from ``args``, as the constructor does. You can pass a
   pointer to a different object to receive the next document.

   A parser made from the context before the reset is still in its old state. Reset it
   too, with ``p.reset(&ctx)``, or make a new parser with ``make_parser()``.

   If ``Root``\ 's constructor throws, the stack is left empty, as it is after a parse
   has ended.

   *Returns:* ``*this``.

A context parses a single document. When ``eof()`` ends the parse, any rules that are
still on the stack are abandoned (see :ref:`reference/rules:when a parse fails`) and the
stack is left empty. After that, any input to this context's parser, or to another
parser made from the same context, fails with ``error::no_active_rule``. To parse another
document, call ``reset()``, or use a new context.

.. code-block:: cpp

   peejay::rules::context<policies, config_rule> ctx{&first};
   auto p = ctx.make_parser();
   p.input(document1).eof();

   ctx.reset(&second);
   p.reset(&ctx).input(document2).eof();

A context holds its stack inside itself, so it never allocates memory. It cannot be
copied or moved.

``context_base`` and ``backend``
--------------------------------

``context_base<Policies>`` is the part of a context that does not depend on its rules.
Each event handler receives a pointer to it, which the handler passes on to ``push()`` and
``pop()``. Its only public member function is ``make_parser()``.

``backend<Policies>`` is the backend of the parser that ``make_parser()`` returns. You
don't normally need to name it. Each of its event functions calls the matching function
of the rule at the top of the stack, or returns ``error::no_active_rule`` if the stack is
empty. Its ``result()`` returns ``void``.

``rule``
--------

.. code-block:: cpp

   template <policies Policies> class rule;

The base class of every rule. To write a rule, derive from ``rule<Policies>``, override
the functions for the events that your rule accepts, and override ``name()``.
:doc:`../howto/write-a-rule` shows how.

Member types
^^^^^^^^^^^^

``policies``
   ``Policies``.

``children``
   A ``peejay::type_list::type_list<...>`` of the rules that this rule may push. In
   ``rule`` itself, it is empty. A rule that pushes other rules must declare its own
   ``children``. See :ref:`reference/rules:stack size`.

Event handlers
^^^^^^^^^^^^^^

Each handler receives a ``context_base<Policies>*`` as its first argument, followed by
the event's value, if any. Return an empty ``std::error_code`` to continue, or any other
value to stop the parse. The default implementations reject the event:

.. list-table::
   :header-rows: 1
   :widths: 50 50

   * - Function
     - Default result
   * - ``integer_value(context, Policies::integer_type)``
     - ``error::unexpected_number``
   * - ``float_value(context, Policies::float_type)``
     - ``error::unexpected_number``
   * - ``string_value(context, std::u8string_view)``
     - ``error::unexpected_string``
   * - ``boolean_value(context, bool)``
     - ``error::unexpected_boolean``
   * - ``null_value(context)``
     - ``error::unexpected_null``
   * - ``begin_array(context)``
     - ``error::unexpected_array``
   * - ``element(context)``
     - Success. Called before each member of an array. See below.
   * - ``end_array(context)``
     - ``error::unexpected_end_array``
   * - ``begin_object(context)``
     - ``error::unexpected_object``
   * - ``key(context, std::u8string_view)``
     - ``error::unexpected_object_key``
   * - ``end_object(context)``
     - ``error::unexpected_end_object``

``element()`` is called before each member of an array, after ``begin_array()`` and
before the events for the member itself. It is not called for an empty array. A rule that
parses an array normally pushes the rule for the member here. The strings passed to
``string_value()`` and ``key()`` are valid only until the handler returns.

Other virtual functions
^^^^^^^^^^^^^^^^^^^^^^^

``virtual char const* name() const noexcept = 0``
   *Returns:* a name for the rule. It is not used by the parse.

``virtual void abandon(bool child_active) noexcept``
   Called at the end of a parse that leaves this rule on the stack: the parse failed, or
   the input ended before the rule's value was complete. ``child_active`` is ``true`` if a
   rule that this rule pushed was also still on the stack, so the value it was parsing is
   incomplete. Override it to remove a partly built value. The default does nothing.

Protected members
^^^^^^^^^^^^^^^^^

``template <typename T, typename Self, typename... Args> static std::error_code push(Self const* self, context_base<Policies>* context, Args&&... args)``
   Constructs a ``T`` from ``args`` at the top of the stack. Pass ``this`` as ``self``.
   ``T`` must be listed in ``Self::children``; if it is not, the call does not compile.

   *Returns:* ``peejay::error::nesting_too_deep`` if the stack has no room for a ``T``,
   otherwise success.

``template <typename Self, typename T> static std::error_code push_terminal(Self const* self, context_base<Policies>* context, T* value)``
   Pushes the terminal rule for type ``T`` (see ``type_to_terminal`` below), which stores
   the next value in ``*value``.

``std::error_code pop(context_base<Policies>* context) noexcept``
   Removes this rule, which must be at the top of the stack. Its storage may be reused by
   the next push, so don't use the rule after this call.

   *Returns:* success, so that a handler can end with ``return this->pop(context);``.

Requirements
^^^^^^^^^^^^

A rule is constructed in place on the stack and discarded without running its destructor.
So a rule must be **trivially destructible**: it should hold pointers to the objects that
it fills in, not the objects themselves. Rules can't be copied or assigned.

To be used as the root of a context, or as one of an ``object_rule``\ 's ``UserRules``, a
rule must have a member type ``value_type``. A ``UserRules`` rule must also be
constructible from a ``value_type*``.

Stack size
----------

The context computes the size of its stack at compile time. It starts from the root rule
and follows each rule's ``children`` to every rule that can be pushed:

* If no rule can be reached from itself, the stack holds exactly the largest number of
  bytes that can be in use at once.
* If a rule can be reached from itself, the grammar is recursive. Then the stack holds
  ``max_stack_depth`` copies of the largest rule. The parser rejects input that nests
  more deeply than ``max_stack_depth``, so the limits agree.

Because ``push()`` checks that it is pushing one of the rule's ``children``, the
calculation cannot be wrong without the program failing to compile. If a push still does
not fit, for example because a rule pushes more than one rule for a single value, it
fails with ``peejay::error::nesting_too_deep`` and the parse stops. The stack never
overflows.

A class that declares ``children`` hides the ``children`` of its base class. A base class
that pushes rules can make them known with a specialization of
``peejay::rules::base_children<Rule>``, whose member type ``type`` lists the rules that
the base class pushes. ``object_rule`` does this, so a class derived from it can declare
its own ``children`` safely.

Terminal rules
--------------

These rules each parse a single value and store it through the pointer passed to their
constructor. They are ``final``.

.. list-table::
   :header-rows: 1
   :widths: 30 25 45

   * - Rule
     - ``value_type``
     - Accepts
   * - ``bool_rule<P>``
     - ``bool``
     - ``true`` or ``false``.
   * - ``integer_rule<P, T>``
     - ``T``, by default ``P::integer_type``
     - An integer that ``T`` can represent. Otherwise ``peejay::error::number_out_of_range``.
   * - ``uinteger_rule<P>``
     - The unsigned type that corresponds to ``P::integer_type``
     - A non-negative integer. This is an alias of ``integer_rule``.
   * - ``float_rule<P>``
     - ``P::float_type``
     - Any number. An integer is converted to ``float_type``, which may round a large
       integer.
   * - ``string_rule<P, String>``
     - ``String``, by default ``std::u8string``
     - A string. See below.

``T`` can be any integer type except ``bool`` and the character types (``char``,
``wchar_t``, ``char8_t``, ``char16_t``, and ``char32_t``). ``signed char`` and ``unsigned
char``, and so ``std::int8_t`` and ``std::uint8_t``, are allowed. This is the
``integer_target`` concept.

The parser reports integers as ``P::integer_type``, and any number that doesn't fit in it
as floating point. So an ``integer_rule`` for a type wider than ``integer_type`` receives
only the values that ``integer_type`` can represent: a larger number fails with
``error::unexpected_number``. To read 64-bit values, use a policy whose ``integer_type`` is
``std::int64_t``. An integer written as ``1.0`` or ``1e2`` is also a floating-point number,
so ``integer_rule`` rejects it.

``String`` must satisfy the ``string_target`` concept:

``std::u8string``
   Receives the string.

``std::string``
   Receives the UTF-8 code units unchanged.

``peejay::arrayvec<char8_t, Size>``
   Receives the string without allocating memory. A string longer than ``Size`` code
   units fails with ``peejay::error::string_too_long`` and leaves the arrayvec unchanged.

The value's previous contents are replaced. The policy's ``max_length`` still limits the
length of every string.

``type_to_terminal``
^^^^^^^^^^^^^^^^^^^^

``template <typename T, policies P> struct type_to_terminal``
   Maps a value type to the terminal rule that stores it. Its member ``type`` is
   ``bool_rule<P>`` for ``bool``, ``integer_rule<P, T>`` for an integer type,
   ``float_rule<P>`` for ``P::float_type``, and ``string_rule<P, T>`` for each string type.
   For any other type, it has no member ``type``. ``push_terminal()`` and the default
   ``Rule`` of ``homogeneous_array_rule`` use it.

``object_rule``
---------------

.. code-block:: cpp

   template <policies Policies, std::size_t Elements, user_rule<Policies>... UserRules>
   class object_rule : public rule<Policies>;

A base class for rules that parse a JSON object with a fixed set of properties, such as
a struct. Derive from it, define ``value_type`` and ``name()``, and pass the properties
to ``object_rule``'s constructor:

.. code-block:: cpp

   class point_rule final : public object_rule<policies, 2> {
   public:
     using value_type = point;
     explicit point_rule(point* const pt) : object_rule{{u8"x"sv, &pt->x}, {u8"y"sv, &pt->y}} {}
     [[nodiscard]] char const* name() const noexcept override { return "point"; }
   };

When the derived class is a template, name the base class in full, as in
``object_rule<Policies, 2>{...}``.

``Elements``
   The largest number of properties.

``UserRules``
   The rules for properties of types that are not built in. A property may point to the
   ``value_type`` of any of these rules. Each ``value_type`` must be different from the
   others and from the built-in types.

``using property = std::pair<std::u8string_view, /* variant of pointers */>``
   A property: an object key and a pointer to the variable that receives its value.

``explicit object_rule(std::initializer_list<property>)``
   Constructs the rule with the given properties. Also available for an iterator and
   sentinel, and (``explicit``) for a range. Each is the same as default-constructing the
   rule and then calling ``set_properties()`` with the same arguments. These constructors
   and the default constructor are ``protected``.

``void set_properties(std::initializer_list<property>)``
   Sets the properties, replacing any that were set before. Also available for an iterator
   and sentinel, and for a range. There must be at most ``Elements`` of them. The keys are
   stored as ``std::u8string_view``, so the strings must outlive the rule. String literals
   are the usual choice.

The pointer in each property chooses the rule that parses its value:

* A pointer to ``bool``, ``Policies::float_type``, ``std::u8string``, or ``std::string``,
  or to any of the integer types in ``builtin_integers<Policies>``, uses the matching
  terminal rule. ``builtin_integers`` holds the standard signed and unsigned integer types,
  from ``signed char`` to ``long long``, plus ``Policies::integer_type`` and its unsigned
  type.
* A pointer to the ``value_type`` of one of ``UserRules`` uses that rule. A property that
  holds an object, an array, an enumeration, or a string in an ``arrayvec`` needs a rule
  here.

Behaviour:

* Every property must be present. A key that is not one of the properties fails with
  ``error::unrecognized_property``. An object that lacks a property fails with
  ``error::incomplete_object`` when it ends.
* Properties can appear in any order. If a key appears more than once, the last value
  wins.
* The value must be an object. Anything else fails with the matching ``unexpected_*``
  error.
* A rule can't name itself in its own ``UserRules``, because the rule is not yet a
  complete type. A recursive grammar needs a rule derived directly from ``rule``.

``homogeneous_array_rule``
--------------------------

.. code-block:: cpp

   template <policies Policies, emplace_back_container Container,
             typename Rule = type_to_terminal<typename Container::value_type, Policies>::type,
             std::size_t MaxElements = 0>
     requires(MaxElements == 0 || std::ranges::sized_range<Container>)
   class homogeneous_array_rule final : public rule<Policies>;

Parses a JSON array whose members all have the same type into a container. Its
``value_type`` is ``Container``, so it can be used as a root rule or as one of an
``object_rule``\ 's ``UserRules``.

``Container``
   The container that receives the members. It must satisfy
   ``emplace_back_container``: it has a member type ``value_type``, an ``emplace_back()``
   that appends a default-constructed element and returns a reference to it, and
   ``clear()`` and ``pop_back()``. ``std::vector``, ``std::deque``, ``std::list``, and
   ``peejay::arrayvec`` all qualify.

``Rule``
   The rule that parses each member. It is constructed from a pointer to the new element.
   You can omit it when the element type is ``bool``, an integer type, the policy's
   ``float_type``, or a string type.

``MaxElements``
   The largest number of members, or 0 for no limit. A longer array fails with
   ``error::too_many_array_elements``.

Behaviour:

* The container is cleared at the start of the array, so the array replaces its previous
  contents.
* For each member, an element is appended and ``Rule`` is pushed to parse its value.
* If the container has ``size()`` and ``max_size()`` member functions, a member that
  would make ``size()`` larger than ``max_size()`` fails with
  ``error::too_many_array_elements``. An ``arrayvec``\ 's ``max_size()`` is its capacity,
  so the rule never overruns it, whatever ``MaxElements`` is.
* If the parse fails, the container holds the members that were complete before the
  failure. The element that was being parsed is removed.

``enum_rule``
-------------

.. code-block:: cpp

   template <policies Policies, enum_target T, std::size_t Elements>
   class enum_rule : public rule<Policies>;

A base class for rules that accept one of a fixed set of strings and store a value that
corresponds to it. Derive from it, define ``name()``, and pass ``enum_rule``'s constructor
a ``T*`` and the enumerators:

.. code-block:: cpp

   class colour_rule final : public enum_rule<policies, colour, 3> {
   public:
     explicit colour_rule(colour* const c)
         : enum_rule{c, {{u8"red"sv, colour::red}, {u8"green"sv, colour::green}, {u8"blue"sv, colour::blue}}} {}
     [[nodiscard]] char const* name() const noexcept override { return "colour"; }
   };

``T``
   The ``value_type``: an enumeration, or one of the integer types that ``integer_rule``
   accepts. To use the rule as one of an ``object_rule``\ 's ``UserRules``, ``T`` must be an
   enumeration, because the integer types are already built in.

``Elements``
   The largest number of enumerators.

``using enumerator = std::pair<std::u8string_view, T>``
   A string and the value it stands for.

``enum_rule(T* value, std::initializer_list<enumerator>)``
   Constructs the rule, which stores its result in ``*value``, with the given enumerators.
   Also available with an iterator and sentinel, or a range, in place of the list. Each is
   the same as constructing the rule from ``value`` alone and then calling
   ``set_enumerators()`` with the remaining arguments. All of the constructors are
   ``protected``.

``void set_enumerators(std::initializer_list<enumerator>)``
   Sets the enumerators, replacing any that were set before. Also available for an iterator and sentinel, and for a range.
   There must be at most ``Elements`` of them, and the strings must outlive the rule.

A string that matches none of the enumerators fails with
``error::unrecognized_enumerator``. If two enumerators have the same string, the first is
used.

When a parse fails
------------------

The parse stops at the first error, and ``last_error()`` reports it. Values parsed before
the error have already been written, so the objects you supplied may hold part of the
document. The rules leave them in a valid state:

* A terminal rule writes nothing when it rejects a value.
* A ``homogeneous_array_rule`` removes the element that it was parsing. Its container holds
  only complete elements.
* Any rule still on the stack when the parse ends has its ``abandon()`` function called,
  from the top of the stack down. Override it in your own rules to tidy up.

Errors
------

.. code-block:: cpp

   #include "peejay/rules/errors.hpp"

The rules report their own errors as values of ``peejay::rules::error``, whose category's
``name()`` is ``"peejay rule"``. Like :cpp:enum:`peejay::error`, the enumeration converts to
``std::error_code`` automatically. The rules can also report
``peejay::error::number_out_of_range``, ``peejay::error::string_too_long``, and
``peejay::error::nesting_too_deep``, as described above. :doc:`errors` lists the values.

.. _rules-arrayvec:

``arrayvec``
------------

.. code-block:: cpp

   #include "peejay/details/arrayvec.hpp"   // Also included by the rules headers.

   namespace peejay {
     template <typename T, std::size_t Size> class arrayvec;
   }

``arrayvec`` is a sequence container with a fixed capacity, ``Size``, that holds its
elements inside itself. It never allocates memory. Its interface follows ``std::vector``:
``size()``, ``max_size()``, ``data()``, iterators, ``operator[]``, ``push_back()``,
``emplace_back()``, ``pop_back()``, ``clear()``, ``assign()``, and comparison operators.
Adding an element when the container is full is undefined behaviour, so check
``size()`` against ``max_size()`` first. The rules accept an arrayvec of ``char8_t`` as a
string and any arrayvec as the container of a ``homogeneous_array_rule``.
