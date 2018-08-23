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
//! @brief Neutral native IAsyncSteps (FTN12) interface for C++
//! @sa https://specs.futoin.org/final/preview/ftn12_async_api.html
//-----------------------------------------------------------------------------

#ifndef FUTOIN_ASYNCSTEPS_HPP
#define FUTOIN_ASYNCSTEPS_HPP
//---

#include "details/reqcpp11.hpp"
//---
#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>
//---
#include "any.hpp"
#include "errors.hpp"
//---

namespace futoin {
    class IAsyncSteps;
    class ISync;

    /**
     * @brief Details of AsyncSteps interface
     */
    namespace asyncsteps {
        class LoopState;

        using GenericCallback = std::function<void(IAsyncSteps&)>;
        using ExecHandler = GenericCallback;
        using ErrorHandler = std::function<void(IAsyncSteps&, ErrorCode)>;
        using CancelCallback = GenericCallback;
        using LoopLabel = const char*;
        using LoopHandler = std::function<void(LoopState&, IAsyncSteps&)>;
        using LoopCondition = std::function<bool(LoopState&)>;

        //---
        constexpr auto MAX_NEXT_ARGS = 4;

        /**
         * @brief A special helper to wrap C-strings and objects passed by
         * pointer.
         */
        template<typename T>
        struct smart_forward
        {
            using RT = typename std::remove_reference<T>::type;

            template<bool = false>
            static T&& it(RT& v)
            {
                static_assert(
                        !std::is_pointer<RT>::value,
                        "Pointers are not allowed.");
                return std::move(v);
            }

            template<bool = false>
            static T&& it(RT&& v)
            {
                static_assert(
                        !std::is_pointer<RT>::value,
                        "Pointers are not allowed.");
                return v;
            }

            static std::string it(const char* v)
            {
                return v;
            }
            static std::u16string it(const char16_t* v)
            {
                return v;
            }
            static std::u32string it(const char32_t* v)
            {
                return v;
            }
        };

        /**
         * @internal
         */
        struct NextArgs : public std::array<any, MAX_NEXT_ARGS>
        {
            struct NoArg
            {};

            template<
                    typename A,
                    typename B = NoArg,
                    typename C = NoArg,
                    typename D = NoArg>
            inline void assign(
                    A&& a, B&& b = {}, C&& c = {}, D&& d = {}) noexcept
            {
                any* p = data();
                *p = any(smart_forward<A>::it(a));
                *(++p) = any(smart_forward<B>::it(b));
                *(++p) = any(smart_forward<C>::it(c));
                *(++p) = any(smart_forward<D>::it(d));
            }

            template<typename A>
            inline void call(
                    IAsyncSteps& asi,
                    const std::function<void(IAsyncSteps&, A)>& exec_handler)
            {
                any* p = data();
                exec_handler(
                        asi,
                        any_cast<typename std::remove_reference<A>::type&&>(
                                *p));
            }
            template<typename A, typename B>
            inline void call(
                    IAsyncSteps& asi,
                    const std::function<void(IAsyncSteps&, A, B)>& exec_handler)
            {
                any* p = data();
                exec_handler(
                        asi,
                        any_cast<typename std::remove_reference<A>::type&&>(*p),
                        any_cast<typename std::remove_reference<B>::type&&>(
                                p + 1));
            }
            template<typename A, typename B, typename C>
            inline void
            call(IAsyncSteps& asi,
                 const std::function<void(IAsyncSteps&, A, B, C)>& exec_handler)
            {
                any* p = data();
                exec_handler(
                        asi,
                        any_cast<typename std::remove_reference<A>::type&&>(*p),
                        any_cast<typename std::remove_reference<B>::type&&>(
                                *(p + 1)),
                        any_cast<typename std::remove_reference<C>::type&&>(
                                *(p + 2)));
            }
            template<typename A, typename B, typename C, typename D>
            inline void call(
                    IAsyncSteps& asi,
                    const std::function<void(IAsyncSteps&, A, B, C, D)>&
                            exec_handler)
            {
                any* p = data();
                exec_handler(
                        asi,
                        any_cast<typename std::remove_reference<A>::type&&>(*p),
                        any_cast<typename std::remove_reference<B>::type&&>(
                                *(p + 1)),
                        any_cast<typename std::remove_reference<C>::type&&>(
                                *(p + 2)),
                        any_cast<typename std::remove_reference<D>::type&&>(
                                *(p + 3)));
            }
        };

