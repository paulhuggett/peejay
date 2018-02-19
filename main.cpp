#include <array>
#include <fstream>
#include <iostream>

#include "dom_types.hpp"
#include "json.hpp"
#include "utf.hpp"

namespace {

    template <typename IStream>
    int slurp (IStream & in) {
        int exit_code = EXIT_SUCCESS;
        using ustreamsize = std::make_unsigned<std::streamsize>::type;
        std::array<char, 256> buffer{{0}};
        json::parser<json::yaml_output> p;

        while ((in.rdstate () &
                (std::ios_base::badbit | std::ios_base::failbit | std::ios_base::eofbit)) == 0) {
            in.read (&buffer[0], buffer.size ());
            p.parse (&buffer[0],
                     static_cast<ustreamsize> (std::max (in.gcount (), std::streamsize{0})));
        }

        p.eof ();

        auto err = p.last_error ();
        if (err) {
            std::cerr << "Error: " << p.last_error ().message () << '\n';
            exit_code = EXIT_FAILURE;
        } else {
            auto obj = p.callbacks ().result ();
            std::cout << "\n----\n" << *obj << '\n';
        }
        return exit_code;
    }

} // end anonymous namespace

int main (int argc, const char * argv[]) {
    int exit_code = EXIT_SUCCESS;
    try {
        if (argc < 2) {
            exit_code = slurp (std::cin);
        } else {
            std::ifstream input (argv[1]);
            exit_code = slurp (input);
        }
    } catch (std::exception const & ex) {
        std::cerr << "Error: " << ex.what () << '\n';
        exit_code = EXIT_FAILURE;
    } catch (...) {
        std::cerr << "Unknown exception.\n";
        exit_code = EXIT_FAILURE;
    }
    return exit_code;
}
