Backends
========

A backend receives the parser's events. Any type that satisfies the
:cpp:concept:`peejay::backend` concept can be a backend. peejay provides four:

* :ref:`The null backend <reference/backends:the null backend>` ignores every event.
* :ref:`The DOM backend <reference/backends:the dom backend>` builds a tree that you
  examine after the parse.
* :ref:`The rules backend <reference/backends:the rules backend>` passes each event to a
  stack of rules that store the values in your own types.
* :ref:`The writer backend <reference/backends:the writer backend>` writes each event as
  JSON text.

The ``backend`` concept
-----------------------

.. code-block:: cpp

   #include "peejay/concepts.hpp"   // Also included by "peejay/json.hpp".

   template <typename Backend> concept backend = /* see below */;

A type ``B`` satisfies ``backend`` when, for an object ``be`` of type ``B``:

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - Expression
     - Requirement
   * - ``typename B::policies``
     - A type that satisfies :cpp:concept:`peejay::policies`. See :doc:`policies`.
   * - ``be.result()``
     - Valid. The return type is not constrained.
   * - ``be.string_value(std::u8string_view{})``
     - Convertible to ``std::error_code``.
   * - ``be.integer_value(policies::integer_type{})``
     - Convertible to ``std::error_code``.
   * - ``be.float_value(policies::float_type{})``
     - Convertible to ``std::error_code``. Required only when ``policies::float_type``
       is not :cpp:struct:`peejay::no_float_type`.
   * - ``be.boolean_value(bool{})``
     - Convertible to ``std::error_code``.
   * - ``be.null_value()``
     - Convertible to ``std::error_code``.
   * - ``be.begin_array()``, ``be.end_array()``
     - Convertible to ``std::error_code``.
   * - ``be.element()``
     - Optional. If present, convertible to ``std::error_code``.
   * - ``be.begin_object()``, ``be.end_object()``
     - Convertible to ``std::error_code``.
   * - ``be.key(std::u8string_view{})``
     - Convertible to ``std::error_code``.

The functions can be static or non-static, ``const`` or not, and ``noexcept`` or not.
They may take their argument by value or by ``const`` reference.

Order of calls
^^^^^^^^^^^^^^

For each JSON value, the parser calls the backend as follows:

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Value
     - Calls
   * - string
     - ``string_value(s)``
   * - number
     - ``integer_value(n)`` or ``float_value(x)``. See
       :ref:`reference/input-format:numbers` for which one is used.
   * - ``true``, ``false``
     - ``boolean_value(b)``
   * - ``null``
     - ``null_value()``
   * - array
     - ``begin_array()``, then for each member in order, ``element()`` (if the backend
       has it) followed by the calls for the member, then ``end_array()``.
   * - object
     - ``begin_object()``, then ``key(k)`` followed by the calls for its value for each
       member in order, then ``end_object()``.

Each function is called as soon as the parser has recognized the corresponding part
of the input. A string or keyword is reported once its last character has been read.
A number is reported once the character after it has been read, because until then
it might continue. If the number is the last thing in the input, it is reported by
``eof()``.

``element()`` lets a backend prepare for each member of an array, for example by
appending an element to a container that the member's value will fill. It is not called
for an empty array. A backend that doesn't need it can leave it out.

Each ``begin_array()`` is matched by one ``end_array()``, and each ``begin_object()`` by
one ``end_object()``, unless the parse stops because of an error.

Return values
^^^^^^^^^^^^^

Return an empty ``std::error_code`` to continue. Returning any other value stops the
parse: the parser records the value, makes no further calls to the backend except
``result()``, and ignores the rest of the input. The value is returned by
:cpp:func:`peejay::parser::last_error`. See :doc:`../howto/stop-the-parse-early`.

String lifetime
^^^^^^^^^^^^^^^

The ``std::u8string_view`` passed to ``string_value()`` and ``key()`` refers to a buffer
inside the parser, and the parser reuses the buffer for the next string. Copy the
contents if you need them after the function returns. Escape sequences have already
been decoded, so the view holds the string's actual UTF-8 content. It may contain
U+0000, which is escaped as ``\u0000`` in JSON.

The ``result()`` function
^^^^^^^^^^^^^^^^^^^^^^^^^

``eof()`` calls ``result()`` once, at the end of every parse, including a parse that
failed. ``eof()`` returns whatever ``result()`` returns, including references, because
it returns ``decltype(auto)``. After an error, the backend may hold a partial result,
so ``result()`` must leave the backend in a valid state whatever has happened.

The null backend
----------------

.. code-block:: cpp

   #include "peejay/null.hpp"

   namespace peejay {
     template <policies Policies = default_policies> class null;
   }

