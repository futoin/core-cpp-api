//-----------------------------------------------------------------------------
//   Copyright 2018 FutoIn Project
//   Copyright 2018 Andrey Galkin
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//-----------------------------------------------------------------------------
//! @file
//! @brief Helper for C++17 std::any backward compatibility
//-----------------------------------------------------------------------------

#ifndef FUTOIN_DETAILS_ANY_HPP
#define FUTOIN_DETAILS_ANY_HPP
//---

#if __cplusplus < 201703L
#    define FUTOIN_USING_OWN_ANY
#endif

#ifdef FUTOIN_USING_OWN_ANY
#    include <array>
#    include <iostream>
#    include <typeinfo>
#    ifdef __GNUC__
#        include <cxxabi.h>
#    endif
#else
#    include <any>
#endif

namespace futoin {
    namespace details {
#ifdef FUTOIN_USING_OWN_ANY
        static inline std::string demangle(const std::type_info &ti) {
#    ifdef __GNUC__
            int status;
            char *name =
                abi::__cxa_demangle(ti.name(), nullptr, nullptr, &status);
            std::string ret{name};
            free(name);
            return ret;
#    else
            return ti.name();
#    endif
        }

        [[noreturn]] static inline void
        throw_bad_cast(const std::type_info &src, const std::type_info &dst) {
            std::cerr << "[FATAL] bad any cast: " << demangle(src) << " -> "
                      << demangle(dst) << std::endl;
            throw std::bad_cast();
        }

        class any {
          public:
            any() noexcept
                : type_info_(void_info_), control_(default_control) {}

            any(const any &other) = delete; // not this time

            any(any &&other) noexcept {
                type_info_ = other.type_info_;
                (control_ = other.control_)(Move, other, *this);
                other.type_info_ = void_info_;
                other.control_ = default_control;
            }

            template <class T>
            explicit any(T &&v) : type_info_(&typeid(T)) { // NOLINT
                using U = typename std::remove_cv<
                    typename std::remove_reference<T>::type>::type;
                Accessor<U>::set(*this, std::forward<U>(v));
            }

            ~any() {
                reset();
            }

            any &operator=(const any &rhs) = delete; // not this time

            any &operator=(any &&other) noexcept {
                if (&other != this) {
                    control_(Cleanup, *this, *this);
                    type_info_ = other.type_info_;
                    (control_ = other.control_)(Move, other, *this);
                    other.type_info_ = void_info_;
                    other.control_ = default_control;
                }

                return *this;
            }

            template <class T> any &operator=(T &&v) {
                using U = typename std::remove_cv<
                    typename std::remove_reference<T>::type>::type;

                control_(Cleanup, *this, *this);

                type_info_ = &typeid(U);
                Accessor<U>::set(*this, std::forward<U>(v));

                return *this;
            }

            void reset() noexcept {
                control_(Cleanup, *this, *this);

                type_info_ = void_info_;
                control_ = default_control;
            }

            // void swap(any &other) noexcept {}

            bool has_value() const noexcept {
                return type_info_->hash_code() != void_info_->hash_code();
            }

            const std::type_info &type() const noexcept {
                return *type_info_;
            }

          private:
            enum ControlMode { Cleanup, Move };
            using ControlHandler = void(ControlMode, any &, any &);

            const std::type_info *type_info_;
            ControlHandler *control_;
            std::array<void *, 8> data_;

            static constexpr const std::type_info *void_info_ = &typeid(void);
            static void default_control(ControlMode, any &, any &) {} // NOLINT

            template <
                typename T,
                bool is_fundamental =
                    std::is_fundamental<T>::value || std::is_pointer<T>::value,
                bool is_small = (sizeof(T) <= sizeof(data_))>
            struct Accessor {};

            template <class T>
            friend inline const T *any_cast(const any *that) noexcept {
                using U = typename std::remove_cv<
                    typename std::remove_reference<T>::type>::type;

                return Accessor<U>::get(*const_cast<any *>(that));
            }

            template <class T> friend inline T *any_cast(any *that) noexcept {
                using U = typename std::remove_cv<
                    typename std::remove_reference<T>::type>::type;

                return Accessor<U>::get(*that);
            }
        };

        template <typename T> struct any::Accessor<T, true, true> {
            static inline void set(any &that, T v) {
                auto p = reinterpret_cast<T *>((void *)that.data_.data());
                *p = v;

                that.control_ = [](ControlMode mode, any &that, any &other) {
                    auto p = reinterpret_cast<T *>((void *)that.data_.data());

                    switch (mode) {
                    case Cleanup:
                        that.control_ = default_control;
                        break;
                    case Move:
                        *reinterpret_cast<T *>((void *)other.data_.data()) = *p;
                        break;
                    }
                };
            }

            static T *get(any &that) {
                if (that.type_info_->hash_code() == typeid(T).hash_code()) {
                    return reinterpret_cast<T *>((void *)that.data_.data());
                }

                throw_bad_cast(*(that.type_info_), typeid(T));
            }
        };

        template <typename T> struct any::Accessor<T, false, true> {
            static inline void set(any &that, T &&v) {
                new (that.data_.data()) T(std::forward<T>(v));

                that.control_ = [](ControlMode mode, any &that, any &other) {
                    auto p = reinterpret_cast<T *>(that.data_.data());

                    switch (mode) {
                    case Cleanup:
                        p->~T();
                        that.control_ = default_control;
                        break;
                    case Move:
                        new (other.data_.data()) T(std::move(*p));
                        break;
                    }
                };
            }

            static T *get(any &that) {
                if (that.type_info_->hash_code() == typeid(T).hash_code()) {
                    return reinterpret_cast<T *>(that.data_.data());
                }

                throw_bad_cast(*(that.type_info_), typeid(T));
            }
        };

        template <typename T> struct any::Accessor<T, false, false> {
            static inline void set(any &that, T &&v) {
                auto p = new T(std::forward<T>(v));
                that.data_[0] = p;

                that.control_ = [](ControlMode mode, any &that, any &other) {
                    auto p = reinterpret_cast<T *>(that.data_[0]);

                    switch (mode) {
                    case Cleanup:
                        delete p;
                        that.control_ = default_control;
                        break;
                    case Move:
                        other.data_[0] = p;
                        that.data_[0] = nullptr;
                        break;
                    }
                };
            }

            static T *get(any &that) {
                if (that.type_info_->hash_code() == typeid(T).hash_code()) {
                    return reinterpret_cast<T *>(that.data_[0]);
                }

                throw_bad_cast(*(that.type_info_), typeid(T));
            }
        };

        template <> struct any::Accessor<any, false, false> {
            template <typename A, typename B>
            static inline void set(A &, B &&) { // NOLINT
                static_assert(!std::is_same<A, B>::value, "Invalid any in any");
            }
        };

        template <class T> inline T any_cast(const any &that) {
            using U = typename std::remove_cv<
                typename std::remove_reference<T>::type>::type;
            return static_cast<T>(*any_cast<U>(&that));
        }

        template <class T> inline T any_cast(any &that) {
            using U = typename std::remove_cv<
                typename std::remove_reference<T>::type>::type;
            return static_cast<T>(*any_cast<U>(&that));
        }

        template <class T> inline T any_cast(any &&that) {
            using U = typename std::remove_cv<
                typename std::remove_reference<T>::type>::type;
            return static_cast<T>(std::move(*any_cast<U>(&that)));
        }
#else
        using std::any;
        using std::any_cast;
#endif
    } // namespace details
} // namespace futoin

//---
#endif // FUTOIN_DETAILS_ANY_HPP
