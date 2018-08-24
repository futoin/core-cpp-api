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

#ifndef FUTOIN_IASYNCSTEPS_HPP
#define FUTOIN_IASYNCSTEPS_HPP
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
#include <boost/pool/pool_alloc.hpp>
//---
#include "any.hpp"
#include "details/asyncloop.hpp"
#include "details/functor_pass.hpp"
#include "details/nextargs.hpp"
#include "details/strip_functor_class.hpp"
#include "errors.hpp"
//---

namespace futoin {
    class IAsyncSteps;
    class ISync;

    /**
     * @brief Details of AsyncSteps interface
     */
    namespace asyncsteps {
        using namespace details::asyncloop;
        using namespace details::nextargs;

        using GenericCallbackSignature = void(IAsyncSteps&);
        using GenericCallback = std::function<GenericCallbackSignature>;
        using ExecHandlerSignature = GenericCallbackSignature;
        using ExecHandler = GenericCallback;
        using ErrorHandlerSignature = void(IAsyncSteps&, ErrorCode);
        using ErrorHandler = std::function<ErrorHandlerSignature>;
        using CancelCallbackSignature = GenericCallbackSignature;
        using CancelCallback = GenericCallback;

        //---

        using State = std::unordered_map<
                std::string,
                any,
                std::unordered_map<std::string, any>::hasher,
                std::unordered_map<std::string, any>::key_equal,
                boost::pool_allocator<any>>;
    } // namespace asyncsteps

    /**
     * @brief Synchronization primitive interface.
     */
    class ISync
    {
    public:
        virtual void lock(IAsyncSteps&) noexcept = 0;
        virtual void unlock(IAsyncSteps&) noexcept = 0;

    protected:
        virtual ~ISync() noexcept = default;
    };

    /**
     * @brief Primary AsyncSteps interface
     */
    class IAsyncSteps
    {
    public:
        using ExecPass =
                details::functor_pass::Simple<asyncsteps::ExecHandlerSignature>;
        using ErrorPass = details::functor_pass::Simple<
                asyncsteps::ErrorHandlerSignature>;
        using CancelPass = details::functor_pass::Simple<
                asyncsteps::CancelCallbackSignature>;

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
        IAsyncSteps& add(ExecPass func, ErrorPass on_error = {}) noexcept
        {
            StepData& step = add_step();
            func.move(step.func_, step.func_storage_);
            on_error.move(step.on_error_, step.on_error_storage_);
            return *this;
        }

        /**
         * @brief Add step which accepts required result variables.
         */
        template<typename... T>
        IAsyncSteps& add(
                details::functor_pass::Simple<void(IAsyncSteps&, T...)> func,
                ErrorPass on_error = {}) noexcept
        {
            StepData& step = add_step();

            // 1. Create holder for actual callback with all parameters
            using OrigFunction = std::function<void(IAsyncSteps&, T...)>;
            auto* orig_fp = new (step.func_extra_.buffer) OrigFunction;
            step.func_extra_.cleanup = [](void* ptr) {
                reinterpret_cast<OrigFunction*>(ptr)->~OrigFunction();
            };

            func.move(*orig_fp, step.func_extra_storage_);

            // 2. Create adapter functor with all allocations in buffers
            auto adapter = [this, orig_fp](IAsyncSteps& asi) {
                this->nextargs().call(asi, *orig_fp);
            };
            ExecPass adapter_pass{adapter};
            adapter_pass.move(step.func_, step.func_storage_);

            // 3. Simple error callback
            on_error.move(step.on_error_, step.on_error_storage_);

            return *this;
        }

        /**
         * @brief Add step which accepts required result variables.
         */
        template<
                typename Functor,
                typename = decltype(&Functor::operator()),
                typename FP =
                        typename details::StripFunctorClass<Functor>::type,
                typename = typename std::enable_if<!std::is_same<
                        FP,
                        asyncsteps::ExecHandlerSignature>::value>::type>
        IAsyncSteps& add(Functor&& func, ErrorPass on_error = {}) noexcept
        {
            return add(
                    details::functor_pass::Simple<FP>(
                            std::forward<Functor>(func)),
                    on_error);
        }

