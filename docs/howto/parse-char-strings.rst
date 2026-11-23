Parse text held as ``char``
===========================

``parser<>::input()`` accepts any input range whose elements are ``char8_t``, which is
the C++ type for UTF-8 code units. Text often arrives as ``std::string``,
``std::string_view``, or an array of ``char``, though. You don't need to copy it: a
``std::views::transform`` view converts each ``char`` as the parser reads it.

.. literalinclude:: ../examples/howto_char_input.cpp
   :language: cpp
   :start-after: SPDX-License-Identifier: MIT

The same approach works for ``unsigned char`` and ``std::byte``. Alternatively,
pass a range of ``std::byte`` straight to ``input()`` together with an
``icubaby::tx_8`` transcoder. It checks the encoding too, and assumes UTF-8 unless the
bytes start with a byte order mark. See :doc:`parse-unknown-encoding`.

If you read the data yourself, you can read it straight into a ``char8_t`` buffer
instead. ``std::istream::read()`` takes a ``char*``, and because ``char`` may alias
any type, you can safely use ``reinterpret_cast<char*>`` on a ``char8_t`` array. The
:ref:`streaming example in the tutorial <tutorial:parse input that arrives in pieces>`
does this.

.. note::

   The conversion does not change the bytes, so the text must already be UTF-8. To
   check that it is, or to parse text in another Unicode encoding, pass an icubaby
   transcoder to ``input()`` and ``eof()`` along with the text. The library already
   depends on icubaby for Unicode conversion. See :doc:`validate-utf8` and
   :ref:`reference/parser:input`.
