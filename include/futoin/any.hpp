//-----------------------------------------------------------------------------
//   Copyright 2018-2023 FutoIn Project
//   Copyright 2018-2023 Andrey Galkin
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

#ifndef FUTOIN_ANY_HPP
#define FUTOIN_ANY_HPP
//---

// Always use custom any due to extensions for binary interface
#if 1 /* __cplusplus < 201703L */
#    define FUTOIN_USING_OWN_ANY
#endif

#ifdef FUTOIN_USING_OWN_ANY
#    include <array>
#    include <iostream>
#    include <typeinfo>
#    ifdef __GNUC__
#        include <cxxabi.h>
#    endif
//---
#    include "./details/binarymove.hpp"
#    include "./imempool.hpp"
#    include "./string.hpp"
#else
#    include <any>
#endif

namespace futoin {
#ifdef FUTOIN_USING_OWN_ANY
    static inline futoin::string demangle(const std::type_info& ti)
    {
#    ifdef __GNUC__
        int status;
        char* name = abi::__cxa_demangle(ti.name(), nullptr, nullptr, &status);
        futoin::string ret{name};
        free(name);
        return ret;
#    else
        return ti.name();
#    endif
    }

    [[noreturn]] inline void throw_bad_cast(
            const std::type_info& src, const std::type_info& dst)
    {
        std::cerr << "[ERROR] bad any cast: " << demangle(src) << " -> "
                  << demangle(dst) << std::endl;
        throw std::bad_cast();
    }

    /**
     * @brief Fast partial implementation of C++17 std::any
     */
    class any
    {
    public:
        any() noexcept : type_info_(void_info_), control_(default_control) {}

        any(const any& other)
        {
            type_info_ = other.type_info_;
            (control_ = other.control_)(Copy, const_cast<any&>(other), *this);
        }

        any(any&& other) noexcept
        {
            type_info_ = other.type_info_;
            (control_ = other.control_)(Move, other, *this);
            other.type_info_ = void_info_;
            other.control_ = default_control;
        }

        template<
                class T,
                typename D = typename std::
                        enable_if<!std::is_same<T, any>::value, void>::type>
        // NOLINTNEXTLINE(misc-forwarding-reference-overload)
        explicit any(T&& v) noexcept : type_info_(&typeid(T))
        {
            using U = typename std::remove_cv<
                    typename std::remove_reference<T>::type>::type;
            Accessor<U>::set(*this, std::forward<U>(v));
        }

        template<
                class T,
                typename D = typename std::
                        enable_if<!std::is_same<T, any>::value, void>::type>
        explicit any(const T& v) noexcept : type_info_(&typeid(T))
        {
            using U = typename std::remove_cv<
                    typename std::remove_reference<T>::type>::type;
            Accessor<U>::set(*this, v);
        }

        ~any()
        {
            reset();
        }

        any& operator=(any&& other) noexcept
        {
            if (&other != this) {
                control_(Cleanup, *this, *this);
                type_info_ = other.type_info_;
                (control_ = other.control_)(Move, other, *this);
                other.type_info_ = void_info_;
                other.control_ = default_control;
            }

            return *this;
        }

        any& operator=(const any& other)
        {
            if (&other != this) {
                control_(Cleanup, *this, *this);
                type_info_ = other.type_info_;
                (control_ = other.control_)(
                        Copy, const_cast<any&>(other), *this);
            }

            return *this;
        }

        template<
                class T,
                typename std::enable_if<!std::is_same<T, any>::value, bool>::
                        type = true>
        any& operator=(T&& v) noexcept
        {
            using U = typename std::remove_cv<
                    typename std::remove_reference<T>::type>::type;

            control_(Cleanup, *this, *this);

            type_info_ = &typeid(U);
            Accessor<U>::set(*this, std::forward<U>(v));

            return *this;
        }

