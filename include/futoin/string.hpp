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

#ifndef FUTOIN_STRING_HPP
#define FUTOIN_STRING_HPP
//---
#include "imempool.hpp"

#include <string>

namespace futoin {
    /**
     * @brief Placeholder for heap synchronization optimized version
     */
    using string = std::basic_string<
            char,
            std::char_traits<char>,
            IMemPool::Allocator<char>>;

    /**
     * @brief Placeholder for heap synchronization optimized version
     */
    using u16string = std::basic_string<
            char16_t,
            std::char_traits<char16_t>,
            IMemPool::Allocator<char>>;

    /**
     * @brief Placeholder for heap synchronization optimized version
     */
    using u32string = std::basic_string<
            char32_t,
            std::char_traits<char32_t>,
            IMemPool::Allocator<char>>;

    template<typename T>
    inline futoin::string key_from_pointer(T* ptr)
    {
        return {reinterpret_cast<char*>(&ptr), sizeof(ptr)};
    }
} // namespace futoin

//---
#endif // FUTOIN_STRING_HPP
