Parse text in an unknown encoding
=================================

JSON exchanged between systems should be UTF-8, but text from files or other programs
is sometimes UTF-16 or UTF-32, often with a byte order mark (BOM) at the start. If you
don't know in advance which encoding you have, pass the raw bytes to the parser with
icubaby's byte transcoder, ``icubaby::tx_8``. It reads the byte order mark, if there
is one, to choose the encoding, and converts the text to UTF-8 for the parser.

.. literalinclude:: ../examples/howto_bytes.cpp
   :language: cpp
   :start-after: SPDX-License-Identifier: MIT

Output:

.. code-block:: text

   OK
   OK
   OK
   OK
   OK
   ill-formed input

The byte transcoder recognizes these byte order marks. Without one, it assumes UTF-8.
The byte order mark is not passed to the parser.

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - Bytes
     - Encoding
   * - ``EF BB BF``
     - UTF-8
   * - ``FE FF``
     - UTF-16 BE
   * - ``FF FE``
     - UTF-16 LE
   * - ``00 00 FE FF``
     - UTF-32 BE
   * - ``FF FE 00 00``
     - UTF-32 LE

As in :doc:`validate-utf8`, the ``true`` passed to ``input()`` and ``eof()`` stops the
parse with ``peejay::error::ill_formed_input`` when the text is not well formed in the
chosen encoding, or ends part-way through a character. Pass ``false``, or leave the
argument out, to have each ill-formed sequence replaced by U+FFFD REPLACEMENT
CHARACTER instead.

The bytes can arrive in any number of pieces, even if a piece ends in the middle of
the byte order mark: pass the same transcoder to every call of ``input()`` and then to
``eof()``. Once the transcoder has chosen an encoding, it keeps it, even after
``eof()``. Use a new transcoder for each new source of text.