        template<
                class T,
                typename std::enable_if<!std::is_same<T, any>::value, bool>::
                        type = true>
        any& operator=(const T& v) noexcept
        {
            using U = typename std::remove_cv<
                    typename std::remove_reference<T>::type>::type;

            control_(Cleanup, *this, *this);

            type_info_ = &typeid(U);
            Accessor<U>::set(*this, v);

            return *this;
        }

        void reset() noexcept
        {
            control_(Cleanup, *this, *this);

            type_info_ = void_info_;
            control_ = default_control;
        }

        // void swap(any &other) noexcept {}

        bool has_value() const noexcept
        {
            return type_info_->hash_code() != void_info_->hash_code();
        }

        const std::type_info& type() const noexcept
        {
            return *type_info_;
        }

        void extract(FutoInBinaryValue& v)
        {
            control_(Extract, *this, reinterpret_cast<any&>(v));
        }

    private:
        enum ControlMode
        {
            Cleanup,
            Move,
            Copy,
            Extract,
        };
        using ControlHandler = void(ControlMode, any&, any&);

        const std::type_info* type_info_;
        ControlHandler* control_;
        // NOLINTNEXTLINE(readability-magic-numbers)
        std::array<void*, 8> data_;

        static constexpr const std::type_info* void_info_ = &typeid(void);
        // NOLINTNEXTLINE(readability-named-parameter)
        static void default_control(ControlMode, any&, any&) noexcept {}

        template<
                typename T,
                bool is_fundamental = std::is_fundamental<T>::value
                                      || std::is_pointer<T>::value,
                bool is_small = (sizeof(T) <= sizeof(data_))>
        struct Accessor
        {};

        template<class T>
        friend inline const T* any_cast(const any* that)
        {
            using U = typename std::remove_cv<
                    typename std::remove_reference<T>::type>::type;

            return Accessor<U>::get(*const_cast<any*>(that));
        }

        template<class T>
        friend inline T* any_cast(any* that)
        {
            using U = typename std::remove_cv<
                    typename std::remove_reference<T>::type>::type;

            return Accessor<U>::get(*that);
        }
    };

    namespace details {
        template<typename T>
        typename std::enable_if<!std::is_copy_constructible<T>::value, void>::
                type
                new_copy(void* p, const T& other)
        {
            (void) p;
            std::cerr << "[ERROR] no copy c-tor: " << demangle(typeid(other))
                      << std::endl;
            throw std::runtime_error("Missing copy c-tor");
        }

        template<typename T>
        typename std::enable_if<std::is_copy_constructible<T>::value, void>::
                type
                new_copy(void* p, const T& other)
        {
            new (p) T(other);
        }
    } // namespace details

    template<typename T>
    struct any::Accessor<T, true, true>
    {
        static inline void set(any& that, T v) noexcept
        {
            auto* p = reinterpret_cast<T*>((void*) that.data_.data());
            *p = v;

            that.control_ = [](ControlMode mode, any& that, any& other) {
                auto* p = reinterpret_cast<T*>((void*) that.data_.data());

                switch (mode) {
                case Cleanup: that.control_ = default_control; break;
                case Move:
                case Copy:
                    *reinterpret_cast<T*>((void*) other.data_.data()) = *p;
                    break;
                case Extract:
                    details::moveTo<T>(
                            reinterpret_cast<FutoInBinaryValue&>(other),
                            std::move(that),
                            std::move(*p));
                    break;
                }
            };
        }

        static T* get(any& that)
        {
            if (that.type_info_->hash_code() == typeid(T).hash_code()) {
                return reinterpret_cast<T*>((void*) that.data_.data());
            }

            throw_bad_cast(*(that.type_info_), typeid(T));
        }
    };

