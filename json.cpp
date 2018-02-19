#include "json.hpp"

namespace json {

    maybe<char> source::pull () {
        if (lookahead_.has_value ()) {
            auto result = *lookahead_;
            lookahead_.reset ();
            return {result};
        }
        if (first_ == last_) {
            return nothing<char> ();
        }
        return just (*(first_++));
    }

} // namespace json
