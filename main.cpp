#include <array>
#include <fstream>
#include <iostream>

#include "json/json.hpp"
#include "json/utf.hpp"

namespace {

class json_writer {
public:
  explicit json_writer (std::ostream& os) : os_{os} {}

  using result_type = void;
  result_type result () {}

  std::error_code string_value (std::string_view const& s) {
    os_ << '"';
    std::copy (std::begin (s), std::end (s),
               std::ostream_iterator<char>{os_, ""});
    os_ << '"';
    return {};
  }

  std::error_code int64_value (std::int64_t v) {
    os_ << v;
    return {};
  }
  std::error_code uint64_value (std::uint64_t v) {
    os_ << v;
    return {};
  }
  std::error_code double_value (double v) {
    os_ << v;
    return {};
  }
  std::error_code boolean_value (bool v) {
    os_ << (v ? "true" : "false");
    return {};
  }
  std::error_code null_value () {
    os_ << "null";
    return {};
  }

  std::error_code begin_array () {
    os_ << '[';
    return {};
  }
  std::error_code end_array () {
    os_ << ']';
    return {};
  }

  std::error_code begin_object () {
    os_ << '{';
    return {};
  }
  std::error_code key (std::string_view const& s) {
    return this->string_value (s);
  }
  std::error_code end_object () {
    os_ << '}';
    return {};
  }

private:
  std::ostream& os_;
};

template <typename IStream>
int slurp (IStream& in) {
  int exit_code = EXIT_SUCCESS;

        using ustreamsize = std::make_unsigned<std::streamsize>::type;
        std::array<char, 256> buffer{{0}};
        json::parser<json_writer> p{json_writer{std::cout}};

        while ((in.rdstate () &
                (std::ios_base::badbit | std::ios_base::failbit | std::ios_base::eofbit)) == 0) {
            in.read (&buffer[0], buffer.size ());
            p.input (std::span<char>{
                &buffer[0], static_cast<ustreamsize> (
                                std::max (in.gcount (), std::streamsize{0}))});
        }

        p.eof ();

        auto err = p.last_error ();
        if (err) {
            std::cerr << "Error: " << p.last_error ().message () << '\n';
            exit_code = EXIT_FAILURE;
        }

  return exit_code;
}

}  // end anonymous namespace

int main (int argc, const char* argv[]) {
  int exit_code = EXIT_SUCCESS;
  try {
    if (argc < 2) {
      exit_code = slurp (std::cin);
    } else {
      std::ifstream input (argv[1]);
      exit_code = slurp (input);
    }
  } catch (std::exception const& ex) {
    std::cerr << "Error: " << ex.what () << '\n';
    exit_code = EXIT_FAILURE;
  } catch (...) {
    std::cerr << "Unknown exception.\n";
    exit_code = EXIT_FAILURE;
  }
  return exit_code;
}
