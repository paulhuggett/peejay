peejay
======

**peejay** (“PJ”, from **P**\ arse **J**\ SON) is a header-only C++20 JSON parser.
You give it UTF-8 text, in one piece or a byte at a time, and it calls functions on an
object you supply as it recognizes each value. It can build a tree of values for you, or
read a document straight into your own types. The parser never allocates memory and
never throws exceptions, and every limit it enforces is fixed at compile time.

This program reads a small document into a struct. Nothing in it allocates memory: the
parser and the context, which holds the stack of rules, are ordinary local variables.

.. literalinclude:: examples/home.cpp
   :language: cpp
   :start-after: SPDX-License-Identifier: MIT

How this documentation is organized
-----------------------------------

:doc:`introduction`
   What peejay is, the ideas it is built on, and when to choose it.

:doc:`tutorial`
   A step-by-step walk-through: validate a document, build a tree of values, write
   your own backend, parse input that arrives in pieces, and read a document into your
   own types. Start here if you are new.

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
