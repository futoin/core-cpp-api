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
//! @brief Memory pool interface
//-----------------------------------------------------------------------------

#ifndef FUTOIN_IMEMPOOL_HPP
#define FUTOIN_IMEMPOOL_HPP
//---
#include <cstdint>

namespace futoin {
    /**
     * @brief Internal of type-erased memory pool
     */
    struct IMemPool
    {
        IMemPool() noexcept = default;
        IMemPool(const IMemPool&) = delete;
        IMemPool& operator=(const IMemPool&) = delete;
        IMemPool(IMemPool&&) = delete;
        IMemPool& operator=(IMemPool&&) = delete;

        virtual ~IMemPool() noexcept = default;

        /**
         * @brief Efficient in-thread memory allocation
         */
        virtual void* allocate(
                std::size_t object_size, std::size_t count) noexcept = 0;

        /**
         * @brief Free memory returned by allocate()
         */
        virtual void deallocate(
                void* ptr,
                std::size_t object_size,
                std::size_t count) noexcept = 0;

        /**
         * @brief Free any unused memory
         */
        virtual void release_memory() noexcept = 0;

        template<typename T>
        class Allocator;
    };

    /**
     * @brief Passthrough to default new/delete()
     */
    struct PassthroughMemPool : IMemPool
    {
        void* allocate(
                std::size_t object_size, std::size_t count) noexcept override
        {
            return ::new char[object_size * count];
        }

        /**
         * @brief Free memory returned by allocate()
         */
        void deallocate(
                void* ptr,
                std::size_t /*object_size*/,
                std::size_t /*count*/) noexcept override
        {
            ::delete[] reinterpret_cast<char*>(ptr);
        }

        /**
         * @brief Free any unused memory
         */
        void release_memory() noexcept override{};
    };

    template<bool>
    class GlobalMemPoolT
    {
    public:
        inline static IMemPool& get_default() noexcept
        {
            return *local;
        }

        inline static void set_thread_default(IMemPool& mem_pool) noexcept
        {
            local = &mem_pool;
        }

    private:
        static PassthroughMemPool global;
        static thread_local IMemPool* local;
    };

    template<bool B>
    PassthroughMemPool GlobalMemPoolT<B>::global;
    template<bool B>
    thread_local IMemPool* GlobalMemPoolT<B>::local =
            &GlobalMemPoolT<B>::global;

    using GlobalMemPool = GlobalMemPoolT<true>;

    /**
     * @brief STL-compliant allocator
     */
    template<typename T>
    class IMemPool::Allocator
    {
    public:
        using value_type = T;
        using pointer = value_type*;
        using const_pointer = const value_type*;
        using reference = value_type&;
        using const_reference = const value_type&;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;

        explicit Allocator(IMemPool& mem_pool) noexcept : mem_pool(&mem_pool) {}
        Allocator(const Allocator&) noexcept = default;
        Allocator& operator=(const Allocator&) noexcept = default;
        Allocator(Allocator&&) noexcept = default;
        Allocator& operator=(Allocator&&) noexcept = default;

        Allocator() : mem_pool(&(GlobalMemPool::get_default())) {}

        template<typename OT>
        explicit Allocator(const Allocator<OT>& other) noexcept :
            mem_pool(other.mem_pool)
        {}

        pointer allocate(size_type n) noexcept
        {
            return reinterpret_cast<pointer>(mem_pool->allocate(sizeof(T), n));
        }

        pointer allocate(size_type n, const_pointer /*ignore*/) noexcept
        {
            return reinterpret_cast<pointer>(mem_pool->allocate(sizeof(T), n));
        }

        void deallocate(pointer p, size_type n) noexcept
        {
            mem_pool->deallocate(p, sizeof(T), n);
        }

        bool operator==(Allocator& other) noexcept
        {
            return mem_pool == other.mem_pool;
        }

        bool operator!=(Allocator& other) noexcept
        {
            return mem_pool != other.mem_pool;
        }

    private:
        template<typename OT>
        friend class Allocator;

        IMemPool* mem_pool;
    };
} // namespace futoin

//---
#endif // FUTOIN_IMEMPOOL_HPP
