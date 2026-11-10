Reference
=========

These pages describe peejay's public interface precisely. Everything is in namespace
``peejay``, the DOM is in namespace ``peejay::dom``, and the rules are in namespace
``peejay::rules``. The types and functions in any namespace named ``details`` are
implementation details. They may change without notice, and you should not use them.
(``peejay::arrayvec`` is public, although its header is in the ``details`` directory.)

.. toctree::
   :maxdepth: 2

   parser
   backends
   rules
   policies
   errors
   input-format
   api