``peejay::null`` accepts every event and ignores it. Its ``result()`` returns
``void``. All of its functions are ``static`` and ``noexcept``. Use it to check that
input is valid JSON, or derive from it to make a backend that defines only the
functions it needs. See :doc:`../howto/use-a-backend-by-reference`.

The DOM backend
---------------

.. code-block:: cpp

   #include "peejay/dom.hpp"

   namespace peejay::dom {
     template <policies PJPolicies = default_policies,
               dom_policies DOMPolicies = default_dom_policies>
     class dom;

     template <policies PJPolicies> class element;
     struct null;
     struct default_dom_policies;
     enum class dom_error;
   }

``dom`` builds a tree of ``element`` objects. Its ``result()`` returns
``std::optional<element>``, which holds the root element. The optional is empty if
the input held no value. After an error, it may hold part of a tree, so check
``last_error()`` before you use it.

``dom`` can be moved but not copied.

``dom::element``
^^^^^^^^^^^^^^^^

An ``element`` holds exactly one of the types in its ``member_types`` list:

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Member type
     - Represents
   * - ``element::null``
     - JSON ``null``. This is :cpp:struct:`peejay::dom::null`, an empty type
       whose objects all compare equal.
   * - ``element::boolean``
     - ``true`` and ``false`` (``bool``).
   * - ``element::integer_type``
     - Integers (``PJPolicies::integer_type``).
   * - ``element::float_type``
     - Other numbers (``PJPolicies::float_type``). Present only when floating point
       is enabled.
   * - ``element::string``
     - Strings (``std::u8string``).
   * - ``element::array``
     - Arrays (``std::vector<element>``).
   * - ``element::object``
     - Objects (``std::unordered_map<std::u8string, element, ...>``). The map supports
       lookup by ``std::u8string_view`` without making a temporary string.

Elements can be moved but not copied. Their members are:

``template <typename T, typename... Args> static element make(Args&&... args)``
   Creates an element that holds a ``T`` constructed from ``args``.

``template <typename T> bool holds() const noexcept``
   *Returns:* ``true`` if the element holds a ``T``.

``template <typename T> T* get_if() noexcept``, ``template <typename T> T const* get_if() const noexcept``
   *Returns:* a pointer to the value if the element holds a ``T``, otherwise ``nullptr``.

``template <typename T> std::optional<std::reference_wrapper<T>> get_object_element(std::u8string_view key)``
   Also available as a ``const`` overload, which returns
   ``std::optional<std::reference_wrapper<T const>>``.

   *Returns:* a reference to the value of member ``key`` if this element is an object
   that has a member ``key`` holding a ``T``. Otherwise returns ``std::nullopt``.

``friend bool operator==(element const&, element const&)``
   *Returns:* ``true`` if both elements hold the same type and equal values. Arrays and
   objects are compared member by member. Floating-point values compare equal if they
   differ by no more than a small relative tolerance.

``T`` must be one of ``member_types`` in all of these. Any other type is rejected at
compile time.

DOM policies
^^^^^^^^^^^^

The second template argument of ``dom`` must satisfy ``peejay::dom::dom_policies``.
It needs two unsigned integral members:

``max_array_size``
   The maximum number of members in an array. More members cause
   :cpp:enumerator:`peejay::dom::dom_error::too_many_array_members`.

``max_object_size``
   The maximum number of members in an object. More members cause
   :cpp:enumerator:`peejay::dom::dom_error::too_many_object_members`.

:cpp:struct:`peejay::dom::default_dom_policies` sets both to 50.

Behaviour
^^^^^^^^^

* If an object has the same key more than once, the last value replaces the earlier
  ones.
* Object members are not kept in document order.
* The DOM allocates memory with ``std::vector``, ``std::unordered_map``, and
  ``std::u8string``, so it can throw ``std::bad_alloc``.

The rules backend
-----------------

.. code-block:: cpp

   #include "peejay/rules/rule.hpp"   // Also included by "peejay/rules/non_terminal_rule.hpp".

   namespace peejay::rules {
     template <policies Policies = default_policies> class backend;
   }

``rules::backend`` reads a document straight into your own variables. It doesn't hold
any rules itself. It refers to a :ref:`context <rules-context>`, which holds a
stack of rules, and passes each event to the rule at the top of that stack. If the stack
is empty, the event fails with ``error::no_active_rule``.

You don't normally construct a ``rules::backend`` or name its type. A context's
``make_parser()`` returns a ``peejay::parser<rules::backend<Policies>>`` that refers to
the context:

