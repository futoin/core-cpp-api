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
//! @brief AsyncSteps loop control details
//-----------------------------------------------------------------------------

#ifndef FUTOIN_DETAILS_ASYNCLOOP_HPP
#define FUTOIN_DETAILS_ASYNCLOOP_HPP
//---
#include <functional>
//---
#include "../any.hpp"
#include "../errors.hpp"
//---

namespace futoin {
    class IAsyncSteps;

    namespace details {
        namespace asyncloop {
            struct LoopState;

            using LoopLabel = const char*;
            using LoopHandler = std::function<void(LoopState&, IAsyncSteps&)>;
            using LoopCondition = std::function<bool(LoopState&)>;

            //---
            struct LoopState
            {
                LoopState(
                        LoopLabel label,
                        LoopHandler&& handler,
                        LoopCondition&& cond = {},
                        any&& data = {}) :
                    i(0),
                    label(label), handler(handler), cond(cond),
                    data(std::forward<any>(data))
                {}

                LoopState() = default;
                LoopState(LoopState&&) = default;
                LoopState& operator=(LoopState&&) = default;

                LoopState(const LoopState&) = delete;
                LoopState& operator=(const LoopState&) = delete;

                std::size_t i;
                LoopLabel label;
                LoopHandler handler;
                LoopCondition cond;
                any data;
            };

            //---
            class LoopBreak : public Error
            {
            public:
                LoopBreak(LoopLabel label) :
                    Error(errors::LoopBreak), label_(label)
                {}

                inline LoopLabel label()
                {
                    return label_;
                }

            private:
                LoopLabel label_;
            };

            //---
            class LoopContinue : public Error
            {
            public:
                LoopContinue(LoopLabel label) :
                    Error(errors::LoopCont), label_(label)
                {}

                inline LoopLabel label()
                {
                    return label_;
                }

            private:
                LoopLabel label_;
            };
        } // namespace asyncloop
    }     // namespace details
} // namespace futoin

//---
#endif // FUTOIN_DETAILS_ASYNCLOOP_HPP
