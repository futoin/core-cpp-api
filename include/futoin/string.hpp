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

#include <string>

/**
 * @brief Main namespace for FutoIn project
 */
namespace futoin {
    /**
     * @brief Placeholder for heap synchronization optimized version
     */
    using string = std::string;

    /**
     * @brief Placeholder for heap synchronization optimized version
     */
    using u16string = std::u16string;

    /**
     * @brief Placeholder for heap synchronization optimized version
     */
    using u32string = std::u32string;
} // namespace futoin

//---
#endif // FUTOIN_STRING_HPP
