Introduction
============

peejay is a JSON parser for C++20 programs. It is a header-only library that turns
UTF-8 text into a sequence of calls to an object you provide. This page explains how
it works and helps you decide whether it suits your program.

What peejay does
----------------

peejay reads one JSON value, as defined by `RFC 8259 <https://www.rfc-editor.org/rfc/rfc8259>`_,
and reports what it finds as it finds it. For this input:

.. code-block:: json

   {"name": "web", "ports": [80, 443]}

the parser makes these calls, in this order:

.. code-block:: text

   begin_object()
   key("name")
   string_value("web")
   key("ports")
   begin_array()
   integer_value(80)
   integer_value(443)
   end_array()
   end_object()

The object that receives these calls is called a **backend**. peejay comes with two
backends, and you can write your own:

* :ref:`peejay::dom::dom <reference/backends:the dom backend>` builds a tree of values
  (a *document object model*, or DOM) that you can examine after the parse.
* :ref:`peejay::null <reference/backends:the null backend>` ignores everything. Use it
  to check that a document is valid JSON.
* A backend you write can do whatever your program needs: fill in your own data
  structures, check the document against a schema, or transform it as it streams past.

Key ideas
---------

The parser
   :cpp:class:`peejay::parser` is a class template that takes the backend type as its
   template argument. You call ``input()`` with as many pieces of text as you like, then
   call ``eof()`` to say that the input is complete. ``eof()`` returns the backend's
   result.

Push parsing
   The parser keeps all of its state between calls to ``input()``. A string, number,
   or keyword can be split between two calls, even in the middle of a multi-byte
   character. So you can parse data as it arrives from a file, socket, or pipe without
   first collecting all of it.

Backends
   A backend is any type that satisfies the :cpp:concept:`peejay::backend` concept: it
   has one member function for each kind of event, and a ``result()`` function. Each
   event function returns a ``std::error_code``. Returning an error stops the parse.

Policies
   A **policy** is a type that sets the parser's compile-time options: the longest
   string it accepts, how deeply arrays and objects may nest, which integer and
   floating-point types it uses, and whether it counts lines. Each backend names its
   policy through a nested ``policies`` type. See :doc:`reference/policies`.

Errors
   The parser does not throw. When it finds a problem, it records a
   ``std::error_code``, ignores the rest of the input, and reports the error through
   ``has_error()`` and ``last_error()``.

Design goals
------------

Predictable resource use
   The parser front end never allocates memory. Its size is fixed when it is compiled
   and depends only on the policy: there is a buffer of ``max_length`` bytes for
   strings and a stack of ``max_stack_depth`` entries for nesting. Input that would
   exceed either limit is rejected with an error. It is never truncated, and it never
   causes the parser to grow. This makes peejay suitable for embedded systems, and
   for services that must not let a hostile document exhaust their memory.

No exceptions
   The parser does not throw or catch exceptions. (The bundled DOM backend uses
   standard containers, which can throw ``std::bad_alloc``.)

Correct numbers
   Integers are passed to the backend as integers. Other numbers are converted to the
   nearest floating-point value using exact arithmetic. When the parser cannot be
   certain of the correctly rounded result, it reports an error rather than guess.

Strict conformance
   peejay accepts only standard JSON. It does not accept comments, trailing commas,
   single-quoted strings, ``NaN``, or a byte-order mark.

When to use peejay
------------------

peejay is a good fit when:

* the input arrives incrementally, or is too large to hold in memory;
* you want to convert JSON straight into your own types without building an
  intermediate tree;
* you need hard limits on the memory and stack that a parse can use;
* your code is built without exceptions.

Consider another library when you need to *write* JSON, or when you want a general,
editable DOM with ordered objects. peejay's DOM is deliberately simple, and peejay does
not generate JSON.

Requirements
------------

* A C++20 compiler and standard library. peejay uses concepts, ranges, ``char8_t``,
  and ``std::u8string_view``. Continuous integration tests Clang 18, GCC 14, Apple
  Clang (macOS 15), and MSVC (Visual Studio 2022).
* `icubaby <https://github.com/paulhuggett/icubaby>`_, a small header-only Unicode
  library that peejay uses to decode ``\uXXXX`` escapes. The CMake build fetches it
  automatically. See :doc:`howto/add-to-project`.

Things to know
--------------

A few behaviours are worth knowing before you start. Each is covered in more detail in
the reference.

* The input must be UTF-8, supplied as ``char8_t``. peejay checks the parts of the
  input it interprets, such as escapes, control characters, and surrogates, but it
  does not check that every multi-byte sequence in a string is well-formed. See
  :doc:`howto/validate-utf8`.
* The strings passed to ``key()`` and ``string_value()`` point into the parser's
  internal buffer. They are valid only until the function returns, so copy them if
  you need to keep them.
* An error is permanent. Once a parser has reported an error, it ignores all further
  input. Use a new parser for the next document.
* A document holds exactly one top-level value. Anything after it other than
  whitespace is an error.