        /**
         * @brief Get pseudo-parallelized IAsyncSteps/
         */
        virtual IAsyncSteps& parallel(ErrorPass on_error = {}) noexcept = 0;

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
                ISync& obj, ExecPass func, ErrorPass on_error = {}) noexcept
        {
            StepData& step = add_step();

            struct SyncFunc
            {
                SyncFunc(ISync& obj) : obj(obj) {}

                ISync& obj;
                asyncsteps::ExecHandler orig_func;
                asyncsteps::ErrorHandler orig_on_error;

                void operator()(IAsyncSteps& asi) const
                {
                    asi.setCancel([&](IAsyncSteps& asi) { obj.unlock(asi); });
                    asi.add([&](IAsyncSteps& asi) { obj.lock(asi); });
                    asi.add(orig_func, orig_on_error);
                    asi.add([&](IAsyncSteps& asi) { obj.unlock(asi); });
                }
            };

            SyncFunc sync_func(obj);

            func.move(sync_func.orig_func, step.func_storage_);
            on_error.move(sync_func.orig_on_error, step.on_error_storage_);

            step.func_ = std::move(sync_func); // NOTE: heap alloc

            return *this;
        }

        /**
         * @brief Add step which accepts required result variables and
         * synchronized against object.
         */
        template<typename... T>
        IAsyncSteps& sync(
                ISync& obj,
                details::functor_pass::Simple<void(IAsyncSteps&, T...)> func,
                ErrorPass on_error = {}) noexcept
        {
            StepData& step = add_step();

            // 1. Create holder for actual callback with all parameters
            using OrigFunction = std::function<void(IAsyncSteps&, T...)>;
            auto* orig_fp = new (step.func_extra_.buffer) OrigFunction;
            step.func_extra_.cleanup = [](void* ptr) {
                reinterpret_cast<OrigFunction*>(ptr)->~OrigFunction();
            };

            func.move(*orig_fp, step.func_extra_storage_);

            // 2. Create sync step
            struct SyncFunc
            {
                SyncFunc(ISync& obj) : obj(obj) {}

                ISync& obj;
                asyncsteps::ExecHandler orig_func;
                asyncsteps::ErrorHandler orig_on_error;

                void operator()(IAsyncSteps& asi) const
                {
                    asi.setCancel([&](IAsyncSteps& asi) { obj.unlock(asi); });
                    asi.add([&](IAsyncSteps& asi) { obj.lock(asi); });
                    asi.add(orig_func, orig_on_error);
                    asi.add([&](IAsyncSteps& asi) { obj.unlock(asi); });
                }
            };

            SyncFunc sync_func(obj);

            // 3. Create adapter functor with all allocations in buffers
            auto adapter = [this, orig_fp](IAsyncSteps& asi) {
                this->nextargs().call(asi, *orig_fp);
            };
            ExecPass adapter_pass{adapter};
            adapter_pass.move(sync_func.orig_func, step.func_storage_);

            // 4. Simple error callback
            on_error.move(sync_func.orig_on_error, step.on_error_storage_);

            // 5. Set function
            step.func_ = std::move(sync_func); // NOTE: heap alloc

            return *this;
        }

