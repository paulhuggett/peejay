Add peejay to your project
==========================

peejay is header-only. To use it, your compiler needs the peejay ``include``
directory and the include directory of `icubaby <https://github.com/paulhuggett/icubaby>`_,
peejay's one dependency. Both libraries must be compiled as C++20 or later.

With CMake ``FetchContent``
---------------------------

This is the easiest way. CMake downloads peejay and icubaby when the project is
configured:

.. code-block:: cmake

   cmake_minimum_required(VERSION 3.22)
   project(my_app LANGUAGES CXX)

   include(FetchContent)
   set(PEEJAY_UNITTEST No)   # Don't build peejay's own unit tests.
   FetchContent_Declare(
     peejay
     GIT_REPOSITORY https://github.com/paulhuggett/peejay.git
     GIT_TAG main   # Better: a specific commit hash, for reproducible builds.
   )
   FetchContent_MakeAvailable(peejay)

   add_executable(my_app main.cpp)
   target_compile_features(my_app PRIVATE cxx_std_20)
   target_link_libraries(my_app PRIVATE peejay::peejay)

The ``peejay::peejay`` target carries the include paths for both libraries.

Setting ``PEEJAY_UNITTEST`` to ``No`` before ``FetchContent_MakeAvailable()`` stops
peejay from downloading Google Test and building its unit tests as part of your project.
:ref:`howto/add-to-project:cmake options` lists the other settings.

With a Git submodule
--------------------

If you keep your dependencies in your own repository, add peejay as a submodule and
include it with ``add_subdirectory()``:

.. code-block:: console

   $ git submodule add https://github.com/paulhuggett/peejay.git third_party/peejay

.. code-block:: cmake

   set(PEEJAY_UNITTEST No)   # Don't build peejay's own unit tests.
   add_subdirectory(third_party/peejay)
   target_link_libraries(my_app PRIVATE peejay::peejay)

peejay's own ``CMakeLists.txt`` still fetches icubaby with ``FetchContent``. To use a
copy of icubaby that you already have, set ``FETCHCONTENT_SOURCE_DIR_ICUBABY`` to its
directory when you configure.

As an installed package
-----------------------

You can install peejay once and use it from any number of projects. From a clone
of the peejay repository, build the library and install it, along with icubaby:

.. code-block:: console

   $ cmake -S . -B build -D CMAKE_BUILD_TYPE=Release -D PEEJAY_UNITTEST=No
   $ cmake --build build --target peejay
   $ cmake --install build --prefix /opt/peejay

Your project then finds it with ``find_package()``:

.. code-block:: cmake

   find_package(peejay 2.0 REQUIRED)
   target_link_libraries(my_app PRIVATE peejay::peejay)

If the install prefix is not one that CMake searches by default, tell CMake where
it is when you configure your project, for example with
``-D CMAKE_PREFIX_PATH=/opt/peejay``.

The package accepts a request for any 2.x version. Linking to ``peejay::peejay``
also brings in icubaby's installed headers.

CMake options
-------------

peejay's ``CMakeLists.txt`` has these options. Set them with ``-D`` when you configure
peejay's own build, as in ``cmake -S . -B build -D PEEJAY_UNITTEST=No``, or with
``set()`` before ``FetchContent_MakeAvailable()`` or ``add_subdirectory()``.

.. list-table::
   :header-rows: 1
   :widths: 30 12 58

   * - Option
     - Default
     - Effect
   * - ``PEEJAY_UNITTEST``
     - ``Yes``
     - Builds the unit tests, and downloads Google Test to do so. Also enables
       ``ctest``, which runs the unit tests and the documentation's example programs.
       Set it to ``No`` when you only want the library: then neither peejay's nor
       icubaby's unit tests are built.
   * - ``PEEJAY_FUZZTEST``
     - ``No``
     - Builds the fuzz tests, and downloads `FuzzTest <https://github.com/google/fuzztest>`_
       to do so. Requires ``PEEJAY_UNITTEST``.
   * - ``PEEJAY_WERROR``
     - ``No``
     - Treats compiler warnings as errors in peejay's own targets.
   * - ``PEEJAY_SANITIZE``
     - ``No``
     - Builds peejay's own targets with AddressSanitizer and UndefinedBehaviorSanitizer.
       GCC and Clang only.
   * - ``PEEJAY_COVERAGE``
     - ``No``
     - Builds peejay's own targets with code coverage instrumentation. GCC and Clang
       only.
   * - ``PEEJAY_MPROTECT_VARIANT``
     - ``No``
     - Defines ``PEEJAY_MPROTECT_VARIANT=1`` for ``peejay::peejay`` and everything that
       links to it. In builds without ``NDEBUG`` on Linux, macOS, and Windows, the
       parser's internal state is then kept in memory pages that are protected while
       they are unused, so a stray access crashes at once. It is a debugging aid and
       makes the parser slower.
   * - ``PEEJAY_KLEE_DUMMY``
     - ``No``
     - Compiles the `KLEE <https://klee-se.org>`_ test sources against a stand-in for
       KLEE's header, to check that they build. When KLEE itself is found, the real KLEE
       targets are built instead, and this option has no effect.

Only ``PEEJAY_MPROTECT_VARIANT`` changes how your own code is compiled. The others
affect only peejay's own targets.

Without CMake
-------------

Add both include directories to your compiler's search path and select C++20:

.. code-block:: console

   $ c++ -std=c++20 -I peejay/include -I icubaby/include main.cpp -o my_app

Which header to include
-----------------------

Include only the header that provides what you use. Each one includes the parser.

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Header
     - Provides
   * - ``peejay/json.hpp``
     - :cpp:class:`peejay::parser`, the concepts, the policies, and the error codes.
       Include this when you write your own backend.
   * - ``peejay/null.hpp``
     - The :cpp:class:`peejay::null` backend, for validation.
   * - ``peejay/dom.hpp``
     - The :cpp:class:`peejay::dom::dom` backend and :cpp:class:`peejay::dom::element`.
   * - ``peejay/rules/non_terminal_rule.hpp``
     - All of the :doc:`rules <../reference/rules>`: the context, the rule base class, the
       terminal rules, ``object_rule``, ``homogeneous_array_rule``, and ``enum_rule``.
   * - ``peejay/rules/terminals.hpp``
     - The context, the rule base class, and the terminal rules only. Include this
       when your own rules don't use the others.
