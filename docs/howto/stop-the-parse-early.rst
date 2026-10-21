Stop the parse early
====================

A backend can stop the parse at any point by returning a non-empty
``std::error_code`` from one of its functions. The parser records that error code,
stops calling the backend, and ignores the rest of the input. You can use this to
reject a document that is valid JSON but wrong for your program, or to stop as soon
as you have found what you need.

This program reads the top-level ``"version"`` property of a document and stops
there. The rest of the input is not examined, so it doesn't matter that the input is
incomplete:

.. literalinclude:: ../examples/howto_stop_early.cpp
   :language: cpp
   :start-after: SPDX-License-Identifier: MIT

Output:

.. code-block:: text

   version 3

How it works
------------

* ``search::status`` is an enumeration that is registered with the standard library as
  an error code enumeration. ``make_error_code()`` and the
  ``std::is_error_code_enum`` specialization let a ``search::status`` value be returned
  wherever a ``std::error_code`` is expected. This is the standard
  ``<system_error>`` pattern, and peejay's own :cpp:enum:`peejay::error` works the
  same way.
* When the backend returns ``search::status::found``, ``last_error()`` returns that
  code. The program compares it with ``search::status::found`` to tell a successful
  search from a real error.
* ``eof()`` still calls ``result()``, so the backend's result is available whether the
  parse stopped early or ran to the end.

If you only need to report a problem and don't need a custom category,
``std::make_error_code(std::errc::invalid_argument)`` or a similar standard code works
too.
