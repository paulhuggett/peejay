Report errors with line numbers
===============================

After ``eof()``, ``last_error()`` holds the first error the parser found, or an empty
``std::error_code`` if the parse succeeded. When the policy's ``pos_tracking`` member
is ``true`` (the default), ``pos()`` gives the line on which the most recent token
started. That is usually the token that caused the error.

.. literalinclude:: ../examples/howto_errors.cpp
   :language: cpp
   :start-after: SPDX-License-Identifier: MIT

Output:

.. code-block:: text

   good.json: OK
   comma.json:2: error: expected token
   deep.json:1: error: objects are too deeply nested
     (the input exceeded one of the parser's policy limits)

Details
-------

* Lines are numbered from 1. A line ends at a line feed (LF), a carriage return (CR),
  or a CR LF pair.
* ``pos()`` is the position where the most recent token started. ``input_pos()`` is
  the position the parser has reached in the input. They differ when line breaks
  follow the most recent token.
* When ``pos_tracking`` is ``false``, ``pos()`` returns an empty
  :cpp:struct:`peejay::coord` with no ``line`` member, so code that reads the line
  does not compile.
* The parser reports only the first error. After an error, it ignores all further
  input and ``eof()`` does not clear the error, so use a new parser for the next
  document.
* Errors raised by the parser compare equal to values of the
  :cpp:enum:`peejay::error` enumeration. An error returned by a backend keeps its
  own category: for example, the DOM backend's
  :cpp:enum:`peejay::dom::dom_error`.

:doc:`../reference/errors` lists every error code and an example input for each.
