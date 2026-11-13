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

.. doxygenconcept:: peejay::policies

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

Rules
-----

.. doxygenclass:: peejay::rules::context
   :members:

.. doxygenclass:: peejay::rules::context_base
   :members:

.. doxygenclass:: peejay::rules::backend
   :members:

.. doxygenclass:: peejay::rules::rule
   :members:
   :protected-members:

.. doxygenstruct:: peejay::rules::base_children
   :members:

.. doxygenclass:: peejay::rules::bool_rule
   :members:

.. doxygenclass:: peejay::rules::integer_rule
   :members:

.. doxygentypedef:: peejay::rules::uinteger_rule

.. doxygenclass:: peejay::rules::float_rule
   :members:

.. doxygenclass:: peejay::rules::string_rule
   :members:

.. doxygenstruct:: peejay::rules::type_to_terminal
   :members:

.. doxygenclass:: peejay::rules::object_rule
   :members:
   :protected-members:

.. doxygenclass:: peejay::rules::homogeneous_array_rule
   :members:

.. doxygenclass:: peejay::rules::enum_rule
   :members:
   :protected-members:

.. doxygentypedef:: peejay::rules::builtin_integers

.. doxygentypedef:: peejay::rules::builtin_rules

.. doxygenconcept:: peejay::rules::integer_target

.. doxygenconcept:: peejay::rules::string_target

.. doxygenconcept:: peejay::rules::enum_target

.. doxygenconcept:: peejay::rules::emplace_back_container

.. doxygenconcept:: peejay::rules::max_size_container

.. doxygenconcept:: peejay::rules::is_rule

.. doxygenconcept:: peejay::rules::user_rule

.. doxygenenum:: peejay::rules::error

.. doxygenclass:: peejay::rules::error_category
   :members:

Containers
----------

.. doxygenclass:: peejay::arrayvec

The members of ``arrayvec`` follow those of ``std::vector``. See
:ref:`rules-arrayvec`.
