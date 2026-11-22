Reject malformed UTF-8
======================

JSON text must be UTF-8. peejay checks the parts of a string that it interprets: it
rejects unescaped control characters, malformed ``\u`` escapes, and unpaired
surrogates. But ``input(range)`` copies every other byte of a string to the backend
unchanged, without checking that the bytes form valid UTF-8 sequences. So a string
such as ``"caf\xFF"`` reaches the backend as it is.

If your input comes from an untrusted source and the rest of your program needs valid
UTF-8, have the parser check it. Pass icubaby's UTF-8 to UTF-8 transcoder,
``icubaby::t8_8``, to ``input()`` along with the bytes, and pass the same transcoder to
``eof()``. icubaby is available to every program that uses peejay.

.. literalinclude:: ../examples/howto_utf8.cpp
   :language: cpp
   :start-after: SPDX-License-Identifier: MIT

Output:

.. code-block:: text

   OK
   ill-formed input
   ill-formed input

The ``true`` passed to both calls asks the parser to stop at the first ill-formed
sequence and record ``peejay::error::ill_formed_input``.

Finish with ``eof(utf8, true)``, not ``eof()``. When the input ends part-way through a
multi-byte sequence, the transcoder is still holding those bytes, and only
``eof(utf8, true)`` checks for them. In the last example, ``eof()`` alone would accept
the ``1``.

To keep going instead, pass ``false``, or leave the argument out. The transcoder then
replaces each ill-formed sequence with U+FFFD REPLACEMENT CHARACTER, so ``"caf\xFF"``
reaches the backend as ``caf`` followed by U+FFFD.

The input can arrive in any number of pieces. Pass the same transcoder to every call
of ``input()`` for a document, as the ``pj2-dump`` program in ``main.cpp`` in the
repository does. The same overloads also accept UTF-16, UTF-32, and raw bytes: see
:ref:`reference/parser:input`.
