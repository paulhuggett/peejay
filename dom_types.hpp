/// \file dom_types.cpp

#ifndef JSON_DOM_TYPES_HPP
#define JSON_DOM_TYPES_HPP

#include <cassert>
#include <string>
#include <unordered_map>
#include <memory>
#include <ostream>
#include <stack>
#include <vector>

namespace json {
    namespace value {

        class boolean_value;
        class null_value;
        class number_long;
        class number_double;
        class string_value;
        class array_value;
        class object_value;

        class dom_element {
        public:
            virtual ~dom_element () noexcept;
            virtual std::ostream & write (std::ostream & os) const = 0;

            virtual boolean_value * as_boolean () noexcept { return nullptr; }
            virtual boolean_value const * as_boolean () const noexcept { return nullptr; }

            virtual null_value * as_null () noexcept { return nullptr; }
            virtual null_value const * as_null () const noexcept { return nullptr; }

            virtual number_long * as_long () noexcept { return nullptr; }
            virtual number_long const * as_long () const noexcept { return nullptr; }

            virtual number_double * as_double () noexcept { return nullptr; }
            virtual number_double const * as_double () const noexcept { return nullptr; }

            virtual string_value * as_string () noexcept { return nullptr; }
            virtual string_value const * as_string () const noexcept { return nullptr; }

            virtual array_value * as_array () noexcept { return nullptr; }
            virtual array_value const * as_array () const noexcept { return nullptr; }

            virtual object_value * as_object () noexcept { return nullptr; }
            virtual object_value const * as_object () const noexcept { return nullptr; }
        };

        inline std::ostream & operator<< (std::ostream & os, dom_element const & v) {
            return v.write (os);
        }

        class boolean_value final : public dom_element {
        public:
            explicit boolean_value (bool b) noexcept
                    : b_{b} {}
            bool operator== (boolean_value const & rhs) const { return b_ == rhs.b_; }
            bool operator!= (boolean_value const & rhs) const { return !(*this == rhs); }

            boolean_value * as_boolean () noexcept override { return this; }
            boolean_value const * as_boolean () const noexcept override { return this; }

            std::ostream & write (std::ostream & os) const override;
            bool get () const noexcept { return b_; }

        private:
            bool b_;
        };

        class null_value final : public dom_element {
        public:
            null_value () noexcept = default;
            ~null_value () noexcept override;
            bool operator== (null_value const &) const noexcept { return true; }
            bool operator!= (null_value const & rhs) const noexcept { return !(*this == rhs); }

            null_value * as_null () noexcept override { return this; }
            null_value const * as_null () const noexcept override { return this; }

            std::ostream & write (std::ostream & os) const override { return os << "null"; }
        };

        class number_base : public dom_element {
        public:
            ~number_base () noexcept override;
        };

        class number_long final : public number_base {
        public:
            explicit number_long (long value) noexcept
                    : value_{value} {}
            ~number_long () noexcept;
            bool operator== (number_long const & rhs) const noexcept {
                return value_ == rhs.value_;
            }
            bool operator!= (number_long const & rhs) const noexcept { return !(*this == rhs); }

            number_long * as_long () noexcept override { return this; }
            number_long const * as_long () const noexcept override { return this; }

            std::ostream & write (std::ostream & os) const override { return os << value_; }
            long get () const noexcept { return value_; }

        private:
            long value_;
        };

        class number_double final : public number_base {
        public:
            explicit number_double (double value) noexcept
                    : value_{value} {}
            ~number_double () noexcept;
            number_double * as_double () noexcept override { return this; }
            number_double const * as_double () const noexcept override { return this; }

            std::ostream & write (std::ostream & os) const override { return os << value_; }
            double get () const noexcept { return value_; }

        private:
            double value_;
        };

        class string_value final : public dom_element {
        public:
            explicit string_value (std::string value) noexcept
                    : value_{std::move (value)} {}
            ~string_value () noexcept;
            bool operator== (string_value const & rhs) const { return value_ == rhs.value_; }
            bool operator!= (string_value const & rhs) const { return !(*this == rhs); }

            string_value * as_string () noexcept override { return this; }
            string_value const * as_string () const noexcept override { return this; }

            std::string const & get () const noexcept { return value_; }
            std::size_t size () const noexcept { return value_.size (); }

            std::ostream & write (std::ostream & os) const override {
                return os << '"' << value_ << '"';
            }

        private:
            std::string const value_;
        };

        class array_value final : public dom_element {
        public:
            using element_ptr = std::shared_ptr<dom_element>;
            using container_type = std::vector<element_ptr>;

            array_value () noexcept {}
            explicit array_value (container_type v) noexcept
                    : v_{std::move (v)} {}

            void push_back (element_ptr v) { v_.push_back (v); }

            array_value * as_array () noexcept override { return this; }
            array_value const * as_array () const noexcept override { return this; }

            std::size_t size () const noexcept { return v_.size (); }

            element_ptr operator[] (std::size_t index) noexcept { return v_[index]; }

            std::ostream & write (std::ostream & os) const override;

        private:
            container_type v_;
        };

        class object_value : public dom_element {
            using element_ptr = std::shared_ptr<dom_element>;
            using container_type = std::unordered_map<std::string, element_ptr>;

        public:
            using const_iterator = container_type::const_iterator;

            object_value () noexcept {}

            object_value * as_object () noexcept override { return this; }
            object_value const * as_object () const noexcept override { return this; }

            std::ostream & write (std::ostream & os) const override;

            void insert (std::string key, element_ptr & value) {
                v_.emplace (std::move (key), value);
            }
            std::size_t size () const { return v_.size (); }
            const_iterator begin () const { return std::begin (v_); }
            const_iterator end () const { return std::end (v_); }
            const_iterator find (std::string const & key) const { return v_.find (key); }

        private:
            container_type v_;
        };

    } // namespace value

    class yaml_output {
    public:
        using result_type = std::shared_ptr<value::dom_element>;

        void string_value (std::string const & s) {
            out_.emplace (new class value::string_value (s));
        }
        void integer_value (long v) { out_.emplace (new value::number_long (v)); }
        void float_value (double v) { out_.emplace (new value::number_double (v)); }
        void boolean_value (bool v) { out_.emplace (new class value::boolean_value (v)); }
        void null_value () { out_.emplace (new class value::null_value ()); }

        void begin_array ();
        void end_array ();

        void begin_object ();
        void end_object ();

        std::shared_ptr<value::dom_element> result () const {
            assert (out_.size () == 1U);
            return out_.top ();
        }

    private:
        std::stack<result_type> out_;
    };
} // namespace json

#endif // JSON_DOM_TYPES_HPP