    template<typename T>
    struct any::Accessor<T, false, true>
    {
        static inline void set(any& that, T&& v) noexcept
        {
            new (that.data_.data()) T(std::forward<T>(v));

            that.control_ = [](ControlMode mode, any& that, any& other) {
                auto* p = reinterpret_cast<T*>(that.data_.data());

                switch (mode) {
                case Cleanup:
                    p->~T();
                    that.control_ = default_control;
                    break;
                case Move: new (other.data_.data()) T(std::move(*p)); break;
                case Copy:
                    details::new_copy(
                            other.data_.data(), static_cast<const T&>(*p));
                    break;
                case Extract:
                    details::moveTo<T>(
                            reinterpret_cast<FutoInBinaryValue&>(other),
                            std::move(that),
                            std::move(*p));
                    break;
                }
            };
        }

        static inline void set(any& that, const T& v) noexcept
        {
            set(that, T(v));
        }

        static T* get(any& that)
        {
            if (that.type_info_->hash_code() == typeid(T).hash_code()) {
                return reinterpret_cast<T*>(that.data_.data());
            }

            throw_bad_cast(*(that.type_info_), typeid(T));
        }
    };

    template<typename T>
    struct any::Accessor<T, false, false>
    {
        static inline void set(any& that, T&& v) noexcept
        {
            auto& mem_pool = GlobalMemPool::get_default();
            auto* p = mem_pool.allocate(sizeof(T), 1);
            new (p) T(std::forward<T>(v));
            that.data_[0] = p;
            that.data_[1] = &mem_pool;

            that.control_ = [](ControlMode mode, any& that, any& other) {
                auto* p = reinterpret_cast<T*>(that.data_[0]);
                auto* mem_pool = reinterpret_cast<IMemPool*>(that.data_[1]);

                switch (mode) {
                case Cleanup:
                    p->~T();
                    mem_pool->deallocate(p, sizeof(T), 1);
                    that.control_ = default_control;
                    break;
                case Move:
                    other.data_[0] = p;
                    other.data_[1] = mem_pool;
                    that.data_[0] = nullptr;
                    that.data_[1] = nullptr;
                    break;
                case Copy: {
                    auto* np = mem_pool->allocate(sizeof(T), 1);
                    details::new_copy(np, static_cast<const T&>(*p));
                    other.data_[0] = np;
                    other.data_[1] = mem_pool;
                } break;
                case Extract:
                    details::moveTo<T>(
                            reinterpret_cast<FutoInBinaryValue&>(other),
                            std::move(that),
                            std::move(*p));
                    break;
                }
            };
        }

        static inline void set(any& that, const T& v) noexcept
        {
            set(that, T(v));
        }

        static T* get(any& that)
        {
            if (that.type_info_->hash_code() == typeid(T).hash_code()) {
                return reinterpret_cast<T*>(that.data_[0]);
            }

            throw_bad_cast(*(that.type_info_), typeid(T));
        }
    };

    template<>
    struct any::Accessor<any, false, false>
    {
        template<typename A, typename B>
        // NOLINTNEXTLINE(readability-named-parameter)
        static inline void set(A&, B&&) noexcept
        {
            static_assert(!std::is_same<A, B>::value, "Invalid any in any");
        }
    };

    template<class T>
    inline T any_cast(const any& that)
    {
        using U = typename std::remove_cv<
                typename std::remove_reference<T>::type>::type;
        return static_cast<T>(*any_cast<U>(&that));
    }

    template<class T>
    inline T any_cast(any& that)
    {
        using U = typename std::remove_cv<
                typename std::remove_reference<T>::type>::type;
        return static_cast<T>(*any_cast<U>(&that));
    }

    template<class T>
    inline T any_cast(any&& that)
    {
        using U = typename std::remove_cv<
                typename std::remove_reference<T>::type>::type;
        return static_cast<T>(std::move(*any_cast<U>(&that)));
    }
#else
    using std::any;
    template<typename T>
    using any_cast = std::any_cast<T>;
#endif
} // namespace futoin

//---
#endif // FUTOIN_ANY_HPP
