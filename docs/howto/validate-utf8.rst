Reject malformed UTF-8
======================

JSON text must be UTF-8. peejay checks the parts of a string that it interprets: it
rejects unescaped control characters, malformed ``\u`` escapes, and unpaired
surrogates. But it copies every other byte of a string to the backend unchanged,
without checking that the bytes form valid UTF-8 sequences. So a string such as
``"caf\xFF"`` reaches the backend as it is.

If your input comes from an untrusted source and the rest of your program needs valid
UTF-8, check the bytes before they reach the parser. icubaby's UTF-8 to UTF-8
transcoder, ``icubaby::t8_8``, does this. It is available to every program that uses
peejay.

The simplest way is to pass the transcoder to the parser along with the bytes, as
``p.input(utf8, bytes)``, and to finish with ``p.eof(utf8)``. These overloads can either
stop with ``peejay::error::ill_formed_input`` or replace each bad sequence with U+FFFD and
carry on. See :ref:`reference/parser:input`. The rest of this page shows how to do the same
check yourself.

.. literalinclude:: ../examples/howto_utf8.cpp
   :language: cpp
   :start-after: SPDX-License-Identifier: MIT

Output:

.. code-block:: text

   OK
   ill-formed UTF-8

The transcoder's output is the same as its input for well-formed text, so the program
discards it and passes the original bytes to the parser. The array that receives the
output must still be large enough for the most that one byte can produce, which is
``icubaby::max_output_bytes``. When the input is ill-formed, a single byte can produce
U+FFFD REPLACEMENT CHARACTER as well as a code point. ``partial()`` reports input
that ends part-way through a multi-byte sequence.

For simplicity, the example passes one byte at a time to both objects. To work in
larger blocks, find the length of the well-formed prefix of each block and pass that
prefix to ``input()``, or use the overloads that take the transcoder, as the
``pj2-dump`` program in ``main.cpp`` in the repository does.
