peejay
======

**peejay** (“PJ”, from **P**\ arse **J**\ SON) is a header-only C++20 JSON parser.
You give it UTF-8 text, in one piece or a byte at a time, and it calls functions on an
object you supply as it recognizes each value. The parser never allocates memory and
never throws exceptions, and every limit it enforces is fixed at compile time.

.. code-block:: cpp

   #include "peejay/dom.hpp"

   peejay::parser<peejay::dom::dom<>> p;
   auto root = p.input(u8R"({"name": "web", "port": 8080})"sv).eof();
   if (!p.has_error()) {
     int port = root->get_object_element<int>(u8"port")->get();  // 8080
   }

How this documentation is organized
-----------------------------------

:doc:`introduction`
   What peejay is, the ideas it is built on, and when to choose it.

:doc:`tutorial`
   A step-by-step walk-through: validate a document, build a tree of values, write
   your own backend, and parse input that arrives in pieces. Start here if you are new.

:doc:`howto/index`
   Short recipes for specific tasks, such as raising the limits, reporting errors
   with line numbers, or stopping the parse early.

:doc:`reference/index`
   Precise descriptions of every public type, function, concept, and error code.

.. toctree::
   :maxdepth: 2
   :hidden:

   introduction
   tutorial
   howto/index
   reference/index
