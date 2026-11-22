The parser
==========

.. code-block:: cpp

   #include "peejay/json.hpp"

   namespace peejay {
     template <backend Backend> class parser;
     template <backend Backend> decltype(auto) make_parser(Backend&& backend);
     template <bool Enabled> struct coord;
   }

``parser``
----------

``template <backend Backend> class parser``

   A push parser for JSON text. You supply input with ``input()``, as many times as
   necessary, and then call ``eof()``. As the parser recognizes each part of the
   input, it calls the corresponding member function of its backend.

   ``Backend`` is a type that satisfies the :cpp:concept:`peejay::backend` concept,
   or an lvalue reference to such a type. When it is a reference, the parser holds a
   reference to an object that you own, and that object must outlive the parser.

Member types
^^^^^^^^^^^^

``policies``
   ``std::remove_reference_t<Backend>::policies``: the policy that configures the
   parser. See :doc:`policies`.

Construction
^^^^^^^^^^^^

``parser()``
   Constructs the backend from ``Backend{}``.

``explicit parser(Backend& backend)``
   Copies ``backend``, or refers to it if ``Backend`` is a reference type.

``explicit parser(Backend&& backend)``
   Moves ``backend`` into the parser.

``template <typename... Args> explicit parser(Args&&... args)``
   *Requires:* ``Backend`` is not a reference type, ``args`` is not empty, and
   ``std::constructible_from<Backend, Args...>`` is true.

   Constructs the backend from ``std::forward<Args>(args)...``. For example, a parser
   for a rules context can be written ``parser<rules::backend<policies>> p{&ctx};``.
   A single argument that is a parser, or a non-const ``Backend``, selects one of the
   constructors above instead.

A parser can be moved. It can be copied only if it holds its backend by value and the
backend can be copied. A copy continues from the same point in the input. A parser that
holds a reference to its backend can be move-constructed, but it can't be copied or
assigned. The DOM backend can't be copied, so neither can a parser that holds one.

Deduction guides let you write ``peejay::parser p{my_backend{}};``.

Input
^^^^^

``template <std::ranges::input_range Range> parser& input(Range const& range)``
   *Requires:* ``std::ranges::range_value_t<Range>`` is ``char8_t``.

   Passes each code unit in ``range`` to the parser in turn. A token may span any
   number of calls, and a call may end anywhere, even in the middle of a multi-byte
   character.

   If the parser has already recorded an error, ``input()`` does nothing. If an
   error occurs part-way through ``range``, the rest of ``range`` is ignored.

   *Returns:* ``*this``, so that calls can be chained.

``template <typename Transcoder, std::ranges::input_range Range> parser& input(Transcoder& transcoder, Range const& range, bool stop_on_ill_formed = false)``
   *Requires:* ``Transcoder`` is an `icubaby <https://github.com/paulhuggett/icubaby>`_
   transcoder whose ``output_type`` is ``char8_t``, such as ``icubaby::t8_8``,
   ``icubaby::t16_8``, ``icubaby::t32_8``, or ``icubaby::tx_8``.
   ``std::ranges::range_value_t<Range>`` is the transcoder's ``input_type``.

   Passes each code unit in ``range`` to ``transcoder``, and parses the UTF-8 that it
   produces as ``input(range)`` does. Use this to parse UTF-16 or UTF-32 text, or to check
   that UTF-8 text is well formed. Pass the same transcoder to every call for a
   document, so that a character can be split between calls.

   When ``range`` holds a sequence that is not well formed, the transcoder's
   ``well_formed()`` becomes ``false``. What happens next depends on
   ``stop_on_ill_formed``:

   * ``true``: the parser records ``peejay::error::ill_formed_input`` and ignores the
     rest of ``range``. None of the output from the code unit that revealed the problem
     reaches the parser.
   * ``false``: the transcoder replaces the sequence with U+FFFD REPLACEMENT CHARACTER
     and parsing continues. The replacement character is valid only inside a string,
     so anywhere else the parser reports a JSON error such as ``expected_token``.

   A transcoder's ``well_formed()`` stays ``false`` once it has seen ill-formed input.
   So if an earlier call was made with ``stop_on_ill_formed`` set to ``false``, a later
   call with it set to ``true`` stops at its first code unit.

   If the parser has already recorded an error, this overload does nothing.

   *Returns:* ``*this``, so that calls can be chained.

   This overload can't tell when the input has ended, so it can't detect a
   character left incomplete at the end. Finish with ``eof(transcoder,
   stop_on_ill_formed)``, described below, rather than ``eof()``.

``decltype(auto) eof()``
   Tells the parser that the input is complete. The parser finishes the token it is
   reading, if any, and checks that the input held exactly one complete value. If
   it did not, an error is recorded.

   ``eof()`` then makes the parser ready for another document and calls the backend's
   ``result()`` function. It does this whether or not there was an error.

   *Returns:* whatever ``backend().result()`` returns.

   .. important::

      ``eof()`` does not clear a recorded error. A parser that has failed stays
      failed. Calling ``input()`` again after a *successful* parse starts a new
      document. The backend is not reset, and the line count continues from where the
      previous document ended. To start completely afresh, call ``reset()``.

