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
//! @brief Interface of FTN15 Native Event API
//! @sa https://specs.futoin.org/final/preview/ftn15_native_event.html
//-----------------------------------------------------------------------------

#ifndef FUTOIN_IEVENTEMITTER_HPP
#define FUTOIN_IEVENTEMITTER_HPP
//---
#include "details/reqcpp11.hpp"
//---
#include "./imempool.hpp"
#include "details/erased_func.hpp"
//---
#include <cstdint>

namespace futoin {
    /**
     * @brief Interface of async EventEmitter
     * @note The interface implementation is assumed to be used as mixin
     */
    class IEventEmitter
    {
    protected:
        using NextArgs = details::nextargs::NextArgs;

        struct Accessor;

    public:
        using SizeType = std::uint16_t;
        using EventID = SizeType;
        using RawEventType = const char*;

        static constexpr EventID NO_EVENT_ID = 0U;

        class EventHandler;

        /**
         * @brief Efficient holder of Event ID associated with EventEmitter
         */
        class EventType final
        {
        public:
            EventType(RawEventType event_type) noexcept :
                raw_event_type_(event_type)
            {}

            EventType(EventType&&) noexcept = default;
            EventType& operator=(EventType&&) noexcept = default;
            EventType(const EventType&) noexcept = default;
            EventType& operator=(const EventType&) noexcept = default;

        private:
            EventType() noexcept = default;

            union
            {
                IEventEmitter* ee_;
                RawEventType raw_event_type_;
            };
            EventID event_id_{0};

            friend struct Accessor;
            friend class EventHandler;
        };

        /**
         * @brief Base handler for EventEmitter
         */
        class EventHandler
        {
        public:
            using ErasedFunc = details::ErasedFunc<>;
            using TestCast = ErasedFunc::TestCast;

            template<typename FP>
            using Pass = details::functor_pass::Simple<
                    FP,
                    ErasedFunc::FUNCTOR_SIZE,
                    details::functor_pass::Function,
                    ErasedFunc::FUNCTOR_ALIGN>;

            EventHandler() = default;
            EventHandler(EventHandler&&) noexcept = delete;
            EventHandler& operator=(EventHandler&&) noexcept = delete;
            EventHandler(const EventHandler&) noexcept = delete;
            EventHandler& operator=(const EventHandler&) noexcept = delete;

            template<
                    typename Functor,
                    typename FP =
                            typename details::StripFunctorClass<Functor>::type>
            explicit EventHandler(Functor&& func) noexcept :
                func_(Pass<FP>(std::forward<Functor>(func)))
            {}

            template<typename Functor>
            EventHandler& operator=(Functor&& func) noexcept
            {
                func_ = std::forward<Functor>(func);
                return *this;
            }

            void operator()(const NextArgs& args) noexcept
            {
                func_.repeatable(args);
            }

            TestCast test_cast() const noexcept
            {
                return func_.test_cast();
            }

            const NextArgs& model_args() const noexcept
            {
                return func_.model_args();
            }

            ~EventHandler() noexcept
            {
                if (event_type_.event_id_ != 0) {
                    event_type_.ee_->off(event_type_, *this);
                }
            }

        protected:
            EventType event_type_;
            ErasedFunc func_;

            friend struct Accessor;
        };

        virtual void on(
                const EventType& event, EventHandler& handler) noexcept = 0;
        virtual void once(
                const EventType& event, EventHandler& handler) noexcept = 0;
        virtual void off(
                const EventType& event, EventHandler& handler) noexcept = 0;
        virtual void emit(const EventType& event) noexcept = 0;
        virtual void emit(const EventType& event, NextArgs&& args) noexcept = 0;

        template<typename... T>
        void emit(const EventType& event, T&&... args) noexcept
        {
            NextArgs nextargs;
            nextargs.assign(std::forward<T>(args)...);
            emit(event, std::move(nextargs));
        }

    protected:
        using TestCast = EventHandler::TestCast;

        struct Accessor
        {
            static EventID& event_id(EventType& et)
            {
                return et.event_id_;
            }

            static EventID event_id(const EventType& et)
            {
                return et.event_id_;
            }

            static EventID& event_id(EventHandler& handler)
            {
                return handler.event_type_.event_id_;
            }

            static IEventEmitter*& event_emitter(EventType& et)
            {
                return et.ee_;
            }

            static IEventEmitter* event_emitter(const EventType& et)
            {
                return et.ee_;
            }

            static RawEventType raw_event_type(const EventType& et)
            {
                return et.raw_event_type_;
            }

            static EventType& event_type(EventHandler& handler)
            {
                return handler.event_type_;
            }

            static const EventType& event_type(const EventHandler& handler)
            {
                return handler.event_type_;
            }
        };

        template<typename... T>
        void register_event(EventType& event) noexcept
        {
            EventHandler model([](T...) {});
            register_event_impl(event, model.test_cast(), model.model_args());
        }

        virtual void register_event_impl(
                EventType& event,
                TestCast test_cast,
                const NextArgs& model_args) noexcept = 0;

        virtual ~IEventEmitter() noexcept = default;
    };
} // namespace futoin

//---
#endif // FUTOIN_IEVENTEMITTER_HPP