        //---
        /**
         * @internal
         */
        template<typename FP>
        struct StripFunctorClassHelper
        {};

        template<typename R, typename... A>
        struct StripFunctorClassHelper<R(A...)>
        {
            using type = R(A...);
        };
        template<typename R, typename C, typename... A>
        struct StripFunctorClassHelper<R (C::*)(A...) const>
        {
            using type = R(A...);
        };

        template<typename R, typename C, typename... A>
        struct StripFunctorClassHelper<R (C::*)(A...)>
            : public StripFunctorClassHelper<R (C::*)(A...) const>
        {};

        /**
         * @internal
         */
        template<typename C>
        struct StripFunctorClass
            : public StripFunctorClassHelper<decltype(&C::operator())>
        {};

        //---

        using State = std::unordered_map<std::string, any>;

        //---
        struct LoopState
        {
            LoopState(
                    LoopLabel label,
                    LoopHandler&& handler,
                    LoopCondition&& cond = {},
                    any&& data = {}) :
                i(0),
                label(label), handler(handler), cond(cond),
                data(std::forward<any>(data))
            {}

            LoopState() = default;
            LoopState(LoopState&&) = default;
            LoopState& operator=(LoopState&&) = default;

            LoopState(const LoopState&) = delete;
            LoopState& operator=(const LoopState&) = delete;

            std::size_t i;
            LoopLabel label;
            LoopHandler handler;
            LoopCondition cond;
            any data;
        };

        //---
        class LoopBreak : public Error
        {
        public:
            LoopBreak(LoopLabel label) : Error(errors::LoopBreak), label_(label)
            {}

            inline LoopLabel label()
            {
                return label_;
            }

        private:
            LoopLabel label_;
        };

        //---
        class LoopContinue : public Error
        {
        public:
            LoopContinue(LoopLabel label) :
                Error(errors::LoopCont), label_(label)
            {}

            inline LoopLabel label()
            {
                return label_;
            }

        private:
            LoopLabel label_;
        };
    } // namespace asyncsteps

    /**
     * @brief Synchronization primitive interface.
     */
    class ISync
    {
    public:
        virtual void sync(
                IAsyncSteps&,
                asyncsteps::ExecHandler&& exec_handler,
                asyncsteps::ErrorHandler&& on_errorandler) noexcept = 0;

    protected:
        virtual ~ISync() noexcept = default;
    };

    /**
     * @brief Primary AsyncSteps interface
     */
    class IAsyncSteps
    {
    public:
        IAsyncSteps(const IAsyncSteps&) = delete;
        IAsyncSteps& operator=(const IAsyncSteps&) = delete;
        IAsyncSteps(IAsyncSteps&&) = default;
        IAsyncSteps& operator=(IAsyncSteps&&) = default;
        virtual ~IAsyncSteps() noexcept = default;

        /**
         * @name Common API
         */
        ///@{

        /**
         * @brief Add generic step.
         */
        IAsyncSteps& add(
                asyncsteps::ExecHandler exec_handler,
                asyncsteps::ErrorHandler on_errorandler = {}) noexcept
        {
            add_step(std::move(exec_handler), std::move(on_errorandler));
            return *this;
        }

        /**
         * @brief Add step which accepts required result variables.
         */
        template<typename... T>
        IAsyncSteps& add(
                std::function<void(IAsyncSteps&, T...)>&& exec_handler,
                asyncsteps::ErrorHandler&& on_errorandler = {}) noexcept
        {
            add_step(
                    std::move(asyncsteps::ExecHandler(
                            [this, exec_handler](IAsyncSteps& asi) {
                                this->nextargs().call(asi, exec_handler);
                            })),
                    std::move(on_errorandler));
            return *this;
        }

