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
//! @brief Include all available native API
//-----------------------------------------------------------------------------

#ifndef FUTOIN_DETAILS_STRIP_FUNCTOR_CLASS_HPP
#define FUTOIN_DETAILS_STRIP_FUNCTOR_CLASS_HPP
//---

namespace futoin {
    namespace details {
        //---
        /**
         * @internal
         */
        template<typename FP>
        struct StripFunctorClassHelper
        {};

        template<typename R, typename... A>
        struct StripFunctorClassHelper<R(A...)>
        {
            using type = R(A...);
        };
        template<typename R, typename C, typename... A>
        struct StripFunctorClassHelper<R (C::*)(A...) const>
        {
            using type = R(A...);
        };

        template<typename R, typename C, typename... A>
        struct StripFunctorClassHelper<R (C::*)(A...)>
            : public StripFunctorClassHelper<R (C::*)(A...) const>
        {};

        /**
         * @internal
         */
        template<typename C>
        struct StripFunctorClass
            : public StripFunctorClassHelper<decltype(&C::operator())>
        {};

        template<typename R, typename... A>
        struct StripFunctorClass<R(A...)>
            : public StripFunctorClassHelper<R(A...)>
        {};
    } // namespace details
} // namespace futoin

//---
#endif // FUTOIN_DETAILS_STRIP_FUNCTOR_CLASS_HPP
