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
//! @brief AsyncTool interface definition to be used in FTN12
//! @sa https://specs.futoin.org/final/preview/ftn12_async_api.html
//-----------------------------------------------------------------------------

#ifndef FUTOIN_IASYNCTOOL_HPP
#define FUTOIN_IASYNCTOOL_HPP
//---
#include "details/reqcpp11.hpp"
//---
#include <chrono>
#include <functional>
//---
#include "details/functor_pass.hpp"
#include "imempool.hpp"

namespace futoin {
    /**
     * @brief Interface of async reactor.
     */
    class IAsyncTool
    {
    public:
        using CallbackSignature = void();
        using CallbackPass = details::functor_pass::Simple<
                CallbackSignature,
                details::functor_pass::REDUCED_SIZE,
                details::functor_pass::Function>;
        using Callback = CallbackPass::Function;
        using HandleCookie = std::ptrdiff_t;

    protected:
        struct HandleAccessor;
        struct InternalHandle
        {
            InternalHandle() noexcept = default;

            InternalHandle(InternalHandle&& other) noexcept = delete;
            InternalHandle& operator=(InternalHandle&& other) noexcept = delete;

            InternalHandle(const InternalHandle& other) = delete;
            InternalHandle& operator=(const InternalHandle& other) = delete;

            Callback callback;
            CallbackPass::Storage storage;
        };

    public:
        IAsyncTool() = default;
        IAsyncTool(const IAsyncTool&) = delete;
        IAsyncTool& operator=(const IAsyncTool&) = delete;
        IAsyncTool(const IAsyncTool&&) = delete;
        IAsyncTool& operator=(const IAsyncTool&&) = delete;
        virtual ~IAsyncTool() noexcept = default;

        /**
         * @brief Handle to scheduled callback
         */
        class Handle
        {
        public:
            Handle(InternalHandle& internal,
                   IAsyncTool& async_tool,
                   HandleCookie cookie) noexcept :
                internal_(&internal),
                async_tool_(&async_tool),
                cookie_(cookie)
            {}

            Handle() = default;

            Handle(const Handle&) = delete;
            Handle& operator=(const Handle&) = delete;

            Handle(Handle&& other) noexcept = default;
            Handle& operator=(Handle&& other) noexcept = default;

            ~Handle() noexcept = default;

            void cancel() noexcept
            {
                if (internal_ != nullptr) {
                    async_tool_->cancel(*this);
                }
            }

            void reset() noexcept
            {
                internal_ = nullptr;
            }

            operator bool() const noexcept
            {
                return (internal_ != nullptr)
                       && async_tool_->is_valid(*const_cast<Handle*>(this));
            }

        private:
            friend struct IAsyncTool::InternalHandle;
            friend struct IAsyncTool::HandleAccessor;

            InternalHandle* internal_{nullptr};
            IAsyncTool* async_tool_{nullptr};
            HandleCookie cookie_{0};
        };

        /**
         * @brief Schedule immediate callback
         */
        virtual Handle immediate(CallbackPass&& cb) noexcept = 0;

        /**
         * @brief Schedule deferred callback
         */
        virtual Handle deferred(
                std::chrono::milliseconds delay,
                CallbackPass&& cb) noexcept = 0;

        /**
         * @brief Check if the same thread as internal event loop
         */
        virtual bool is_same_thread() noexcept = 0;

        /**
         * @brief Result of one internal cycle
         */
        struct CycleResult
        {
            CycleResult(
                    bool have_work, std::chrono::milliseconds delay) noexcept :
                delay(delay),
                have_work(have_work)
            {}

            std::chrono::milliseconds delay;
            bool have_work;
        };

        /**
         * @brief Iterate a cycle of internal loop.
         * @note For integration with external event loop.
         */
        virtual CycleResult iterate() noexcept = 0;

        /**
         * @brief IMemPool interface
         */
        virtual IMemPool& mem_pool(
                std::size_t object_size = 1,
                bool optimize = false) noexcept = 0;

        /**
         * @brief Try to release as much of unused allocated memory as possible
         */
        virtual void release_memory() noexcept = 0;

    protected:
        struct HandleAccessor
        {
            HandleAccessor(Handle& handle) : handle(handle) {}

            Handle& handle;

            InternalHandle*& internal()
            {
                return handle.internal_;
            }

            IAsyncTool*& async_tool()
            {
                return handle.async_tool_;
            }

            HandleCookie& cookie()
            {
                return handle.cookie_;
            }
        };

        virtual void cancel(Handle& h) noexcept = 0;
        virtual bool is_valid(Handle& h) noexcept = 0;
    };
} // namespace futoin

//---
#endif // FUTOIN_IASYNCTOOL_HPP