.. code-block:: cpp

   peejay::rules::context<policies, config_rule> ctx{&c};
   auto p = ctx.make_parser();
   p.input(document).eof();

* Its ``result()`` returns ``void``. The rules store each value as it is parsed, so after
  ``eof()``, the variables that you gave the rules hold the document. If the parse fails,
  they may hold part of it.
* ``result()`` also empties the context's stack. A context parses one document, so call
  the context's ``reset()`` before you parse another.
* The backend holds only a pointer to the context. It can be copied, and the context must
  outlive it.
* The backend allocates no memory. The context holds its rules in a fixed-size buffer, so
  only your own types, such as ``std::string`` or ``std::vector``, allocate.
* Like the parser's own errors, the rules' errors are ``std::error_code`` values that stop
  the parse. They belong to ``peejay::rules::error``: see
  :ref:`reference/errors:rules errors`.

See :doc:`rules` for the context, the rules that peejay provides, and how to write your
own, and :ref:`the tutorial <tutorial:read values into your own types>` for an
introduction.

The writer backend
------------------

.. code-block:: cpp

   #include "peejay/writer.hpp"

   namespace peejay {
     template <std::output_iterator<char> OutputIterator, policies Policies = default_policies>
     class writer;

     template <policies Policies = default_policies, std::output_iterator<char> OutputIterator>
     writer<OutputIterator, Policies> make_writer(OutputIterator out, std::size_t indent = 0);
   }

``writer`` writes the values that it receives to an output iterator as JSON text. Its
``result()`` returns the output iterator, one past the last character written. Use it
to reformat a document, or call its functions yourself to produce JSON.

.. code-block:: cpp

   std::string out;
   auto p = peejay::make_parser(peejay::writer{std::back_inserter(out), 2});
   p.input(u8R"({"name":"caf\u00e9","sizes":[1,2]})"sv).eof();
   // out is:
   // {
   //   "name": "caf\u00e9",
   //   "sizes": [
   //     1,
   //     2
   //   ]
   // }

The constructor's second argument, ``indent``, chooses the layout:

``0`` (the default)
   Compact. No whitespace is written at all: ``{"name":"caf\u00e9","sizes":[1,2]}``.

Greater than ``0``
   Pretty-printed. Each member of an array or object is written on its own line,
   indented by ``indent`` spaces for each level of nesting, and each key is followed by
   ``": "``. Empty arrays and objects are written as ``[]`` and ``{}``.

Neither layout writes a newline after the top-level value.

Use :cpp:func:`peejay::make_writer` to give your own policies without naming the
iterator type: ``peejay::make_writer<my_policies>(std::back_inserter(out))``.

Behaviour
^^^^^^^^^

* The output is always ASCII. In strings and keys, every character that isn't ASCII is
  written as a ``\uXXXX`` escape with lowercase hexadecimal digits. Characters outside
  the Basic Multilingual Plane are written as a UTF-16 surrogate pair, so U+1F600 is
  written as ``\ud83d\ude00``.
* ``"`` and ``\`` are escaped as ``\"`` and ``\\``. Control characters are written as
  ``\b``, ``\f``, ``\n``, ``\r``, or ``\t`` where JSON has a short escape, and as
  ``\u00XX`` otherwise. ``/`` and DEL (U+007F) are not escaped.
* Integers are written in decimal. Floating-point values are written in the shortest
  form that converts back to the same value. A whole number may have neither a decimal
  point nor an exponent: ``1.0`` is written as ``1``, so if the output is parsed again
  it is passed to ``integer_value()``.
* Object members are written in the order received, including duplicate keys.
* ``writer`` allocates no memory. It needs no buffer for the document, only a bit for
  each level of nesting allowed by the policy's ``max_stack_depth``.

Errors
^^^^^^

* ``string_value()`` and ``key()`` return
  :cpp:enumerator:`peejay::error::bad_unicode_code_point` if the string is not
  well-formed UTF-8. Each ill-formed sequence is written as ``\ufffd``, so the output is
  still valid JSON. The parser passes the bytes of a string to the backend without
  checking them, so this can happen when the parser drives the writer unless you
  :doc:`validate the input <../howto/validate-utf8>`.
* ``float_value()`` returns :cpp:enumerator:`peejay::error::number_out_of_range` for an
  infinity or NaN, which JSON can't represent, and writes nothing.
* ``begin_array()`` and ``begin_object()`` return
  :cpp:enumerator:`peejay::error::nesting_too_deep` if more than ``max_stack_depth``
  arrays and objects would be open, and write nothing.

The parser never causes the last two: it doesn't produce infinities or NaNs, and it
reports ``nesting_too_deep`` itself before it calls the backend. They guard calls that
you make yourself.
