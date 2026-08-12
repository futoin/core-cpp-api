//-----------------------------------------------------------------------------
// Copyright 2018-2026 FutoIn Project (https://futoin.org)
// Copyright 2018-2026 Andrey Galkin <andrey@futoin.org>
//
// Licensed under the FutoIn Public License 1.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://specs.futoin.org/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
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
            IMemPool::Allocator<char16_t>>;

    /**
     * @brief Placeholder for heap synchronization optimized version
     */
    using u32string = std::basic_string<
            char32_t,
            std::char_traits<char32_t>,
            IMemPool::Allocator<char32_t>>;

    template<typename T>
    inline futoin::string key_from_pointer(T* ptr)
    {
        return {reinterpret_cast<char*>(&ptr), sizeof(ptr)};
    }

    inline std::string to_std(const futoin::string& o)
    {
        return {o.data(), o.size()};
    }

    inline futoin::string to_futoin(const std::string& o)
    {
        return {o.data(), o.size()};
    }
} // namespace futoin

//---
#endif // FUTOIN_STRING_HPP