        /**
         * @brief Add step which accepts required result variables.
         */
        template<typename F>
        IAsyncSteps& add(
                const F& functor_handler,
                asyncsteps::ErrorHandler on_errorandler = {}) noexcept
        {
            using FP = typename asyncsteps::StripFunctorClass<F>::type;
            return add(
                    std::move(std::function<FP>(functor_handler)),
                    std::move(on_errorandler));
        }

        /**
         * @brief Get pseudo-parallelized IAsyncSteps/
         */
        virtual IAsyncSteps& parallel(
                asyncsteps::ErrorHandler on_errorandler = {}) noexcept = 0;

        /**
         * @brief Reference to associated state object.
         */
        virtual asyncsteps::State& state() noexcept = 0;

        /**
         * @brief Copy steps from a model step.
         */
        virtual IAsyncSteps& copyFrom(IAsyncSteps& other) noexcept = 0;

        /**
         * @brief Add generic step synchronized against object.
         */
        IAsyncSteps& sync(
                ISync& obj,
                asyncsteps::ExecHandler exec_handler,
                asyncsteps::ErrorHandler on_errorandler = {}) noexcept
        {
            obj.sync(*this, std::move(exec_handler), std::move(on_errorandler));
            return *this;
        }

        /**
         * @brief Add step which accepts required result variables and
         * synchronized against object.
         */
        template<typename... T>
        IAsyncSteps& sync(
                ISync& obj,
                std::function<void(IAsyncSteps&, T...)>&& exec_handler,
                asyncsteps::ErrorHandler&& on_errorandler = {}) noexcept
        {
            obj.sync(
                    *this,
                    std::move(asyncsteps::ExecHandler(
                            [this, exec_handler](IAsyncSteps& asi) {
                                this->nextargs().call(asi, exec_handler);
                            })),
                    std::move(on_errorandler));
            return *this;
        }

        /**
         * @brief Add step which accepts required result variables and
         * synchronized against object.
         */
        template<typename F>
        IAsyncSteps& sync(
                ISync& obj,
                const F& functor_handler,
                asyncsteps::ErrorHandler on_errorandler = {}) noexcept
        {
            using FP = typename asyncsteps::StripFunctorClass<F>::type;
            return obj.sync(
                    *this,
                    std::move(std::function<FP>(functor_handler)),
                    std::move(on_errorandler));
        }

        /**
         * @brief Create a new instance for standalone execution
         */
        virtual std::unique_ptr<IAsyncSteps> newInstance() noexcept = 0;

        ///@}

        /**
         * @name Execution API
         */
        ///@{

        /**
         * @brief Generic success completion
         */
        void success() noexcept
        {
            handle_success();
        }

        /**
         * @brief Completion with result variables
         */
        template<typename... T>
        void success(T&&... args) noexcept
        {
            nextargs().assign(std::forward<T>(args)...);
            handle_success();
        }

        /**
         * @brief Step abort with specified error
         */
        [[noreturn]] void error(ErrorCode error, ErrorMessage error_info = {}) {
            state()["error_info"] = any(std::move(error_info));
            handle_error(error);
            throw Error(error);
        }

        /**
         * @brief Set time limit of execution.
         * Inform execution engine to wait for either success() or error() for
         * specified timeout in ms. On timeout, error("Timeout") is called
         */
        virtual void setTimeout(std::chrono::milliseconds) noexcept = 0;

        /**
         * @brief Alias of success()
         */
        void operator()() noexcept
        {
            success();
        }

