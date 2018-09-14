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
//! @brief Helper for type-erased callbacks with use of NextArgs.
//-----------------------------------------------------------------------------

#ifndef FUTOIN_DETAILS_ERASED_FUNC_HPP
#define FUTOIN_DETAILS_ERASED_FUNC_HPP
//---
#include "./functor_pass.hpp"
#include "./nextargs.hpp"

namespace futoin {
    namespace details {
        template<
                size_t FunctorSize = functor_pass::DEFAULT_SIZE,
                size_t FuntorAlign = functor_pass::DEFAULT_ALIGN>
        class ErasedFunc
        {
        public:
            static constexpr size_t FUNCTOR_SIZE = FunctorSize;
            static constexpr size_t FUNCTOR_ALIGN = FuntorAlign;

            using Storage = functor_pass::StorageBase<FunctorSize, FuntorAlign>;
            using NextArgs = nextargs::NextArgs;
            using TestCast = void (*)(const NextArgs&);
            template<typename FP>
            using SimplePass = functor_pass::Simple<
                    FP,
                    FunctorSize,
                    functor_pass::Function,
                    FuntorAlign>;

            template<typename... A>
            ErasedFunc(SimplePass<void(A...)>&& pass) noexcept
            {
                new (impl_) TypedImpl<A...>(pass, storage_);
            }

            template<typename... A>
            ErasedFunc& operator=(SimplePass<void(A...)>&& pass) noexcept
            {
                impl().~Impl();
                new (impl_) TypedImpl<A...>(pass, storage_);
            }

            ErasedFunc() noexcept
            {
                new (impl_) InvalidImpl;
            }

            ~ErasedFunc() noexcept
            {
                impl().~Impl();
            }

            bool is_valid() const noexcept
            {
                return impl()->is_valid();
            }

            operator bool() const
            {
                return is_valid();
            }
            bool operator!() const
            {
                return !is_valid();
            }

            void repeatable(const NextArgs& args) const noexcept
            {
                impl().repeatable(args);
            }
            void operator()(const NextArgs& args) const noexcept
            {
                impl().repeatable(args);
            }

            TestCast test_cast() const noexcept
            {
                return impl().test_cast();
            }

        private:
            struct Impl
            {
                virtual ~Impl() noexcept = default;
                virtual bool is_valid() const noexcept = 0;
                virtual void repeatable(const NextArgs& args) const
                        noexcept = 0;
                virtual TestCast test_cast() const noexcept = 0;
            };

            struct InvalidImpl final : Impl
            {
                InvalidImpl() noexcept = default;

                bool is_valid() const noexcept override
                {
                    return false;
                }

                void repeatable(const NextArgs& /*args*/) const
                        noexcept override
                {
                    std::cerr << "FATAL: ErasedFunc::repeatable() with no "
                                 "callback!"
                              << std::endl;
                    std::terminate();
                }

                TestCast test_cast() const noexcept override
                {
                    std::cerr << "FATAL: ErasedFunc::test_cast() with no "
                                 "callback!"
                              << std::endl;
                    std::terminate();
                }
            };

            template<typename... A>
            struct TypedImpl final : Impl
            {
                using Pass = functor_pass::Simple<
                        void(A...),
                        FunctorSize,
                        functor_pass::Function,
                        FuntorAlign>;
                using Function = typename Pass::Function;

                TypedImpl(Pass& p, Storage& storage) noexcept
                {
                    p.move(func_, storage);
                }

                bool is_valid() const noexcept override
                {
                    return true;
                }

                void repeatable(const NextArgs& args) const noexcept override
                {
                    args.repeatable(func_);
                }

                TestCast test_cast() const noexcept override
                {
                    return [](const NextArgs& args) { args.test_cast<A...>(); };
                }

                Function func_;
            };

            Impl& impl()
            {
                return *reinterpret_cast<Impl*>(impl_);
            }

            const Impl& impl() const
            {
                return *reinterpret_cast<const Impl*>(impl_);
            }

            Storage storage_;
            alignas(TypedImpl<>) char impl_[sizeof(TypedImpl<>)];
        };
    } // namespace details
} // namespace futoin

//---
#endif // FUTOIN_DETAILS_ERASED_FUNC_HPP
