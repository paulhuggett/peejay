Tutorial
========

This tutorial builds four small programs. Each one introduces one more part of peejay:

#. :ref:`tutorial:check that a document is valid`, using the ``null`` backend.
#. :ref:`tutorial:read values from a document`, using the DOM backend.
#. :ref:`tutorial:write your own backend`.
#. :ref:`tutorial:parse input that arrives in pieces`.

You need a C++20 compiler and CMake 3.22 or later. The tutorial takes about
twenty minutes. The complete source of every program is in the ``docs/examples``
directory of the peejay repository.

Set up a project
----------------

Create an empty directory with this ``CMakeLists.txt``:

.. code-block:: cmake

   cmake_minimum_required(VERSION 3.22)
   project(pj_tutorial LANGUAGES CXX)

   include(FetchContent)
   FetchContent_Declare(
     peejay
     GIT_REPOSITORY https://github.com/paulhuggett/peejay.git
     GIT_TAG main
   )
   FetchContent_MakeAvailable(peejay)

   add_executable(validate validate.cpp)
   target_compile_features(validate PRIVATE cxx_std_20)
   target_link_libraries(validate PRIVATE peejay::peejay)

Linking to the ``peejay::peejay`` target sets the include paths for peejay and for icubaby,
the Unicode library it depends on. The library itself is header-only. As you add the programs below, add an ``add_executable()`` line and a matching
``target_link_libraries()`` line for each one.

Check that a document is valid
------------------------------

The simplest useful thing a parser can do is tell you whether its input is valid
JSON. Save this as ``validate.cpp``:

.. literalinclude:: examples/tutorial_1_validate.cpp
   :language: cpp
   :start-after: SPDX-License-Identifier: MIT

Build and run it:

.. code-block:: console

   $ cmake -S . -B build && cmake --build build
   $ ./build/validate
   valid JSON

Here is what the program does.

``peejay::parser<peejay::null<>>``
   A parser always has a backend type, which it receives as its template argument.
   ``peejay::null`` is a backend that ignores everything it is given. Its template
   argument is a policy, and the empty ``<>`` selects the default policy,
   :cpp:struct:`peejay::default_policies`.

``p.input(...)``
   Passes text to the parser. The text must be UTF-8 code units of type ``char8_t``,
   so the example uses a ``u8`` string literal. The ``sv`` suffix makes it a
   ``std::u8string_view``. ``input()`` accepts any range of ``char8_t``.

``p.eof()``
   Tells the parser that there is no more input. The parser needs this because some
   values, such as the number ``123``, could still be continuing until it knows the
   input has ended. ``eof()`` returns the backend's result. The ``null`` backend
   returns nothing.

``p.last_error()``
   Returns a ``std::error_code``. An empty error code means the parse succeeded.

Try changing the input to ``{ "name": "web", "port": }`` and running the program
again. It reports ``invalid JSON: expected token``.

Read values from a document
---------------------------

To use the values in a document, you need a backend that stores them.
:cpp:class:`peejay::dom::dom` builds a tree of :cpp:class:`peejay::dom::element`
objects. Each element holds a null, boolean, integer, floating-point number,
string, array, or object.

.. literalinclude:: examples/tutorial_2_dom.cpp
   :language: cpp
   :start-after: SPDX-License-Identifier: MIT

Output:

.. code-block:: text

   name: web
   port: 8080
   debug: false
   host: alpha
   host: beta

Some points to notice:

* ``p.input(config).eof()`` chains the two calls, because ``input()`` returns the
  parser. ``eof()`` returns the DOM backend's result: a
  ``std::optional<element>`` that holds the root of the tree.
* Always check ``last_error()`` before you use the result. If the parse failed, the
  result may be empty, or it may hold part of the document.
* :cpp:func:`get_object_element\<T>(key) <peejay::dom::element::get_object_element>`
  looks up a member of an object. It returns an empty ``std::optional`` if the element
  is not an object, the key is missing, or the value is not of type ``T``. Otherwise
  it returns a ``std::reference_wrapper`` to the value. Call ``get()`` to reach the value.
* :cpp:func:`get_if\<T>() <peejay::dom::element::get_if>` works like
  ``std::get_if``: it returns a pointer to the value if the element holds a ``T``,
  and ``nullptr`` otherwise.
* Strings are ``std::u8string``. The ``to_string()`` helper copies one into a
  ``std::string`` so that it can be written to ``std::cout``.
* With the default policy, integers are ``int`` and floating-point numbers are
  ``double``. The aliases ``element::integer_type`` and ``element::float_type`` always
  name the right types.

.. note::

   The DOM backend's default limits are deliberately small: at most 50 members in an
   array or object, strings of at most 64 bytes, and at most 8 levels of nesting.
   :doc:`howto/configure-policies` and :doc:`howto/work-with-the-dom` show how to
   raise them.

Write your own backend
----------------------

Building a DOM is convenient, but it allocates memory for every value and keeps the
whole document in memory. Often you can do the work directly in a backend instead.
This program counts the strings, numbers, and keys in a document, and measures how
deeply it nests:

.. literalinclude:: examples/tutorial_3_backend.cpp
   :language: cpp
   :start-after: SPDX-License-Identifier: MIT

Output:

.. code-block:: text

   strings: 2
   numbers: 2
   keys: 3
   max depth: 2

A backend must have:

* A nested type named ``policies`` that satisfies the :cpp:concept:`peejay::policy`
  concept. The parser uses it for its own configuration.
* One member function for each kind of event: ``string_value``, ``integer_value``,
  ``float_value``, ``boolean_value``, ``null_value``, ``begin_array``, ``end_array``,
  ``begin_object``, ``key``, and ``end_object``. Each returns a ``std::error_code``.
  Return an empty error code to continue the parse.
* A ``result()`` member function. ``eof()`` returns whatever ``result()`` returns.

The parser checks all of this at compile time with the :cpp:concept:`peejay::backend`
concept. If you leave out a function or give it the wrong signature, the compiler
names the requirement that is not met.

The parser default-constructs ``stats_backend`` and stores it inside itself. If you
need to reach a backend directly, for example to pass it arguments when it is
constructed or to examine it after the parse, see
:doc:`howto/use-a-backend-by-reference`.

Parse input that arrives in pieces
----------------------------------

So far, each program has passed the whole document to ``input()`` at once. The parser
works just as well when the document arrives in pieces. This program reads standard
input 256 bytes at a time:

.. literalinclude:: examples/tutorial_4_stream.cpp
   :language: cpp
   :start-after: SPDX-License-Identifier: MIT

Try it with valid and invalid input:

.. code-block:: console

   $ echo '{"a": [1, 2, 3]}' | ./build/stream
   valid JSON
   $ printf '{"a": [1,\n2, tru]}' | ./build/stream
   line 2: error: unrecognized token

The parser treats input split into many pieces exactly as it would the same text in
one piece. A piece can end anywhere, even in the middle of a string, a number, or a
multi-byte UTF-8 character.

The program also shows how to report where an error occurred. With the default
policy, the parser counts lines, and ``pos().line`` gives the line of the most recent
token: the token that caused the error. :doc:`howto/report-errors` describes this
in more detail.

Next steps
----------

* Browse the :doc:`how-to guides <howto/index>` for recipes that solve common problems.
* Read :doc:`reference/backends` for the exact order in which a backend's functions are
  called.
* Read :doc:`reference/policies` before you choose limits for production use.
