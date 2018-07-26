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
//! @brief Neutral native AsyncSteps (FTN8) interface for C++
//! @sa https://specs.futoin.org/final/preview/ftn12_async_api.html
//-----------------------------------------------------------------------------

#ifndef FTN_ASYNCSTEPS_HPP
#define FTN_ASYNCSTEPS_HPP
//---

#include "details/reqcpp11.hpp"
//---
#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>
//---
#include "details/any.hpp"
#include "errors.hpp"
//---

namespace futoin {
    class AsyncSteps;
    class ISync;

    /**
     * @brief Details of AsyncSteps interface
     */
    namespace asyncsteps {
        using GenericCallback = std::function<void(AsyncSteps &)>;
        using ExecHandler = GenericCallback;
        using ErrorHandler = std::function<void(AsyncSteps &, ErrorCode)>;
        using CancelCallback = GenericCallback;
        

        /**
         * @brief Interface of Async reactor
         */
        class AsyncTool {
          protected:
            AsyncTool() = default;
            virtual ~AsyncTool() noexcept = 0;

          private:
            AsyncTool(const AsyncTool &) = delete;
            AsyncTool(AsyncTool &&) = delete;
            AsyncTool &operator=(const AsyncTool &) = delete;
            AsyncTool &operator=(AsyncTool &&) = delete;
        };

        //---
        constexpr auto MAX_NEXT_ARGS = 4;

        using details::any;
        using details::any_cast;

        /**
         * @brief A special helper to wrap C-strings and objects passed by
         * pointer.
         */
        template <typename T> struct smart_forward {
            using RT = typename std::remove_reference<T>::type;

            template <bool = false> static T &&it(RT &v) {
                static_assert(
                    !std::is_pointer<RT>::value, "Pointers are not allowed.");
                return std::move(v);
            }

            template <bool = false> static T &&it(RT &&v) {
                static_assert(
                    !std::is_pointer<RT>::value, "Pointers are not allowed.");
                return v;
            }

            static std::string it(const char *v) {
                return v;
            }
            static std::u16string it(const char16_t *v) {
                return v;
            }
            static std::u32string it(const char32_t *v) {
                return v;
            }
        };

        /**
         * @internal
         */
        struct NextArgs : public std::array<any, MAX_NEXT_ARGS> {
            struct NoArg {};

            template <
                typename A, typename B = NoArg, typename C = NoArg,
                typename D = NoArg>
            inline void
            assign(A &&a, B &&b = {}, C &&c = {}, D &&d = {}) noexcept {
                any *p = data();
                *p = any(smart_forward<A>::it(a));
                *(++p) = any(smart_forward<B>::it(b));
                *(++p) = any(smart_forward<C>::it(c));
                *(++p) = any(smart_forward<D>::it(d));
            }

            template <typename A>
            inline void call(
                AsyncSteps &asi,
                const std::function<void(AsyncSteps &, A)> &exec_handler) {
                any *p = data();
                exec_handler(
                    asi,
                    any_cast<typename std::remove_reference<A>::type &&>(*p));
            }
            template <typename A, typename B>
            inline void call(
                AsyncSteps &asi,
                const std::function<void(AsyncSteps &, A, B)> &exec_handler) {
                any *p = data();
                exec_handler(
                    asi,
                    any_cast<typename std::remove_reference<A>::type &&>(*p),
                    any_cast<typename std::remove_reference<B>::type &&>(
                        *(p + 1)));
            }
            template <typename A, typename B, typename C>
            inline void call(
                AsyncSteps &asi,
                const std::function<void(AsyncSteps &, A, B, C)>
                    &exec_handler) {
                any *p = data();
                exec_handler(
                    asi,
                    any_cast<typename std::remove_reference<A>::type &&>(*p),
                    any_cast<typename std::remove_reference<B>::type &&>(
                        *(p + 1)),
                    any_cast<typename std::remove_reference<C>::type &&>(
                        *(p + 2)));
            }
            template <typename A, typename B, typename C, typename D>
            inline void call(
                AsyncSteps &asi,
                const std::function<void(AsyncSteps &, A, B, C, D)>
                    &exec_handler) {
                any *p = data();
                exec_handler(
                    asi,
                    any_cast<typename std::remove_reference<A>::type &&>(*p),
                    any_cast<typename std::remove_reference<B>::type &&>(
                        *(p + 1)),
                    any_cast<typename std::remove_reference<C>::type &&>(
                        *(p + 2)),
                    any_cast<typename std::remove_reference<D>::type &&>(
                        *(p + 3)));
            }
        };

        //---
        /**
         * @internal
         */
        template <typename FP> struct StripFunctorClassHelper {};

        template <typename R, typename... A>
        struct StripFunctorClassHelper<R(A...)> {
            using type = R(A...);
        };
        template <typename R, typename C, typename... A>
        struct StripFunctorClassHelper<R (C::*)(A...) const> {
            using type = R(A...);
        };

        template <typename R, typename C, typename... A>
        struct StripFunctorClassHelper<R (C::*)(A...)>
            : public StripFunctorClassHelper<R (C::*)(A...) const> {};

        /**
         * @internal
         */
        template <typename C>
        struct StripFunctorClass
            : public StripFunctorClassHelper<decltype(&C::operator())> {};

        //---

        using State = std::unordered_map<std::string, any>;
    } // namespace asyncsteps

    /**
     * @brief Synchronization primitive interface.
     */
    class ISync {
      public:
        virtual void sync(
            AsyncSteps &, asyncsteps::ExecHandler &&exec_handler,
            asyncsteps::ErrorHandler &&error_handler) noexcept = 0;

      protected:
        virtual ~ISync() noexcept = 0;
    };

