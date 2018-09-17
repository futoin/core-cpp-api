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
#include "../fatalmsg.hpp"
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
            static void test_cast_anchor(const NextArgs& args)
            {
                args.test_cast<A...>();
            }

            template<typename... A>
            ErasedFunc(SimplePass<void(A...)>&& pass) noexcept
            {
                new (&impl_) TypedImpl<A...>(pass, storage_);
            }

            template<typename... A>
            ErasedFunc& operator=(SimplePass<void(A...)>&& pass) noexcept
            {
                impl().~Impl();
                new (&impl_) TypedImpl<A...>(pass, storage_);
                return *this;
            }

            ErasedFunc() noexcept
            {
                new (&impl_) InvalidImpl;
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

            const NextArgs& model_args() const noexcept
            {
                return impl().model_args();
            }

        private:
            struct Impl
            {
                virtual ~Impl() noexcept = default;
                virtual bool is_valid() const noexcept = 0;
                virtual void repeatable(const NextArgs& args) const
                        noexcept = 0;
                virtual TestCast test_cast() const noexcept = 0;
                virtual const NextArgs& model_args() const noexcept = 0;
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
                    FatalMsg() << "ErasedFunc::repeatable() with no callback!";
                }

                TestCast test_cast() const noexcept override
                {
                    FatalMsg() << "ErasedFunc::test_cast() with no callback!";
                    return nullptr;
                }

                const NextArgs& model_args() const noexcept override
                {
                    FatalMsg() << "ErasedFunc::model_args() with no callback!";
                    return NextArgs::Model<>::instance;
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
                    return &ErasedFunc::test_cast_anchor<A...>;
                }

                const NextArgs& model_args() const noexcept override
                {
                    return NextArgs::Model<A...>::instance;
                }

                Function func_;
            };

            Impl& impl()
            {
                return *reinterpret_cast<Impl*>(&impl_);
            }

            const Impl& impl() const
            {
                return *reinterpret_cast<const Impl*>(&impl_);
            }

            Storage storage_;
            struct
            {
                alignas(TypedImpl<>) char buf[sizeof(TypedImpl<>)];
            } impl_;
        };
    } // namespace details
} // namespace futoin

//---
#endif // FUTOIN_DETAILS_ERASED_FUNC_HPP
