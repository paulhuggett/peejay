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

With a Git submodule
--------------------

If you keep your dependencies in your own repository, add peejay as a submodule and
include it with ``add_subdirectory()``:

.. code-block:: console

   $ git submodule add https://github.com/paulhuggett/peejay.git third_party/peejay

.. code-block:: cmake

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

   $ cmake -S . -B build -D CMAKE_BUILD_TYPE=Release
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
