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
//! @brief AsyncSteps argument forward handling details
//-----------------------------------------------------------------------------

#ifndef FUTOIN_DETAILS_NEXTARGS_HPP
#define FUTOIN_DETAILS_NEXTARGS_HPP
//---
#include <array>
#include <functional>
//---
#include "../any.hpp"

namespace futoin {
    class IAsyncSteps;

    namespace details {
        namespace nextargs {
            //---
            constexpr auto MAX_NEXT_ARGS = 4;

            /**
             * @brief A special helper to wrap C-strings and objects passed by
             * pointer.
             */
            template<typename T>
            struct smart_forward
            {
                using RT = typename std::remove_reference<T>::type;

                template<typename = void>
                static T&& it(RT& v)
                {
                    static_assert(
                            !std::is_pointer<RT>::value,
                            "Pointers are not allowed.");
                    return std::move(v);
                }

                static futoin::string it(const char* v)
                {
                    return v;
                }
                static futoin::u16string it(const char16_t* v)
                {
                    return v;
                }
                static futoin::u32string it(const char32_t* v)
                {
                    return v;
                }
            };

            /**
             * @internal
             */
            struct NextArgs : public std::array<any, MAX_NEXT_ARGS>
            {
                struct NoArg
                {};

                NextArgs() noexcept = default;

                template<typename... T>
                NextArgs(T&&... args) noexcept
                {
                    assign(std::forward<T>(args)...);
                }

                template<
                        typename A = NoArg,
                        typename B = NoArg,
                        typename C = NoArg,
                        typename D = NoArg>
                inline void assign(
                        A&& a = {}, B&& b = {}, C&& c = {}, D&& d = {}) noexcept
                {
                    auto* p = data();
                    *p = any(smart_forward<A>::it(a));
                    *(p + 1) = any(smart_forward<B>::it(b));
                    *(p + 2) = any(smart_forward<C>::it(c));
                    *(p + 3) = any(smart_forward<D>::it(d));
                }

                // Moving calls
                //=============
                template<typename T>
                using argref = typename std::remove_reference<T>::type&&;

                template<template<typename> class Function>
                inline void once(
                        IAsyncSteps& asi,
                        const Function<void(IAsyncSteps&)>& exec_handler)
                {
                    exec_handler(asi);
                }

                template<typename A, template<typename> class Function>
                inline void once(
                        IAsyncSteps& asi,
                        const Function<void(IAsyncSteps&, A)>& exec_handler)
                {
                    auto* p = data();
                    exec_handler(asi, any_cast<argref<A>>(*p));
                }

                template<
                        typename A,
                        typename B,
                        template<typename>
                        class Function>
                inline void once(
                        IAsyncSteps& asi,
                        const Function<void(IAsyncSteps&, A, B)>& exec_handler)
                {
                    auto* p = data();
                    exec_handler(
                            asi,
                            any_cast<argref<A>>(*p),
                            any_cast<argref<B>>(*(p + 1)));
                }

                template<
                        typename A,
                        typename B,
                        typename C,
                        template<typename>
                        class Function>
                inline void
                once(IAsyncSteps& asi,
                     const Function<void(IAsyncSteps&, A, B, C)>& exec_handler)
                {
                    auto* p = data();
                    exec_handler(
                            asi,
                            any_cast<argref<A>>(*p),
                            any_cast<argref<B>>(*(p + 1)),
                            any_cast<argref<C>>(*(p + 2)));
                }

                template<
                        typename A,
                        typename B,
                        typename C,
                        typename D,
                        template<typename>
                        class Function>
                inline void once(
                        IAsyncSteps& asi,
                        const Function<void(IAsyncSteps&, A, B, C, D)>&
                                exec_handler)
                {
                    auto* p = data();
                    exec_handler(
                            asi,
                            any_cast<argref<A>>(*p),
                            any_cast<argref<B>>(*(p + 1)),
                            any_cast<argref<C>>(*(p + 2)),
                            any_cast<argref<D>>(*(p + 3)));
                }

