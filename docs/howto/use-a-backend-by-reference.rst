Use a backend that lives outside the parser
===========================================

By default, a parser contains its backend: ``peejay::parser<my_backend>`` either
default-constructs a ``my_backend`` or copies or moves one that you pass to its
constructor. Sometimes you want the parser to use an object that you already own
instead. For example, the object might gather results that you read after the parse,
or it might be too expensive to copy.

Pass the object to :cpp:func:`peejay::make_parser` as an lvalue. The result is a
``parser<my_backend&>``, which holds a reference to your object:

.. literalinclude:: ../examples/howto_reference.cpp
   :language: cpp
   :start-after: SPDX-License-Identifier: MIT

Output:

.. code-block:: text

   id
   tags
   colour
   size

The parser does not own the backend, so the backend must outlive the parser. If you
pass an rvalue to ``make_parser()`` instead, the parser holds its own copy, moved from
your object.

Other ways to reach a backend
-----------------------------

* Construct the parser from a backend object: ``peejay::parser p{my_backend{args}};``.
  Class template argument deduction gives you a ``parser<my_backend>``.
* Call ``p.backend()`` to get a reference to the backend inside a parser.

Inheriting from ``peejay::null``
--------------------------------

The example derives ``key_collector`` from :cpp:class:`peejay::null`. ``null``
provides every function that a backend needs, together with the ``policies`` type and
a ``result()`` function. Each one does nothing. A class derived from it needs to
define only the functions it cares about. Because a derived class's function hides
the base class's function of the same name, there is no need for ``virtual``.
