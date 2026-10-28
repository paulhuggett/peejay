Parse text held as ``char``
===========================

``parser<>::input()`` accepts any input range whose elements are ``char8_t``, which is
the C++ type for UTF-8 code units. Text often arrives as ``std::string``,
``std::string_view``, or an array of ``char``, though. You don't need to copy it: a
``std::views::transform`` view converts each ``char`` as the parser reads it.

.. literalinclude:: ../examples/howto_char_input.cpp
   :language: cpp
   :start-after: SPDX-License-Identifier: MIT

The same approach works for ``unsigned char`` and ``std::byte``.

If you read the data yourself, you can read it straight into a ``char8_t`` buffer
instead. ``std::istream::read()`` takes a ``char*``, and because ``char`` may alias
any type, you can safely use ``reinterpret_cast<char*>`` on a ``char8_t`` array. The
:ref:`streaming example in the tutorial <tutorial:parse input that arrives in pieces>`
does this.

.. note::

   The conversion does not change the bytes, so the text must already be UTF-8.
   icubaby, which peejay already depends on, can validate and convert to UTF-8 from
   other Unicode encodings.