        /**
         * @brief Alias of success()
         */
        template<typename... T>
        void operator()(T&&... args) noexcept
        {
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

        /**
         * @brief Generic infinite loop
         */
        IAsyncSteps& loop(
                const asyncsteps::ExecHandler& handler,
                asyncsteps::LoopLabel label = nullptr)
        {
            loop_logic(asyncsteps::LoopState(
                    label, [handler](asyncsteps::LoopState&, IAsyncSteps& as) {
                        handler(as);
                    }));
            return *this;
        }

        /**
         * @brief Loop with iteration limit
         */
        IAsyncSteps& repeat(
                std::size_t count,
                const std::function<void(IAsyncSteps&, std::size_t i)>& handler,
                asyncsteps::LoopLabel label = nullptr)
        {
            loop_logic(asyncsteps::LoopState(
                    label,
                    [handler](asyncsteps::LoopState& ls, IAsyncSteps& as) {
                        handler(as, ls.i++);
                    },
                    [count](asyncsteps::LoopState& ls) {
                        return ls.i < count;
                    }));
            return *this;
        }

        /**
         * @brief Loop over std::map-like container
         */
        template<
                typename C,
                typename F,
                typename V = typename C::mapped_type,
                bool map = true>
        IAsyncSteps& forEach(
                C& c, const F& handler, asyncsteps::LoopLabel label = nullptr)
        {

            auto iter = std::begin(c);
            auto end = std::end(c);
            using Iter = decltype(iter);

            loop_logic(asyncsteps::LoopState(
                    label,
                    [handler](asyncsteps::LoopState& ls, IAsyncSteps& as) {
                        Iter& iter = any_cast<Iter&>(ls.data);
                        auto& pair = *iter;
                        ++iter; // make sure to increment before handler call
                        handler(as, pair.first, pair.second);
                    },
                    [end](asyncsteps::LoopState& ls) {
                        return any_cast<Iter&>(ls.data) != end;
                    },
                    any(std::move(iter))));
            return *this;
        }

        /**
         * @brief Loop over std::vector-like container
         */
        template<typename C, typename F, typename V = decltype(C().back())>
        IAsyncSteps& forEach(
                C& c, const F& handler, asyncsteps::LoopLabel label = nullptr)
        {

            auto iter = std::begin(c);
            auto end = std::end(c);
            using Iter = decltype(iter);

            loop_logic(asyncsteps::LoopState(
                    label,
                    [handler](asyncsteps::LoopState& ls, IAsyncSteps& as) {
                        Iter& iter = any_cast<Iter&>(ls.data);
                        auto& val = *iter;
                        ++iter; // make sure to increment before handler call
                        handler(as, ls.i++, val);
                    },
                    [end](asyncsteps::LoopState& ls) {
                        return any_cast<Iter&>(ls.data) != end;
                    },
                    any(std::move(iter))));
            return *this;
        }

        /**
         * @brief Break async loop.
         */
        [[noreturn]] inline void breakLoop(
                asyncsteps::LoopLabel label = nullptr)
        {
            throw asyncsteps::LoopBreak(label);
        }

        /**
         * @brief Continue async loop from the next iteration.
         */
        [[noreturn]] inline void continueLoop(
                asyncsteps::LoopLabel label = nullptr)
        {
            throw asyncsteps::LoopContinue(label);
        }

        ///@}

    protected:
        IAsyncSteps() = default;

        virtual void add_step(
                asyncsteps::ExecHandler&&, asyncsteps::ErrorHandler&&) = 0;
        virtual void handle_success() = 0;
        virtual void handle_error(ErrorCode) = 0;
        virtual asyncsteps::NextArgs& nextargs() noexcept = 0;
        virtual void loop_logic(asyncsteps::LoopState&&) noexcept = 0;

        /**
         * @name Control API
         */
        ///@{

        /**
         * @brief Start execution of root IAsyncSteps object
         */
        virtual void execute() noexcept = 0;

        /**
         * @brief Cancel execution of root IAsyncSteps object
         */
        virtual void cancel() noexcept = 0;

        ///@}
    };
} // namespace futoin

//---
#endif // FUTOIN_ASYNCSTEPS_HPP
