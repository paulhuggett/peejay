API listing
===========

This page is generated from the comments in peejay's headers. The other reference
pages describe how the pieces work together. This page lists every public
declaration.

Parser
------

.. doxygenclass:: peejay::parser
   :members:

.. doxygenfunction:: peejay::make_parser

.. doxygenstruct:: peejay::coord
   :members:

.. doxygenstruct:: peejay::coord< true >
   :members:

Concepts and policies
---------------------

.. doxygenconcept:: peejay::backend

.. doxygenconcept:: peejay::policy

.. doxygenstruct:: peejay::default_policies
   :members:

.. doxygenstruct:: peejay::no_float_type

Errors
------

.. doxygenenum:: peejay::error

.. doxygenclass:: peejay::error_category
   :members:

Null backend
------------

.. doxygenclass:: peejay::null
   :members:

DOM backend
-----------

.. doxygenclass:: peejay::dom::dom
   :members:

.. doxygenclass:: peejay::dom::element
   :members:

.. doxygenstruct:: peejay::dom::null

.. doxygenconcept:: peejay::dom::dom_policies

.. doxygenstruct:: peejay::dom::default_dom_policies
   :members:

.. doxygenenum:: peejay::dom::dom_error

.. doxygenclass:: peejay::dom::dom_error_category
   :members:
