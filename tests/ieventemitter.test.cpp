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

#include <boost/test/unit_test.hpp>

#include <futoin/ieventemitter.hpp>

#include <typeindex>
#include <typeinfo>

class TestEventEmitter : public futoin::IEventEmitter
{
public:
    TestEventEmitter() noexcept = default;

    void on(const EventType& event, EventHandler& handler) noexcept override
    {
        BOOST_CHECK_EQUAL(Accessor::event_emitter(event), this);
        BOOST_CHECK_EQUAL(Accessor::event_id(event), 1);
        Accessor::event_type(handler) = event;
    }
    void once(const EventType& event, EventHandler& handler) noexcept override
    {
        BOOST_CHECK_EQUAL(Accessor::event_emitter(event), this);
        BOOST_CHECK_EQUAL(Accessor::event_id(event), 1);
        Accessor::event_type(handler) = event;
    }
    void off(const EventType& event, EventHandler& handler) noexcept override
    {
        BOOST_CHECK_EQUAL(Accessor::event_emitter(event), this);
        BOOST_CHECK_EQUAL(Accessor::event_id(event), 1);
        Accessor::event_id(handler) = 0;
    }

    void emit(const EventType& event) noexcept override
    {
        BOOST_CHECK_EQUAL(Accessor::event_emitter(event), this);
        BOOST_CHECK_EQUAL(Accessor::event_id(event), 1);
    }
    void emit(const EventType& event, NextArgs&& args) noexcept override
    {
        BOOST_CHECK_EQUAL(Accessor::event_emitter(event), this);
        BOOST_CHECK_EQUAL(Accessor::event_id(event), 1);
        test_cast_(args);
    }

    using IEventEmitter::register_event;

protected:
    void register_event_impl(
            EventType& event, TestCast test_cast) noexcept override
    {
        BOOST_CHECK_EQUAL(Accessor::event_id(event), 0);
        BOOST_CHECK_EQUAL(Accessor::raw_event_type(event), "TestEvent");
        Accessor::event_emitter(event) = this;
        Accessor::event_id(event) = 1;
        test_cast_ = test_cast;
    }

    std::size_t handle_hash_;
    IEventEmitter::EventHandler::TestCast test_cast_;
};

BOOST_AUTO_TEST_SUITE(ieventemitter) // NOLINT

BOOST_AUTO_TEST_CASE(instance) // NOLINT
{
    TestEventEmitter tee;
}

BOOST_AUTO_TEST_CASE(register_event) // NOLINT
{
    TestEventEmitter tee;

    TestEventEmitter::EventType test_event("TestEvent");
    tee.register_event(test_event);
}

BOOST_AUTO_TEST_CASE(on) // NOLINT
{
    TestEventEmitter tee;
    futoin::IEventEmitter& ee = tee;

    TestEventEmitter::EventType test_event("TestEvent");
    tee.register_event(test_event);

    TestEventEmitter::EventHandler handler([]() {});
    ee.on(test_event, handler);
    ee.off(test_event, handler);
}

BOOST_AUTO_TEST_CASE(once) // NOLINT
{
    TestEventEmitter tee;
    futoin::IEventEmitter& ee = tee;

    TestEventEmitter::EventType test_event("TestEvent");
    tee.register_event(test_event);

    TestEventEmitter::EventHandler handler([]() {});
    ee.once(test_event, handler);
    ee.off(test_event, handler);
}

BOOST_AUTO_TEST_CASE(emit) // NOLINT
{
    TestEventEmitter tee;
    futoin::IEventEmitter& ee = tee;

    TestEventEmitter::EventType test_event("TestEvent");
    tee.register_event(test_event);

    ee.emit(test_event);
}

BOOST_AUTO_TEST_CASE(with_args) // NOLINT
{
    TestEventEmitter tee;
    futoin::IEventEmitter& ee = tee;

    TestEventEmitter::EventType test_event1("TestEvent");
    tee.register_event<int>(test_event1);

    TestEventEmitter::EventHandler handler1([](int) {});
    ee.on(test_event1, handler1);
    ee.once(test_event1, handler1);
    ee.emit(test_event1, 123);
    ee.off(test_event1, handler1);

    TestEventEmitter::EventType test_event2("TestEvent");
    tee.register_event<int, futoin::string>(test_event2);

    TestEventEmitter::EventHandler handler2([](int, const futoin::string&) {});
    ee.on(test_event2, handler2);
    ee.once(test_event2, handler2);
    ee.emit(test_event2, 123, "str");
    ee.emit(test_event2, 123, futoin::string{"str"});
    ee.off(test_event2, handler2);

    TestEventEmitter::EventType test_event3("TestEvent");
    tee.register_event<int, futoin::string, std::vector<int>>(test_event3);
    TestEventEmitter::EventHandler handler3(
            [](int, const futoin::string&, const std::vector<int>&) {});
    ee.on(test_event3, handler3);
    ee.once(test_event3, handler3);
    ee.emit(test_event3, 123, "str", std::vector<int>{1, 2, 3});
    ee.off(test_event3, handler3);

    TestEventEmitter::EventType test_event4("TestEvent");
    tee.register_event<int, futoin::string, std::vector<int>, bool>(
            test_event4);
    TestEventEmitter::EventHandler handler4(
            [](int, const futoin::string&, const std::vector<int>&, bool) {});
    ee.on(test_event4, handler4);
    ee.once(test_event4, handler4);
    ee.emit(test_event4, 123, "str", std::vector<int>{1, 2, 3}, true);
    ee.off(test_event4, handler4);
}

BOOST_AUTO_TEST_SUITE_END() // NOLINT
