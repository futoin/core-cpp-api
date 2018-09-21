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
#include <cassert>
#include <chrono>
#include <cstdint>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <thread>
#include <type_traits>
#include <typeinfo>
//---
#include "any.hpp"
#include "details/asyncloop.hpp"
#include "details/functor_pass.hpp"
#include "details/nextargs.hpp"
#include "details/strip_functor_class.hpp"
#include "errors.hpp"
#include "imempool.hpp"
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
        namespace functor_pass = details::functor_pass;

        using ExecHandlerSignature = void(IAsyncSteps&);
        using ExecPass = functor_pass::Simple<
                ExecHandlerSignature,
                functor_pass::DEFAULT_SIZE,
                functor_pass::Function>;
        using ExecHandler = ExecPass::Function;

        using ErrorHandlerSignature = void(IAsyncSteps&, ErrorCode);
        using ErrorPass = functor_pass::Simple<
                ErrorHandlerSignature,
                functor_pass::DEFAULT_SIZE,
                functor_pass::Function>;
        using ErrorHandler = ErrorPass::Function;

        using CancelCallbackSignature = void(IAsyncSteps&) /*noexcept*/;
        using CancelPass = functor_pass::Simple<
                CancelCallbackSignature,
                functor_pass::DEFAULT_SIZE,
                functor_pass::Function>;
        using CancelCallback = CancelPass::Function;

        using AwaitCallbackSignature =
                bool(IAsyncSteps&, std::chrono::milliseconds, bool);
        using AwaitPass = functor_pass::Simple<
                AwaitCallbackSignature,
                functor_pass::DEFAULT_SIZE,
                functor_pass::Function>;
        using AwaitCallback = AwaitPass::Function;

        //---
        using ReferenceStateMap = std::map<futoin::string, any>;
        using StateMap = std::map<
                futoin::string,
                any,
                ReferenceStateMap::key_compare,
                IMemPool::Allocator<ReferenceStateMap::value_type>>;

        class State
        {
        public:
            explicit State(IMemPool& mem_pool) noexcept :
                dynamic_items{StateMap::allocator_type(mem_pool)},
                mem_pool_(&mem_pool)
            {
                catch_trace = [&](const std::exception& /*e*/) noexcept
                {
                    last_exception = std::current_exception();
                };
            }

            using key_type = StateMap::key_type;
            using mapped_type = StateMap::mapped_type;
            using CatchTrace =
                    std::function<void(const std::exception&) noexcept>;
            using UnhandledError = std::function<void(ErrorCode) noexcept>;

            inline mapped_type& operator[](const key_type& key) noexcept
            {
                return dynamic_items[key];
            }

            inline mapped_type& operator[](key_type&& key) noexcept
            {
                return dynamic_items[std::forward<key_type>(key)];
            }

            inline IMemPool& mem_pool() noexcept
            {
                return *mem_pool_;
            }

            StateMap dynamic_items;
            ErrorMessage error_info;
            LoopLabel error_loop_label{nullptr};
            std::exception_ptr last_exception{nullptr};
            CatchTrace catch_trace;
            UnhandledError unhandled_error;
            any promise;

        private:
            IMemPool* mem_pool_;
        };

        struct StepData
        {
            ExecPass::Storage func_storage_;
            ExecPass::Storage func_orig_storage_;
            ErrorPass::Storage on_error_storage_;
            asyncsteps::ExecHandler func_;
            details::functor_pass::StorageBase<sizeof(asyncsteps::ExecHandler)>
                    func_orig_;
            asyncsteps::ErrorHandler on_error_;
        };

        template<bool = false>
        void default_destroy_cb(void* /*ptr*/) noexcept {};
    } // namespace asyncsteps

    /**
     * @brief Synchronization primitive interface.
     */
    class ISync
    {
    public:
        /**
         * @brief std::mutex replacement for single IAsyncTool cases.
         */
        struct NoopOSMutex
        {
            void lock() noexcept
            {
                assert(thread_id == std::this_thread::get_id());
            }

            void unlock() noexcept
            {
                assert(thread_id == std::this_thread::get_id());
            }

            const std::thread::id thread_id{std::this_thread::get_id()};
        };

        ISync() noexcept = default;
        ISync(const ISync&) = delete;
        ISync& operator=(const ISync&) = delete;
        ISync(ISync&&) = delete;
        ISync& operator=(ISync&&) = delete;

        virtual void lock(IAsyncSteps&) = 0;
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
        using ExecPass = asyncsteps::ExecPass;
        using ErrorPass = asyncsteps::ErrorPass;
        using CancelPass = asyncsteps::CancelPass;
        using StackDestroyHandler = void (*)(void*);
        using StepData = asyncsteps::StepData;
        using State = asyncsteps::State;

        template<typename FP>
        using ExtendedExecPass = details::functor_pass::Simple<
                FP,
                details::functor_pass::DEFAULT_SIZE,
                details::functor_pass::Function>;

        using SyncRootID = ptrdiff_t;

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
        template<typename... T, size_t S, template<typename> class F>
        IAsyncSteps&
        add(details::functor_pass::Simple<void(IAsyncSteps&, T...), S, F> func,
            ErrorPass& on_error) noexcept
        {
            StepData& step = add_step();

            // 1. Create holder for actual callback with all parameters
            using OrigFunction = typename decltype(func)::Function;
            auto* orig_fp = new (step.func_orig_.buffer) OrigFunction;
            step.func_orig_.cleanup = [](void* ptr) {
                reinterpret_cast<OrigFunction*>(ptr)->~OrigFunction();
            };

            func.move(*orig_fp, step.func_orig_storage_);

            // 2. Create adapter functor with all allocations in buffers
            auto adapter = [orig_fp](IAsyncSteps& asi) {
                asi.nextargs().once(asi, *orig_fp);
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
                    ExtendedExecPass<FP>(std::forward<Functor>(func)),
                    on_error);
        }

        /**
         * @brief Get pseudo-parallelized IAsyncSteps/
         */
        virtual IAsyncSteps& parallel(ErrorPass on_error = {}) noexcept = 0;

        /**
         * @brief Reference to associated state object.
         */
        virtual State& state() noexcept = 0;

        /**
         * @brief Handy helper to access state variables
         */
        template<typename T>
        T& state(const State::key_type& key)
        {
            return any_cast<T&>(state()[key]);
        }

        /**
         * @brief Handy helper to access state variables with default value
         */
        template<typename T>
        T& state(const State::key_type& key, T&& def_val)
        {
            any& val = state()[key];

            if (!val.has_value()) {
                val = std::forward<T>(def_val);
            }

            return any_cast<T&>(val);
        }

        /**
         * @brief Copy steps from a model step.
         */
        virtual IAsyncSteps& copyFrom(IAsyncSteps& other) noexcept = 0;

        /**
         * @brief Get root step ID for usage in ISync interface
         */
        virtual SyncRootID sync_root_id() const = 0;

        /**
         * @brief Add generic step synchronized against object.
         */
        IAsyncSteps& sync(
                ISync& obj, ExecPass func, ErrorPass on_error = {}) noexcept
        {
            StepData& step = add_sync(obj);
            func.move(step.func_, step.func_storage_);
            on_error.move(step.on_error_, step.on_error_storage_);
            return *this;
        }

        /**
         * @brief Add step which accepts required result variables and
         * synchronized against object.
         */
        template<typename... T, size_t S, template<typename> class F>
        IAsyncSteps&
        sync(ISync& obj,
             details::functor_pass::Simple<void(IAsyncSteps&, T...), S, F> func,
             ErrorPass& on_error) noexcept
        {
            StepData& step = add_sync(obj);

            // 1. Create holder for actual callback with all parameters
            using OrigFunction = typename decltype(func)::Function;
            auto* orig_fp = new (step.func_orig_.buffer) OrigFunction;
            step.func_orig_.cleanup = [](void* ptr) {
                reinterpret_cast<OrigFunction*>(ptr)->~OrigFunction();
            };

            func.move(*orig_fp, step.func_orig_storage_);

            // 2. Create adapter functor with all allocations in buffers
            auto adapter = [orig_fp](IAsyncSteps& asi) {
                asi.nextargs().once(asi, *orig_fp);
            };
            ExecPass adapter_pass{adapter};
            adapter_pass.move(step.func_, step.func_storage_);

            // 3. Simple error callback
            on_error.move(step.on_error_, step.on_error_storage_);

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
                    ExtendedExecPass<FP>(std::forward<Functor>(func)),
                    on_error);
        }

        /**
         * @brief Create a new instance for standalone execution
         */
        virtual std::unique_ptr<IAsyncSteps> newInstance() noexcept = 0;

        /**
         * @brief Wait for external std::future with no result
         */
        void await(std::future<void>&& future)
        {
            using Future = std::future<void>;
            await_impl(FutureWait<Future>(std::forward<Future>(future)));
        }

        /**
         * @brief Wait for external std::future with result
         */
        template<typename Result>
        void await(std::future<Result>&& future)
        {
            using Future = std::future<Result>;
            await_impl(FutureWait<Future>(std::forward<Future>(future)));
        }

        /**
         * @brief Create memory allocation with lifetime of the step.
         */
        virtual void* stack(
                std::size_t object_size,
                StackDestroyHandler destroy_cb =
                        &asyncsteps::default_destroy_cb) noexcept = 0;

        /**
         * @brief Create memory allocation with lifetime of the step.
         */
        template<typename T, typename... Args>
        T& stack(Args&&... args) noexcept
        {
            void* ptr = stack(sizeof(T), [](void* ptr) {
                reinterpret_cast<T*>(ptr)->~T();
            });
            auto tptr = new (ptr) T(std::forward<Args>(args)...);
            return *tptr;
        }
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
        [[noreturn]] void error(
                ErrorCode error, ErrorMessage&& error_info = {}) {
            state().error_info = std::move(error_info);
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

        /**
         * @brief Check if step is in valid state for usage.
         */
        virtual operator bool() const noexcept = 0;

        /**
         * @brief Check if step is in invalid state for usage.
         */
        bool operator!() const noexcept
        {
            return !operator bool();
        }

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
                details::functor_pass::Simple<
                        void(IAsyncSteps&, std::size_t i),
                        asyncsteps::functor_pass::DEFAULT_SIZE,
                        asyncsteps::functor_pass::Function> func,
                asyncsteps::LoopLabel label = nullptr)
        {
            auto& ls = add_loop();

            ls.label = label;
            ls.i = 0;

            typename decltype(func)::Function handler;
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
         * @note Mind lifetime of the container!
         */
        template<
                typename C,
                typename FP,
                typename V = typename C::mapped_type,
                size_t S,
                template<typename> class ImplF,
                bool map = true>
        IAsyncSteps& forEach(
                std::reference_wrapper<C> cr,
                details::functor_pass::Simple<FP, S, ImplF> func,
                asyncsteps::LoopLabel label = nullptr)
        {
            auto& c = cr.get();
            auto iter = std::begin(c);
            auto end = std::end(c);
            using Iter = decltype(iter);

            auto& ls = add_loop();

            ls.label = label;
            ls.data = any(std::move(iter));

            typename decltype(func)::Function handler;
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
         * @note Mind lifetime of the container!
         */
        template<
                typename C,
                typename FP,
                typename V = decltype(C().back()),
                size_t S,
                template<typename> class ImplF>
        IAsyncSteps& forEach(
                std::reference_wrapper<C> cr,
                details::functor_pass::Simple<FP, S, ImplF> func,
                asyncsteps::LoopLabel label = nullptr)
        {
            auto& c = cr.get();
            auto iter = std::begin(c);
            auto end = std::end(c);
            using Iter = decltype(iter);

            auto& ls = add_loop();

            ls.label = label;
            ls.i = 0;
            ls.data = any(std::move(iter));

            typename decltype(func)::Function handler;
            func.move(handler, ls.outer_func_storage);

            ls.set_handler(
                    [handler](asyncsteps::LoopState& ls, IAsyncSteps& as) {
                        // NOLINTNEXTLINE(modernize-use-auto)
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
         * @note Mind lifetime of the container!
         */
        template<
                typename C,
                typename F,
                typename = decltype(&F::operator()),
                typename FP = typename details::StripFunctorClass<F>::type>
        IAsyncSteps& forEach(
                std::reference_wrapper<C> c,
                F func,
                asyncsteps::LoopLabel label = nullptr)
        {
            return forEach(c, ExtendedExecPass<FP>(func), label);
        }

        /**
         * @brief Loop over std::map-like container
         * @note Lifetime of the container is ensured by move!
         */
        template<
                typename C,
                typename FP,
                typename V = typename C::mapped_type,
                size_t S,
                template<typename> class ImplF,
                bool map = true>
        IAsyncSteps& forEach(
                C&& cm,
                details::functor_pass::Simple<FP, S, ImplF> func,
                asyncsteps::LoopLabel label = nullptr)
        {
            auto& ls = add_loop();

            ls.container_data = std::forward<C>(cm);

            auto& c = any_cast<C&>(ls.container_data);
            auto iter = std::begin(c);
            auto end = std::end(c);
            using Iter = decltype(iter);

            ls.label = label;
            ls.data = any(std::move(iter));

            typename decltype(func)::Function handler;
            func.move(handler, ls.outer_func_storage);

            ls.set_handler(
                    [handler](asyncsteps::LoopState& ls, IAsyncSteps& as) {
                        // NOLINTNEXTLINE(modernize-use-auto)
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
         * @note Lifetime of the container is ensured by move!
         */
        template<
                typename C,
                typename FP,
                typename V = decltype(C().back()),
                size_t S,
                template<typename> class ImplF>
        IAsyncSteps& forEach(
                C&& cm,
                details::functor_pass::Simple<FP, S, ImplF> func,
                asyncsteps::LoopLabel label = nullptr)
        {
            auto& ls = add_loop();

            ls.container_data = std::forward<C>(cm);

            auto& c = any_cast<C&>(ls.container_data);
            auto iter = std::begin(c);
            auto end = std::end(c);
            using Iter = decltype(iter);

            ls.label = label;
            ls.i = 0;
            ls.data = any(std::move(iter));

            typename decltype(func)::Function handler;
            func.move(handler, ls.outer_func_storage);

            ls.set_handler(
                    [handler](asyncsteps::LoopState& ls, IAsyncSteps& as) {
                        // NOLINTNEXTLINE(modernize-use-auto)
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
         * @note Lifetime of the container is ensured by copy!
         */
        template<
                typename C,
                typename F,
                typename = decltype(&F::operator()),
                typename FP = typename details::StripFunctorClass<F>::type>
        IAsyncSteps& forEach(C c, F func, asyncsteps::LoopLabel label = nullptr)
        {
            return forEach(std::move(c), ExtendedExecPass<FP>(func), label);
        }

        /**
         * @brief Break async loop.
         */
        [[noreturn]] inline void breakLoop(
                asyncsteps::LoopLabel label = nullptr)
        {
            state().error_loop_label = label;
            throw asyncsteps::LoopBreak(label);
        }

        /**
         * @brief Continue async loop from the next iteration.
         */
        [[noreturn]] inline void continueLoop(
                asyncsteps::LoopLabel label = nullptr)
        {
            state().error_loop_label = label;
            throw asyncsteps::LoopContinue(label);
        }

        ///@}

        /**
         * @name Control API
         * @note Only allowed to be used on root instance.
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

        /**
         * @brief Get native future.
         * @note Either void or single result variable is supported.
         */
        template<typename Result = void>
        std::future<Result> promise() noexcept
        {
            auto& state = this->state();
            using Promise = std::promise<Result>;

            state.promise = Promise();
            auto& promise = futoin::any_cast<Promise&>(state.promise);

            state.unhandled_error = [&](ErrorCode err) {
                auto eptr = std::current_exception();

                if (eptr == nullptr) {
                    promise.set_exception(std::make_exception_ptr(Error(err)));
                } else {
                    promise.set_exception(eptr);
                }
            };
            promise_complete_step(promise);
            execute();

            return promise.get_future();
        }

        ///@}

    protected:
        using AwaitPass = asyncsteps::AwaitPass;

        /**
         * Generic Future wrapping Functor to alloc both
         * resource consuming polling and suspend approach
         * in implementation.
         */
        template<typename Future>
        struct FutureWait
        {
            using Result = decltype(Future().get());

            FutureWait(Future&& future) noexcept :
                future(std::forward<Future>(future))
            {}

            FutureWait(FutureWait&& other) noexcept :
                future(std::move(other.future))
            {}

            inline bool operator()(
                    IAsyncSteps& asi,
                    std::chrono::milliseconds delay,
                    bool complete)
            {
                if (future.wait_for(delay) != std::future_status::ready) {
                    return false;
                }

                if (complete) {
                    complete::done(future, asi);
                }

                return true;
            }

            struct complete_normal
            {
                static inline void done(Future& f, IAsyncSteps& asi)
                {
                    asi(f.get());
                }
            };

            struct complete_void
            {
                static inline void done(Future& f, IAsyncSteps& asi)
                {
                    f.get();
                    asi();
                }
            };

            using complete = typename std::conditional<
                    std::is_same<Result, void>::value,
                    complete_void,
                    complete_normal>::type;

            Future future;
        };

        IAsyncSteps() = default;

        virtual StepData& add_step() = 0;
        virtual void handle_success() = 0;
        virtual void handle_error(ErrorCode) = 0;
        virtual asyncsteps::NextArgs& nextargs() noexcept = 0;
        virtual asyncsteps::LoopState& add_loop() noexcept = 0;
        virtual StepData& add_sync(ISync&) noexcept = 0;
        virtual void await_impl(AwaitPass) noexcept = 0;

    private:
        void promise_complete_step(std::promise<void>& promise)
        {
            add([&](IAsyncSteps&) { promise.set_value(); });
        }

        template<typename Result>
        void promise_complete_step(std::promise<Result>& promise)
        {
            add([&](IAsyncSteps&, Result&& res) {
                promise.set_value(std::forward<Result>(res));
            });
        }
    };
} // namespace futoin

//---
#endif // FUTOIN_IASYNCSTEPS_HPP
