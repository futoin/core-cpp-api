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

#ifndef FUTOIN_DETAILS_FUNCTOR_PASS_HPP
#define FUTOIN_DETAILS_FUNCTOR_PASS_HPP
//---
#include <functional>
//---
#include "./strip_functor_class.hpp"

namespace futoin {
    class IAsyncSteps;

    namespace details {
        namespace functor_pass {
            /**
             * @brief Storage for functors behind std::function
             *        to avoid expensive heap operations.
             */
            template<size_t S>
            struct StorageBase
            {
                StorageBase() = default;
                StorageBase(const StorageBase&) = delete;
                StorageBase& operator=(const StorageBase&) = delete;
                StorageBase(StorageBase&&) = delete;
                StorageBase& operator=(StorageBase&&) = delete;

                ~StorageBase() noexcept
                {
                    if (cleanup != nullptr) {
                        cleanup(&buffer);
                        cleanup = nullptr;
                    }
                }

                using CleanupCB = void (*)(void* buf);

                CleanupCB cleanup{nullptr};

                static constexpr auto BUFFER_ALIGN_AS = sizeof(std::ptrdiff_t);
                alignas(BUFFER_ALIGN_AS) std::uint8_t buffer[S];
            };

            /**
             * @brief Universal parameter definition to handle different
             *        use cases.
             * @note It must never be used outside of function parameter
             * definition!
             * @note It depend on lifetime of related function parameter value
             * during call.
             */
            template<typename FP, size_t S = sizeof(std::ptrdiff_t) * 4>
            class Simple
            {
            public:
                using Function = std::function<FP>;
                using Storage = StorageBase<S>;

                Simple(const Simple&) noexcept = default;
                Simple& operator=(const Simple&) noexcept = default;
                Simple(Simple&&) noexcept = default;
                Simple& operator=(Simple&&) noexcept = default;

                Simple() :
                    move_cb_([](void* /*ptr*/,
                                Function& /*func*/,
                                Storage& /*storage*/) {})
                {}

                // Bare Function Pointer
                Simple(FP* fp) :
                    ptr_(fp), move_cb_([](void* ptr,
                                          Function& func,
                                          Storage& /*storage*/) {
                        auto fp = reinterpret_cast<FP*>(ptr);
                        func = fp;
                    })
                {}

                // std::function for move
                // NOLINTNEXTLINE(misc-forwarding-reference-overload)
                Simple(Function&& f) :
                    ptr_(&f), move_cb_([](void* ptr,
                                          Function& func,
                                          Storage& /*storage*/) {
                        auto f = reinterpret_cast<Function*>(ptr);
                        func = std::move(*f);
                    })
                {}

                // std::function for copy
                Simple(const Function& f) :
                    ptr_(&const_cast<Function&>(f)),
                    move_cb_([](void* ptr,
                                Function& func,
                                Storage& /*storage*/) {
                        auto f = reinterpret_cast<const Function*>(ptr);
                        func = *f;
                    })
                {}

                template<
                        typename Functor,
                        typename = decltype(&Functor::operator()),
                        typename = typename std::enable_if<std::is_same<
                                FP,
                                typename StripFunctorClass<Functor>::type>::
                                                                   value>::type>
                struct is_functor
                {};

                // Any functor for move
                template<typename Functor, typename = is_functor<Functor>>
                // NOLINTNEXTLINE(misc-forwarding-reference-overload)
                Simple(Functor&& f) :
                    ptr_(&f),
                    move_cb_([](void* ptr, Function& func, Storage& storage) {
                        auto f = reinterpret_cast<Functor*>(ptr);
                        auto fs = new (storage.buffer) Functor(std::move(*f));
                        storage.cleanup = [](void* buf) {
                            reinterpret_cast<Functor*>(buf)->~Functor();
                        };
                        func = std::ref(*fs);
                    })
                {
                    static_assert(
                            sizeof(f) <= sizeof(Storage::buffer),
                            "Functor is too large for local buffer");
                    /*static_assert(
                            alignof(Functor) > Storage::BUFFER_ALIGN_AS,
                            "Functor alignment mismatches local buffer");*/
                    static_assert(
                            std::is_same<
                                    FP,
                                    typename StripFunctorClass<Functor>::type>::
                                    value,
                            "Functor type mismatches required signature");
                }

                // Any functor for copy
                template<typename Functor, typename = is_functor<Functor>>
                Simple(const Functor& f) :
                    ptr_(&const_cast<Functor&>(f)),
                    move_cb_([](void* ptr, Function& func, Storage& storage) {
                        auto f = reinterpret_cast<const Functor*>(ptr);
                        auto fs = new (storage.buffer) Functor(*f);
                        storage.cleanup = [](void* buf) {
                            reinterpret_cast<Functor*>(buf)->~Functor();
                        };
                        func = std::ref(*fs);
                    })
                {
                    static_assert(
                            sizeof(f) <= sizeof(Storage::buffer),
                            "Functor is too large for local buffer");
                    /*static_assert(
                            alignof(Functor) > Storage::BUFFER_ALIGN_AS,
                            "Functor alignment mismatches local buffer");*/
                    static_assert(
                            std::is_same<
                                    FP,
                                    typename StripFunctorClass<Functor>::type>::
                                    value,
                            "Functor type mismatches required signature");
                }

                // Any functor as reference
                template<
                        typename Functor,
                        typename = is_functor<Functor>,
                        typename FunctorNoConst =
                                typename std::remove_const<Functor>::type>
                Simple(std::reference_wrapper<Functor> f) :
                    ptr_(&const_cast<FunctorNoConst&>(f.get())),
                    move_cb_([](void* ptr,
                                Function& func,
                                Storage& /*storage*/) {
                        auto f = reinterpret_cast<Functor*>(ptr);
                        func = std::ref(*f);
                    })
                {
                    static_assert(
                            std::is_same<
                                    FP,
                                    typename StripFunctorClass<Functor>::type>::
                                    value,
                            "Functor type mismatches required signature");
                }

                void move(Function& dst, Storage& storage)
                {
                    move_cb_(ptr_, dst, storage);
                }

            protected:
                using ConvertCB = void (*)(void*, Function&, Storage&);

                void* ptr_;
                ConvertCB move_cb_;
            };
        } // namespace functor_pass
    }     // namespace details
} // namespace futoin

//---
#endif // FUTOIN_DETAILS_FUNCTOR_PASS_HPP