                // Const calls
                //=============
                template<typename T>
                using cargref = const typename std::remove_reference<T>::type&;

                template<template<typename> class Function>
                inline void repeatable(
                        const Function<void()>& exec_handler) const
                {
                    exec_handler();
                }

                template<typename A, template<typename> class Function>
                inline void repeatable(
                        const Function<void(A)>& exec_handler) const
                {
                    auto* p = data();
                    exec_handler(any_cast<cargref<A>>(*p));
                }

                template<
                        typename A,
                        typename B,
                        template<typename>
                        class Function>
                inline void repeatable(
                        const Function<void(A, B)>& exec_handler) const
                {
                    auto* p = data();
                    exec_handler(
                            any_cast<cargref<A>>(*p),
                            any_cast<cargref<B>>(*(p + 1)));
                }

                template<
                        typename A,
                        typename B,
                        typename C,
                        template<typename>
                        class Function>
                inline void repeatable(
                        const Function<void(A, B, C)>& exec_handler) const
                {
                    auto* p = data();
                    exec_handler(
                            any_cast<cargref<A>>(*p),
                            any_cast<cargref<B>>(*(p + 1)),
                            any_cast<cargref<C>>(*(p + 2)));
                }

                template<
                        typename A,
                        typename B,
                        typename C,
                        typename D,
                        template<typename>
                        class Function>
                inline void repeatable(
                        const Function<void(A, B, C, D)>& exec_handler) const
                {
                    auto* p = data();
                    exec_handler(
                            any_cast<cargref<A>>(*p),
                            any_cast<cargref<B>>(*(p + 1)),
                            any_cast<cargref<C>>(*(p + 2)),
                            any_cast<cargref<D>>(*(p + 3)));
                }

                // Test casts
                //=============

                // NOTE: multiple functions are used to support partial checks!

                template<bool = true>
                inline void test_cast() const
                {}

                template<typename A>
                inline void test_cast() const
                {
                    auto* p = data();
                    any_cast<cargref<A>>(*p);
                }

                template<typename A, typename B>
                inline void test_cast() const
                {
                    auto* p = data();
                    any_cast<cargref<A>>(*p);
                    any_cast<cargref<B>>(*(p + 1));
                }

                template<typename A, typename B, typename C>
                inline void test_cast() const
                {
                    auto* p = data();
                    any_cast<cargref<A>>(*p);
                    any_cast<cargref<B>>(*(p + 1));
                    any_cast<cargref<C>>(*(p + 2));
                }

                template<typename A, typename B, typename C, typename D>
                inline void test_cast() const
                {
                    auto* p = data();
                    any_cast<cargref<A>>(*p);
                    any_cast<cargref<B>>(*(p + 1));
                    any_cast<cargref<C>>(*(p + 2));
                    any_cast<cargref<D>>(*(p + 3));
                }

                // Model steps for testing purposes
                //=================================

                template<typename T>
                using rem_constref = typename std::remove_const<
                        typename std::remove_reference<T>::type>::type;

                template<
                        typename A = NoArg,
                        typename B = NoArg,
                        typename C = NoArg,
                        typename D = NoArg>
                struct Model;
            };

            template<typename A, typename B, typename C, typename D>
            struct NextArgs::Model
            {
                static const NextArgs instance;
            };

            template<typename A, typename B, typename C, typename D>
            // NOLINTNEXTLINE(cert-err58-cpp)
            const NextArgs NextArgs::Model<A, B, C, D>::instance{
                    NextArgs::rem_constref<A>(),
                    NextArgs::rem_constref<B>(),
                    NextArgs::rem_constref<C>(),
                    NextArgs::rem_constref<D>()};
        } // namespace nextargs
    }     // namespace details
} // namespace futoin

//---
#endif // FUTOIN_DETAILS_NEXTARGS_HPP