    /**
     * @brief Primary AsyncSteps interface
     */
    class AsyncSteps {
      public:
        /**
         * @name Common API
         */
        ///@{

        /**
         * @brief Add generic step.
         */
        AsyncSteps &
        add(asyncsteps::ExecHandler exec_handler,
            asyncsteps::ErrorHandler error_handler = {}) noexcept {
            add_step(std::move(exec_handler), std::move(error_handler));
            return *this;
        }

        /**
         * @brief Add step which accepts required result variables.
         */
        template <typename... T>
        AsyncSteps &
        add(std::function<void(AsyncSteps &, T...)> &&exec_handler,
            asyncsteps::ErrorHandler &&error_handler = {}) noexcept {
            add_step(
                std::move(asyncsteps::ExecHandler(
                    [this, exec_handler](AsyncSteps &asi) {
                        this->nextargs().call(asi, exec_handler);
                    })),
                std::move(error_handler));
            return *this;
        }

        /**
         * @brief Add step which accepts required result variables.
         */
        template <typename F>
        AsyncSteps &
        add(const F &functor_handler,
            asyncsteps::ErrorHandler error_handler = {}) noexcept {
            typedef typename asyncsteps::StripFunctorClass<F>::type FP;
            return add(
                std::move(std::function<FP>(functor_handler)),
                std::move(error_handler));
        }

        /**
         * @brief Get pseudo-parallelized AsyncSteps/
         */
        virtual AsyncSteps &
        parallel(asyncsteps::ErrorHandler error_handler = {}) noexcept = 0;

        /**
         * @brief Reference to associated state object.
         */
        asyncsteps::State &state;

        /**
         * @brief Copy steps from a model step.
         */
        virtual AsyncSteps &copyFrom(AsyncSteps &other) noexcept = 0;

        /**
         * @brief Add generic step synchronized against object.
         */
        AsyncSteps &sync(
            ISync &obj, asyncsteps::ExecHandler exec_handler,
            asyncsteps::ErrorHandler error_handler = {}) noexcept {
            obj.sync(*this, std::move(exec_handler), std::move(error_handler));
            return *this;
        }

        /**
         * @brief Add step which accepts required result variables and
         * synchronized against object.
         */
        template <typename... T>
        AsyncSteps &sync(
            ISync &obj, std::function<void(AsyncSteps &, T...)> &&exec_handler,
            asyncsteps::ErrorHandler &&error_handler = {}) noexcept {
            obj.sync(
                *this,
                std::move(asyncsteps::ExecHandler(
                    [this, exec_handler](AsyncSteps &asi) {
                        this->nextargs().call(asi, exec_handler);
                    })),
                std::move(error_handler));
            return *this;
        }

        /**
         * @brief Add step which accepts required result variables and
         * synchronized against object.
         */
        template <typename F>
        AsyncSteps &sync(
            ISync &obj, const F &functor_handler,
            asyncsteps::ErrorHandler error_handler = {}) noexcept {
            typedef typename asyncsteps::StripFunctorClass<F>::type FP;
            return obj.sync(
                *this, std::move(std::function<FP>(functor_handler)),
                std::move(error_handler));
        }

        ///@}

        /**
         * @name Execution API
         */
        ///@{

        /**
         * @brief Generic success completion
         */
        virtual void success() noexcept = 0;

        /**
         * @brief Completion with result variables
         */
        template <typename... T> void success(T &&... args) noexcept {
            nextargs().assign(std::forward<T>(args)...);
            success();
        }

        /**
         * @brief Step abort with specified error
         */
        virtual void error(ErrorCode, ErrorMessage = {}) throw(Error) = 0;

        /**
         * @brief Set time limit of execution.
         * Inform execution engine to wait for either success() or error() for
         * specified timeout in ms. On timeout, error("Timeout") is called
         */
        virtual void setTimeout(std::chrono::milliseconds) noexcept = 0;

        /**
         * @brief Alias of success()
         */
        void operator()() noexcept {
            success();
        }

        /**
         * @brief Alias of success()
         */
        template <typename... T> void operator()(T &&... args) noexcept {
            nextargs().assign(std::forward<T>(args)...);
            success();
        }

        /**
         * @brief Set callback, to be used to cancel execution.
         */
        virtual void setCancel(asyncsteps::CancelCallback) noexcept = 0;

        /**
         * @brief Prevent implicit as.success() behavior of current step.
         */
        virtual void waitExternal() noexcept = 0;

        ///@}

        /**
         * @name Execution Loop API
         */
        ///@{

        ///@}

      protected:
        AsyncSteps(asyncsteps::State &state) : state(state) {}
        virtual ~AsyncSteps() noexcept = 0;

        virtual void
        add_step(asyncsteps::ExecHandler &&, asyncsteps::ErrorHandler &&) = 0;
        virtual asyncsteps::NextArgs &nextargs() noexcept = 0;

        /**
         * @name Control API
         */
        ///@{

        /**
         * @brief Start execution of root AsyncSteps object
         */
        virtual void execute() noexcept = 0;

        /**
         * @brief Cancel execution of root AsyncSteps object
         */
        virtual void cancel() noexcept = 0;

        ///@}

      private:
        AsyncSteps &operator=(const AsyncSteps &) = delete;
        AsyncSteps &operator=(AsyncSteps &&) = delete;
    };

    // Ensure d-tors are available even when declared pure virtual
    asyncsteps::AsyncTool::~AsyncTool() noexcept {}
    AsyncSteps::~AsyncSteps() noexcept {}
    ISync::~ISync() noexcept {}
} // namespace futoin

//---
#endif // FTN_ASYNCSTEPS_HPP
