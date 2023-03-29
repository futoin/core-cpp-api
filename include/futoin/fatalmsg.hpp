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
//! @brief Fatal termination message object.
//-----------------------------------------------------------------------------

#ifndef FUTOIN_FATALMSG_HPP
#define FUTOIN_FATALMSG_HPP
//---
#include <exception>
#include <iostream>

namespace futoin {
    template<bool>
    struct FatalMsgHookT
    {
    public:
        using Terminate = void (*)();

        static void stream(std::ostream& s) noexcept
        {
            stream_ = &s;
        }

        static std::ostream& stream() noexcept
        {
            return *stream_;
        }

    private:
        static std::ostream* stream_;
    };

    template<bool B>
    std::ostream* FatalMsgHookT<B>::stream_{&std::cerr};

    using FatalMsgHook = FatalMsgHookT<true>;

    /**
     * @brief A common helper for FATAL messages.
     */
    struct FatalMsg final
    {
        FatalMsg() noexcept
        {
            stream() << std::endl << std::endl << "FATAL: ";
        }

        [[noreturn]] ~FatalMsg() noexcept
        {
            stream() << std::endl;
            std::terminate();
        }

        // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
        std::ostream& stream() noexcept
        {
            return FatalMsgHook::stream();
        }

        operator std::ostream&() const noexcept
        {
            return FatalMsgHook::stream();
        }

        template<typename T>
        std::ostream& operator<<(T&& arg)
        {
            return stream() << arg;
        }

        template<typename T>
        std::ostream& operator<<(const T& arg)
        {
            return stream() << arg;
        }
    };
} // namespace futoin

//---
#endif // FUTOIN_FATALMSG_HPP
