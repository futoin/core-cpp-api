//-----------------------------------------------------------------------------
//   Copyright 2020 FutoIn Project
//   Copyright 2020 Andrey Galkin
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
//! @brief FutoIn specification tools interface
//-----------------------------------------------------------------------------

#ifndef FUTOIN_ISPECTOOLS_HPP
#define FUTOIN_ISPECTOOLS_HPP
//---

#include "details/reqcpp11.hpp"
//---
#include "ieventemitter.hpp"
#include "imempool.hpp"
#include "ispec.hpp"
#include "string.hpp"
//---
#include <vector>
//---

namespace futoin {
    namespace spectools {
        /**
         * @brief Representation of spec repository & tools
         */
        class ISpecTools : virtual public IEventEmitter
        {
        public:
            using SpecDir = futoin::string;
            using SpecDirs = std::vector<SpecDir, IMemPool::Allocator<SpecDir>>;

            const EventType& EVT_ERROR{evt_error_};

            ISpecTools() = default;
            ISpecTools(const ISpecTools&) = delete;
            ISpecTools& operator=(const ISpecTools&) = delete;
            ISpecTools(ISpecTools&&) = delete;
            ISpecTools& operator=(ISpecTools&&) = delete;

            ~ISpecTools() noexcept override = default;

            virtual void add_spec(
                    IAsyncSteps& asi, const futoin::string& spec) = 0;
            virtual void load_spec(
                    IAsyncSteps& asi, const IfaceVer& iface_ver) = 0;
            virtual void load_spec(
                    IAsyncSteps& asi, const futoin::string& iface_ver) = 0;
            virtual IfaceVer parse_iface_ver(
                    const futoin::string& iface_ver) = 0;

            virtual const SpecDirs& spec_dirs() const = 0;
            virtual void add_spec_dirs(const SpecDir& path) = 0;

        protected:
            EventType evt_error_{"error"};
        };
    } // namespace spectools

    using ISpecTools = spectools::ISpecTools;
} // namespace futoin

//---
#endif // FUTOIN_ISPECTOOLS_HPP