``template <typename Transcoder> decltype(auto) eof(Transcoder& transcoder, bool stop_on_ill_formed = false)``
   *Requires:* ``Transcoder`` is an icubaby transcoder whose ``output_type`` is
   ``char8_t``.

   Use this in place of ``eof()`` after ``input(transcoder, range, stop_on_ill_formed)``,
   passing the same transcoder. It calls the transcoder's ``end_cp()``, which detects input that ends
   part-way through a character, and then does everything that ``eof()`` does.

   If the input ends part-way through a character, the transcoder's ``well_formed()``
   becomes ``false``. If ``stop_on_ill_formed`` is ``true``, the parser records
   ``peejay::error::ill_formed_input``. If it is ``false``, the U+FFFD REPLACEMENT
   CHARACTER that the transcoder produces is parsed, as ``input()`` would. So
   ``1`` followed by the first byte of a two-byte UTF-8 sequence gives
   ``unexpected_extra_input``, where ``eof()`` alone would accept the ``1``.

   As with ``input()``, if ``stop_on_ill_formed`` is ``true``, the error is
   recorded whenever the transcoder has seen ill-formed input, not only at the end.

   ``end_cp()`` is called even if the parser has already recorded an error, so the
   transcoder is always ready for another document. The parser's error is not changed.

   *Returns:* whatever ``backend().result()`` returns.

Starting again
^^^^^^^^^^^^^^

``template <typename... Args> parser& reset(Args&&... args)``
   *Requires:* ``Backend`` is not a reference type, ``std::constructible_from<Backend,
   Args...>`` is true, and ``Backend`` can be move-assigned.

   Puts the parser back into the state of a newly constructed one. Any document that
   was part-way through is discarded, the recorded error is cleared, and the line count
   returns to 1. The backend is replaced by ``Backend(std::forward<Args>(args)...)``, so
   ``reset()`` with no arguments gives a default-constructed backend. A single argument
   that is a ``Backend`` selects one of the overloads below instead.

   The new backend is constructed before anything else changes. If its constructor
   throws, the parser is unchanged.

   *Returns:* ``*this``, so that calls can be chained, as in
   ``p.reset().input(text).eof()``.

``parser& reset(Backend const& backend)``, ``parser& reset(Backend&& backend)``
   *Requires:* ``Backend`` is not a reference type, and can be copy-assigned (for the
   first) or move-assigned (for the second).

   As ``reset(args...)``, but the backend is assigned a copy of ``backend``, or has
   ``backend`` moved into it. The assignment happens before anything else changes. If it
   throws, the rest of the parser is unchanged, and the backend is in whatever state its
   assignment operator leaves it.

   *Returns:* ``*this``.

``parser& reset()``
   *Requires:* ``Backend`` is a reference type.

   Puts the parser back into the state of a newly constructed one, as above, but keeps
   the backend. The parser still refers to the same object, and that object is not
   changed. If the backend needs to start again too, reset it yourself.

   *Returns:* ``*this``.

.. note::

   A parser made by a rules context's ``make_parser()`` holds a backend that refers to
   the context. Resetting the parser doesn't reset the context: after a parse has ended,
   ``p.reset(&ctx)`` alone gives a parser that reports ``no active rule``. To parse
   another document, reset the context as well, as in ``ctx.reset(&value)`` followed by
   ``p.reset(&ctx)``, or pass a new context to ``p.reset()``. See :ref:`rules-context`.

Errors
^^^^^^

``bool has_error() const noexcept``
   *Returns:* ``true`` if an error has been recorded.

``std::error_code const& last_error() const noexcept``
   *Returns:* the recorded error, or an empty ``std::error_code`` if there is none.
   Errors detected by the parser have the category of :cpp:enum:`peejay::error`.
   Errors returned by the backend keep their own category. See :doc:`errors`.

Backend access
^^^^^^^^^^^^^^

``Backend& backend() & noexcept``, ``Backend const& backend() const& noexcept``
   *Returns:* a reference to the backend.

``Backend&& backend() && noexcept``
   *Returns:* an rvalue reference to the backend, so that it can be moved out of a
   parser that is about to be destroyed.

Position
^^^^^^^^

``coord<policies::pos_tracking> pos() const noexcept``
   *Returns:* the position at which the most recent token started. After an error,
   this is normally the position of the token that caused it.

``coord<policies::pos_tracking> input_pos() const noexcept``
   *Returns:* the position that the parser has reached in the input.

``make_parser``
---------------

``template <backend Backend> decltype(auto) make_parser(Backend&& backend)``
   *Returns:* ``parser<Backend>(std::forward<Backend>(backend))``. Because of the way
   forwarding references deduce their type, an lvalue argument gives a
   ``parser<T&>`` that refers to your object, and an rvalue argument gives a
   ``parser<T>`` that owns a moved copy of it.

``coord``
---------

``template <bool Enabled> struct coord``
   A position in the input. When ``Enabled`` is ``false``, ``coord`` is an empty
   type, and it takes no space inside the parser.

``coord<true>``
   ``unsigned line``
      The line number, starting at 1.

   ``coord`` objects can be compared with ``<=>`` and ``==``, and written to a
   ``std::ostream`` in the form ``(line)``. When ``<format>`` is available,
   ``to_string()`` returns the same text as a ``std::string``.

Thread safety
-------------

A parser has no shared state. You can use different parser objects in different
threads at the same time. You must not use the same parser object from more than one
thread at a time.
