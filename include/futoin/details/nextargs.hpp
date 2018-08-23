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

                template<bool = false>
                static T&& it(RT& v)
                {
                    static_assert(
                            !std::is_pointer<RT>::value,
                            "Pointers are not allowed.");
                    return std::move(v);
                }

                template<bool = false>
                static T&& it(RT&& v)
                {
                    static_assert(
                            !std::is_pointer<RT>::value,
                            "Pointers are not allowed.");
                    return v;
                }

                static std::string it(const char* v)
                {
                    return v;
                }
                static std::u16string it(const char16_t* v)
                {
                    return v;
                }
                static std::u32string it(const char32_t* v)
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

                template<
                        typename A,
                        typename B = NoArg,
                        typename C = NoArg,
                        typename D = NoArg>
                inline void assign(
                        A&& a, B&& b = {}, C&& c = {}, D&& d = {}) noexcept
                {
                    any* p = data();
                    *p = any(smart_forward<A>::it(a));
                    *(++p) = any(smart_forward<B>::it(b));
                    *(++p) = any(smart_forward<C>::it(c));
                    *(++p) = any(smart_forward<D>::it(d));
                }

                template<typename A>
                inline void
                call(IAsyncSteps& asi,
                     const std::function<void(IAsyncSteps&, A)>& exec_handler)
                {
                    any* p = data();
                    exec_handler(
                            asi,
                            any_cast<typename std::remove_reference<A>::type&&>(
                                    *p));
                }
                template<typename A, typename B>
                inline void call(
                        IAsyncSteps& asi,
                        const std::function<void(IAsyncSteps&, A, B)>&
                                exec_handler)
                {
                    any* p = data();
                    exec_handler(
                            asi,
                            any_cast<typename std::remove_reference<A>::type&&>(
                                    *p),
                            any_cast<typename std::remove_reference<B>::type&&>(
                                    p + 1));
                }
                template<typename A, typename B, typename C>
                inline void call(
                        IAsyncSteps& asi,
                        const std::function<void(IAsyncSteps&, A, B, C)>&
                                exec_handler)
                {
                    any* p = data();
                    exec_handler(
                            asi,
                            any_cast<typename std::remove_reference<A>::type&&>(
                                    *p),
                            any_cast<typename std::remove_reference<B>::type&&>(
                                    *(p + 1)),
                            any_cast<typename std::remove_reference<C>::type&&>(
                                    *(p + 2)));
                }
                template<typename A, typename B, typename C, typename D>
                inline void call(
                        IAsyncSteps& asi,
                        const std::function<void(IAsyncSteps&, A, B, C, D)>&
                                exec_handler)
                {
                    any* p = data();
                    exec_handler(
                            asi,
                            any_cast<typename std::remove_reference<A>::type&&>(
                                    *p),
                            any_cast<typename std::remove_reference<B>::type&&>(
                                    *(p + 1)),
                            any_cast<typename std::remove_reference<C>::type&&>(
                                    *(p + 2)),
                            any_cast<typename std::remove_reference<D>::type&&>(
                                    *(p + 3)));
                }
            };
        } // namespace nextargs
    }     // namespace details
} // namespace futoin

//---
#endif // FUTOIN_DETAILS_NEXTARGS_HPP
