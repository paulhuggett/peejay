//===- unit_tests/harness.cpp ---------------------------------------------===//
//*  _                                     *
//* | |__   __ _ _ __ _ __   ___  ___ ___  *
//* | '_ \ / _` | '__| '_ \ / _ \/ __/ __| *
//* | | | | (_| | |  | | | |  __/\__ \__ \ *
//* |_| |_|\__,_|_|  |_| |_|\___||___/___/ *
//*                                        *
//===----------------------------------------------------------------------===//
// Copyright © 2025 Paul Bowen-Huggett
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// “Software”), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// SPDX-License-Identifier: MIT
//===----------------------------------------------------------------------===//
#include <algorithm>
#include <cstring>
#include <memory>

#if defined(_WIN32)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#if defined(_MSC_VER)
#include <crtdbg.h>
#endif
#endif

#include <gmock/gmock.h>

using testing::TestInfo;
using testing::TestPartResult;
using testing::UnitTest;

namespace {

bool loud_mode_enabled(int argc, char **argv) {
  if (argc < 2) {
    return false;
  }
  return std::any_of(&argv[1], argv + argc, [](char const *a) { return std::strcmp(a, "--loud") == 0; });
}

class quiet_listener : public testing::TestEventListener {
public:
  explicit quiet_listener(testing::TestEventListener *const listener) : listener_(listener) {}
  quiet_listener(quiet_listener const &) = delete;
  quiet_listener(quiet_listener &&) = delete;
  ~quiet_listener() override = default;

  quiet_listener &operator=(quiet_listener const &) = delete;
  quiet_listener &operator=(quiet_listener &&) = delete;

  void OnTestProgramStart(UnitTest const &test) override { listener_->OnTestProgramStart(test); }
  void OnTestIterationStart(UnitTest const &test, int iteration) override {
    listener_->OnTestIterationStart(test, iteration);
  }
  void OnEnvironmentsSetUpStart(UnitTest const &unit_test) override { (void)unit_test; }
  void OnEnvironmentsSetUpEnd(UnitTest const &unit_test) override { (void)unit_test; }
  void OnTestStart(TestInfo const &test_info) override { (void)test_info; }
  void OnTestPartResult(TestPartResult const &result) override { listener_->OnTestPartResult(result); }
  void OnTestEnd(TestInfo const &test_info) override {
    if (test_info.result()->Failed()) {
      listener_->OnTestEnd(test_info);
    }
  }
  void OnEnvironmentsTearDownStart(UnitTest const &unit_test) override { (void)unit_test; }
  void OnEnvironmentsTearDownEnd(UnitTest const &unit_test) override { (void)unit_test; }
  void OnTestIterationEnd(UnitTest const &test, int iteration) override {
    listener_->OnTestIterationEnd(test, iteration);
  }
  void OnTestProgramEnd(UnitTest const &test) override { listener_->OnTestProgramEnd(test); }

private:
  std::unique_ptr<TestEventListener> listener_;
};

}  // end anonymous namespace

int main(int argc, char **argv) {
  try {
    // Since Google Mock depends on Google Test, InitGoogleMock() is
    // also responsible for initializing Google Test. Therefore there's
    // no need for calling InitGoogleTest() separately.
    testing::InitGoogleMock(&argc, argv);

    // Unless the user enables "loud mode" by passing the appropriate switch, we
    // silence much of google test/mock's output so that we only see detailed
    // information about tests that fail.
    if (!loud_mode_enabled(argc, argv)) {
      // Remove the default listener
      auto &listeners = UnitTest::GetInstance()->listeners();
      auto *const default_printer = listeners.Release(listeners.default_result_printer());

      // Add our listener. By default everything is on (as when using the
      // default listener) but here we turn everything off so we only see the 3
      // lines for the result (plus any failures at the end), like:
      //
      // [==========] Running 149 tests from 53 test cases.
      // [==========] 149 tests from 53 test cases ran. (1 ms total)
      // [  PASSED  ] 149 tests.

      listeners.Append(new quiet_listener(default_printer));
    }

#if defined(_WIN32)
    // Disable all of the possible ways Windows conspires to make automated
    // testing impossible.
    ::SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
#if defined(_MSC_VER)
    ::_set_error_mode(_OUT_TO_STDERR);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif
#endif
    return RUN_ALL_TESTS();
  } catch (std::exception const &ex) {
    std::cerr << "Error: " << ex.what() << '\n';
  } catch (...) {
    std::cerr << "Unknown exception\n";
  }
  return EXIT_FAILURE;
}
