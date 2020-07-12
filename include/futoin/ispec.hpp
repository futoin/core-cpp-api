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
//! @brief FutoIn interface specification
//-----------------------------------------------------------------------------

#ifndef FUTOIN_ISPEC_HPP
#define FUTOIN_ISPEC_HPP
//---

#include "details/reqcpp11.hpp"
//---
#include <cstdint>
#include <limits>
#include <map>
#include <regex>
#include <set>
#include <vector>
//---
#include "any.hpp"
#include "iasyncsteps.hpp"
#include "imempool.hpp"
#include "string.hpp"
//---

namespace futoin {
    namespace spectools {
        using Desc = futoin::string;
        using Iface = futoin::string;

        class Version
        {
        public:
            using VersionPart = uint32_t;
            static constexpr VersionPart DV =
                    std::numeric_limits<VersionPart>::max();

            Version() = default;
            Version(const Version&) = default;
            Version(Version&&) = default;
            ~Version() noexcept = default;
            Version& operator=(const Version&) = default;
            Version& operator=(Version&&) = default;

            Version(VersionPart major, VersionPart minor) noexcept :
                major_(major), minor_(minor)
            {}

            VersionPart majorPart() const noexcept
            {
                return major_;
            }
            VersionPart minorPart() const noexcept
            {
                return minor_;
            }

            bool operator==(const Version& o) const
            {
                return (major_ == o.major_) && (minor_ == o.minor_);
            }
            bool operator!=(const Version& o) const
            {
                return (major_ != o.major_) || (minor_ != o.minor_);
            }
            bool operator<(const Version& o) const
            {
                return (major_ < o.major_)
                       || ((major_ == o.major_) && (minor_ < o.minor_));
            }
            bool operator>(const Version& o) const
            {
                return (major_ > o.major_)
                       || ((major_ == o.major_) && (minor_ > o.minor_));
            }
            bool operator<=(const Version& o) const
            {
                return (major_ < o.major_)
                       || ((major_ == o.major_) && (minor_ <= o.minor_));
            }
            bool operator>=(const Version& o) const
            {
                return (major_ > o.major_)
                       || ((major_ == o.major_) && (minor_ >= o.minor_));
            }

        protected:
            VersionPart major_{0};
            VersionPart minor_{DV};
        };

        using FTN3Rev = Version;

        class Type
        {
        public:
            using Name = futoin::string;
            using Length = uint16_t;
            using RegEx = std::regex;

            struct Field
            {
                using Name = futoin::string;

                Type::Name type;
                bool optional;
                Desc desc;
            };

            using ReferenceFields = std::map<Field::Name, Field>;
            using Fields = std::map<
                    Field::Name,
                    Field,
                    ReferenceFields::key_compare,
                    IMemPool::Allocator<ReferenceFields::value_type>>;

            using Items =
                    std::vector<futoin::any, IMemPool::Allocator<futoin::any>>;

            Name type;
            Desc desc;

            Length min{0};
            Length max{std::numeric_limits<Length>::max()};
            Length minlen{0};
            Length maxlen{std::numeric_limits<Length>::max()};

            Name elemtype;
            RegEx regex;
            Fields fields;
            Items items;
        };

        struct Function
        {
            using Name = futoin::string;
            using Exception = futoin::string;
            using Throws =
                    std::vector<Exception, IMemPool::Allocator<Exception>>;
            using Bytes = uint32_t;
            using SecurityLevel = futoin::string;

            struct Param
            {
                using Name = futoin::string;

                Name name;
                Type::Name type;
                futoin::any defaultVal;
                Desc desc;
            };

            using Params = std::vector<Param, IMemPool::Allocator<Param>>;

            struct ResultVar
            {
                using Name = futoin::string;

                Name name;
                Type::Name type;
                Desc desc;
            };

            using ResultVars =
                    std::vector<ResultVar, IMemPool::Allocator<ResultVar>>;

            static constexpr Bytes DEFAULT_MSG_SIZE = 64 * 1024;

            bool rawupload{false};
            bool rawresult{false};
            bool heavy{false};

            Params params;
            ResultVars result;
            Type::Name result_single;

            Throws throws;
            Bytes maxreqsize{DEFAULT_MSG_SIZE};
            Bytes maxrspsize{DEFAULT_MSG_SIZE};
            SecurityLevel seclvl;
            Desc desc;

            bool is_single_result() const noexcept
            {
                return !result_single.empty();
            }
        };

        using ReferenceTypes = std::map<Type::Name, Type>;
        using Types = std::map<
                Type::Name,
                Type,
                ReferenceTypes::key_compare,
                IMemPool::Allocator<ReferenceTypes::value_type>>;

        using ReferenceFunctions = std::map<Function::Name, Function>;
        using Functions = std::map<
                Function::Name,
                Function,
                ReferenceFunctions::key_compare,
                IMemPool::Allocator<ReferenceFunctions::value_type>>;

        class IfaceVer
        {
        public:
            IfaceVer() = default;
            IfaceVer(const IfaceVer&) = default;
            IfaceVer(IfaceVer&&) = default;
            ~IfaceVer() noexcept = default;
            IfaceVer& operator=(const IfaceVer&) = default;
            IfaceVer& operator=(IfaceVer&&) = default;

            IfaceVer(Iface iface, Version ver) :
                iface_(std::move(iface)), ver_(ver)
            {}

            const Iface& iface() const noexcept
            {
                return iface_;
            }
            const Version& ver() const noexcept
            {
                return ver_;
            }

        protected:
            Iface iface_;
            Version ver_;
        };

        using ImportList = std::vector<IfaceVer, IMemPool::Allocator<IfaceVer>>;

        using Requirement = futoin::string;
        using ReferenceRequirementList = std::set<Requirement>;
        using RequirementList = std::set<
                Requirement,
                ReferenceRequirementList::key_type,
                IMemPool::Allocator<ReferenceRequirementList::value_type>>;

        class ISpecTools;

        /**
         * @brief Representation of FTN3 specification
         */
        class ISpec
        {
        public:
            ISpec() = default;
            ISpec(const ISpec&) = delete;
            ISpec& operator=(const ISpec&) = delete;
            ISpec(ISpec&&) noexcept = default;
            ISpec& operator=(ISpec&&) noexcept = default;

            virtual ~ISpec() noexcept = default;

            virtual ISpecTools& spectools() const noexcept = 0;
            virtual const futoin::string& orig_json() const = 0;
            virtual futoin::string build_json() const = 0;
            virtual futoin::string build_name() const = 0;
            virtual futoin::string build_filename() const = 0;

            /**
             * @name Info API directly corresponding FTN3 fields
             */
            ///@{
            virtual const Iface& iface() const noexcept = 0;
            virtual const Version& version() const noexcept = 0;
            virtual const FTN3Rev& ftn3rev() const noexcept = 0;
            virtual const Types& types() const noexcept = 0;
            virtual const Functions& funcs() const noexcept = 0;
            virtual const IfaceVer& inherit() const noexcept = 0;
            virtual const ImportList& imports() const noexcept = 0;
            virtual const Desc& desc() const noexcept = 0;
            virtual const RequirementList& requires() const noexcept = 0;
            ///@}
        };
    } // namespace spectools

    using ISpec = spectools::ISpec;
} // namespace futoin

//---
#endif // FUTOIN_ISPEC_HPP