        /**
         * @brief Add step which accepts required result variables and
         * synchronized against object.
         */
        template<
                typename Functor,
                typename = decltype(&Functor::operator()),
                typename FP =
                        typename details::StripFunctorClass<Functor>::type,
                typename = typename std::enable_if<!std::is_same<
                        FP,
                        asyncsteps::ExecHandlerSignature>::value>::type>
        IAsyncSteps& sync(
                ISync& obj, Functor&& func, ErrorPass on_error = {}) noexcept
        {
            return sync(
                    obj,
                    details::functor_pass::Simple<FP>(
                            std::forward<Functor>(func)),
                    on_error);
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
        virtual void setCancel(CancelPass) noexcept = 0;

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
        IAsyncSteps& loop(ExecPass func, asyncsteps::LoopLabel label = nullptr)
        {
            auto& ls = add_loop();

            ls.label = label;

            asyncsteps::ExecHandler handler;
            func.move(handler, ls.outer_func_storage);

            ls.set_handler([handler](asyncsteps::LoopState&, IAsyncSteps& as) {
                handler(as);
            });

            return *this;
        }

        /**
         * @brief Loop with iteration limit
         */
        IAsyncSteps& repeat(
                std::size_t count,
                details::functor_pass::Simple<void(IAsyncSteps&, std::size_t i)>
                        func,
                asyncsteps::LoopLabel label = nullptr)
        {
            auto& ls = add_loop();

            ls.label = label;
            ls.i = 0;

            std::function<void(IAsyncSteps&, std::size_t i)> handler;
            func.move(handler, ls.outer_func_storage);

            ls.set_handler(
                    [handler](asyncsteps::LoopState& ls, IAsyncSteps& as) {
                        handler(as, ls.i++);
                    });
            ls.set_cond([count](asyncsteps::LoopState& ls) {
                return ls.i < count;
            });

            return *this;
        }

        /**
         * @brief Loop over std::map-like container
         */
        template<
                typename C,
                typename FP,
                typename V = typename C::mapped_type,
                bool map = true>
        IAsyncSteps& forEach(
                C& c,
                details::functor_pass::Simple<FP> func,
                asyncsteps::LoopLabel label = nullptr)
        {
            auto iter = std::begin(c);
            auto end = std::end(c);
            using Iter = decltype(iter);

            auto& ls = add_loop();

            ls.label = label;
            ls.data = any(std::move(iter));

            std::function<FP> handler;
            func.move(handler, ls.outer_func_storage);

            ls.set_handler(
                    [handler](asyncsteps::LoopState& ls, IAsyncSteps& as) {
                        Iter& iter = any_cast<Iter&>(ls.data);
                        auto& pair = *iter;
                        ++iter; // make sure to increment before handler call
                        handler(as, pair.first, pair.second);
                    });
            ls.set_cond([end](asyncsteps::LoopState& ls) {
                return any_cast<Iter&>(ls.data) != end;
            });

            return *this;
        }

        /**
         * @brief Loop over std::vector-like container
         */
        template<typename C, typename FP, typename V = decltype(C().back())>
        IAsyncSteps& forEach(
                C& c,
                details::functor_pass::Simple<FP> func,
                asyncsteps::LoopLabel label = nullptr)
        {
            auto iter = std::begin(c);
            auto end = std::end(c);
            using Iter = decltype(iter);

            auto& ls = add_loop();

            ls.label = label;
            ls.i = 0;
            ls.data = any(std::move(iter));

            std::function<FP> handler;
            func.move(handler, ls.outer_func_storage);

            ls.set_handler(
                    [handler](asyncsteps::LoopState& ls, IAsyncSteps& as) {
                        Iter& iter = any_cast<Iter&>(ls.data);
                        auto& val = *iter;
                        ++iter; // make sure to increment before handler call
                        handler(as, ls.i++, val);
                    });
            ls.set_cond([end](asyncsteps::LoopState& ls) {
                return any_cast<Iter&>(ls.data) != end;
            });

            return *this;
        }

        /**
         * @brief Loop over std::vector-like container
         */
        template<
                typename C,
                typename F,
                typename = decltype(&F::operator()),
                typename FP = typename details::StripFunctorClass<F>::type>
        IAsyncSteps& forEach(
                C& c, F func, asyncsteps::LoopLabel label = nullptr)
        {
            return forEach(c, details::functor_pass::Simple<FP>(func), label);
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
        struct StepData
        {
            ExecPass::Storage func_storage_;
            ExecPass::Storage func_extra_storage_;
            ErrorPass::Storage on_error_storage_;
            asyncsteps::ExecHandler func_;
            details::functor_pass::StorageBase<sizeof(asyncsteps::ExecHandler)>
                    func_extra_;
            asyncsteps::ErrorHandler on_error_;
        };

        IAsyncSteps() = default;

        virtual StepData& add_step() = 0;
        virtual void handle_success() = 0;
        virtual void handle_error(ErrorCode) = 0;
        virtual asyncsteps::NextArgs& nextargs() noexcept = 0;
        virtual asyncsteps::LoopState& add_loop() noexcept = 0;

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
#endif // FUTOIN_IASYNCSTEPS_HPP
