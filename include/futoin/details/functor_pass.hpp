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
//! @brief Helper to pass capture various types of callbacks.
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
            constexpr size_t DEFAULT_ALIGN = sizeof(std::ptrdiff_t);
            constexpr size_t DEFAULT_SIZE = sizeof(std::ptrdiff_t) * 4;
            constexpr size_t REDUCED_SIZE = sizeof(std::ptrdiff_t) * 2;

            /**
             * @brief Storage for functors behind std::function
             *        to avoid expensive heap operations.
             */
            template<size_t S, size_t Align = DEFAULT_ALIGN>
            struct StorageBase
            {
                using CleanupCB = void (*)(void* buf);

                StorageBase() = default;
                StorageBase(const StorageBase&) = delete;
                StorageBase& operator=(const StorageBase&) = delete;
                StorageBase(StorageBase&&) = delete;
                StorageBase& operator=(StorageBase&&) = delete;

                ~StorageBase() noexcept
                {
                    set_cleanup(&default_cleanup);
                }

                void set_cleanup(CleanupCB cb)
                {
                    cleanup(buffer);
                    cleanup = cb;
                }

                static void default_cleanup(void* /*buf*/) {}

                CleanupCB cleanup{&default_cleanup};
                alignas(Align) std::uint8_t buffer[S];
            };

            /**
             * @brief Lightweight std::function partial replacement
             *        to be used with StorageBase.
             */
            template<typename FP>
            class Function;

            template<typename R, typename... A>
            class Function<R(A...)>
            {
            public:
                using FP = R(A...);

                Function() noexcept = default;
                Function(const Function&) noexcept = default;
                Function& operator=(const Function&) noexcept = default;

                Function(Function&& other) noexcept :
                    impl_(other.impl_),
                    data_(other.data_)
                {
                    other.impl_ = nullptr;
                }

                // NOLINTNEXTLINE(misc-unconventional-assign-operator)
                void operator=(Function&& other) noexcept
                {
                    if (this != &other) {
                        impl_ = other.impl_;
                        data_ = other.data_;
                        other.impl_ = nullptr;
                    }
                }

                // NOLINTNEXTLINE(misc-unconventional-assign-operator)
                void operator=(std::nullptr_t)
                {
                    impl_ = nullptr;
                }

                Function(FP* fp) noexcept
                {
                    this->operator=(fp);
                }

                // NOLINTNEXTLINE(misc-unconventional-assign-operator)
                void operator=(FP* fp)
                {
                    impl_ = [](void* ptr, A... args) {
                        auto fp = reinterpret_cast<FP*>(ptr);
                        return fp(std::forward<A>(args)...);
                    };
                    data_ = reinterpret_cast<void*>(fp);
                }

                template<
                        typename Functor,
                        typename FunctorNoConst =
                                typename std::remove_const<Functor>::type,
                        typename = decltype(
                                (R(FunctorNoConst::*)(A...))
                                & FunctorNoConst::operator())>
                Function(std::reference_wrapper<Functor> f) noexcept
                {
                    this->operator=(f);
                }

                template<
                        typename Functor,
                        typename FunctorNoConst =
                                typename std::remove_const<Functor>::type>
                // NOLINTNEXTLINE(misc-unconventional-assign-operator)
                void operator=(std::reference_wrapper<Functor> f) noexcept
                {
                    impl_ = [](void* ptr, A... args) {
                        auto fp = reinterpret_cast<Functor*>(ptr);
                        return (*fp)(std::forward<A>(args)...);
                    };
                    data_ = const_cast<FunctorNoConst*>(&f.get());
                }

                inline R operator()(A... args) const
                {
                    return (*impl_)(data_, std::forward<A>(args)...);
                }

                void target_type() const noexcept {}

                operator bool() const noexcept
                {
                    return impl_ != nullptr;
                }

            private:
                using Impl = R(void*, A...);
                Impl* impl_{nullptr};
                void* data_{nullptr};
            };

            /**
             * @brief Universal parameter definition to handle different
             *        use cases.
             * @note It must never be used outside of function parameter
             * definition!
             * @note It depend on lifetime of related function parameter value
             * during call.
             */
            template<
                    typename FP,
                    size_t FunctorSize = DEFAULT_SIZE,
                    template<typename> class FunctionType = std::function,
                    size_t Align = DEFAULT_ALIGN>
            class Simple
            {
            public:
                using FunctionSignature = FP;
                using Function = FunctionType<FP>;
                using Storage = StorageBase<FunctorSize, Align>;

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
                    ptr_(reinterpret_cast<void*>(fp)),
                    move_cb_([](void* ptr,
                                Function& func,
                                Storage& /*storage*/) {
                        auto fp = reinterpret_cast<FP*>(ptr);
                        func = fp;
                    })
                {}

                // std::function for move
                // NOLINTNEXTLINE(misc-forwarding-reference-overload)
                Simple(Function&& f) :
                    ptr_(&f),
                    move_cb_([](void* ptr,
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

                Simple(Function& f) : Simple(const_cast<const Function&>(f)) {}

                // Any functor for move
                template<
                        typename Functor,
                        typename =
                                decltype(Function(std::ref(*(Functor*) nullptr))
                                                 .target_type())>
                // NOLINTNEXTLINE(misc-forwarding-reference-overload)
                Simple(Functor&& f) :
                    ptr_(&f),
                    move_cb_([](void* ptr, Function& func, Storage& storage) {
                        auto f = reinterpret_cast<Functor*>(ptr);
                        auto fs = new (storage.buffer) Functor(std::move(*f));
                        storage.set_cleanup([](void* buf) {
                            reinterpret_cast<Functor*>(buf)->~Functor();
                        });
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
                template<
                        typename Functor,
                        typename =
                                decltype(Function(std::ref(*(Functor*) nullptr))
                                                 .target_type()),
                        typename FunctorNoConst =
                                typename std::remove_const<Functor>::type>
                Simple(Functor& f) :
                    ptr_(&const_cast<FunctorNoConst&>(f)),
                    move_cb_([](void* ptr, Function& func, Storage& storage) {
                        auto f = reinterpret_cast<Functor*>(ptr);
                        auto fs = new (storage.buffer) FunctorNoConst(*f);
                        storage.set_cleanup([](void* buf) {
                            reinterpret_cast<FunctorNoConst*>(buf)
                                    ->~FunctorNoConst();
                        });
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
                        typename =
                                decltype(Function(std::ref(*(Functor*) nullptr))
                                                 .target_type()),
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
                {}

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
