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

#define BOOST_TEST_MAIN

#include <boost/test/unit_test.hpp>

#include <array>
#include <vector>

#include <futoin/iasyncsteps.hpp>
#include <futoin/imempool.hpp>

using namespace futoin;

struct TestMemPool : IMemPool
{
    void* allocate(size_t /*object_size*/, size_t /*count*/) noexcept override
    {
        return nullptr;
    }

    void deallocate(
            void* /*ptr*/,
            size_t /*object_size*/,
            size_t /*count*/) noexcept override
    {}

    void release_memory() noexcept override {}
};

struct TestSteps : IAsyncSteps
{
    TestSteps() :
        exec_handler_(step_.func_),
        on_error_handler_(step_.on_error_),
        state_(mem_pool_)
    {}

    asyncsteps::State& state() noexcept override
    {
        return state_;
    }

    StepData& add_step() noexcept override
    {
        return step_;
    };
    IAsyncSteps& parallel(ErrorPass on_error = {}) noexcept override
    {
        on_error.move(step_.on_error_, step_.on_error_storage_);
        return *this;
    };
    void handle_success() noexcept override {}
    void handle_error(ErrorCode /*code*/) override {}

    ~TestSteps() noexcept override = default;
    asyncsteps::NextArgs& nextargs() noexcept override
    {
        return next_args_;
    };
    IAsyncSteps& copyFrom(IAsyncSteps& /*asi*/) noexcept override
    {
        return *this;
    }

    void setTimeout(std::chrono::milliseconds /*to*/) noexcept override {}
    void setCancel(CancelPass /*on_cancel*/) noexcept override {}
    void waitExternal() noexcept override {}
    void execute() noexcept override {}
    void cancel() noexcept override {}
    asyncsteps::LoopState& add_loop() noexcept override
    {
        ExecPass([&](IAsyncSteps& asi) {
            loop_state_.handler(loop_state_, asi);
        })
                .move(step_.func_, step_.func_storage_);

        return loop_state_;
    }
    std::unique_ptr<IAsyncSteps> newInstance() noexcept override
    {
        return std::unique_ptr<IAsyncSteps>(new TestSteps());
    };

    operator bool() const noexcept override
    {
        return true;
    };

    SyncRootID sync_root_id() const override
    {
        return reinterpret_cast<SyncRootID>(this);
    }

    IAsyncSteps::StepData step_;
    asyncsteps::NextArgs next_args_;
    asyncsteps::ExecHandler& exec_handler_;
    asyncsteps::ErrorHandler& on_error_handler_;
    TestMemPool mem_pool_;
    asyncsteps::State state_;
    asyncsteps::LoopState loop_state_;
};

struct TestSync : ISync
{
    void lock(IAsyncSteps& asi) noexcept override
    {
        asi.success();
    }

    void unlock(IAsyncSteps& /*asi*/) noexcept override {}
};

BOOST_AUTO_TEST_CASE(success_with_args) // NOLINT
{
    TestSteps ts;
    IAsyncSteps& as = ts;

    as.success(1, 1.0, "str", true);
    as.success(1, 1.0, "str");
    as.success(1, 1.0);
    as.success(1);
    as.success();
    as.success(std::vector<int>());
}

BOOST_AUTO_TEST_CASE(add_with_args) // NOLINT
{
    TestSteps ts;
    IAsyncSteps& as = ts;

    as.add([](IAsyncSteps&) {});

    as.add([](IAsyncSteps&) {}, [](IAsyncSteps&, ErrorCode) {});

    as.add([](IAsyncSteps&, int, double, std::string&&, bool) {});

    as.add([](IAsyncSteps&, std::vector<int>&&) {});
}

BOOST_AUTO_TEST_CASE(exec_handlers) // NOLINT
{
    TestSteps ts;
    IAsyncSteps& as = ts;

    int count = 0;

    as.add([&](IAsyncSteps&) { ++count; });
    BOOST_CHECK_EQUAL(count, 0);

    ts.exec_handler_(as);
    BOOST_CHECK_EQUAL(count, 1);

    // Complex
    as.success(1, 1.0, "str", true);
    as.add([&](IAsyncSteps&, int, double, std::string&&, bool) { ++count; });
    ts.exec_handler_(as);
    BOOST_CHECK_EQUAL(count, 2);

    // Small object
    as.success(std::vector<int>({1, 2, 3}));
    as.add([&](IAsyncSteps&, std::vector<int>&& v) {
        BOOST_CHECK_EQUAL(v[0], 1);
        BOOST_CHECK_EQUAL(v[1], 2);
        BOOST_CHECK_EQUAL(v[2], 3);
        ++count;
    });
    ts.exec_handler_(as);
    BOOST_CHECK_EQUAL(count, 3);

    // Large object
    as.success(std::array<int, 1024>({1, 2, 3}));
    as.add([&](IAsyncSteps&, std::array<int, 1024>&& v) {
        BOOST_CHECK_EQUAL(v[0], 1);
        BOOST_CHECK_EQUAL(v[1], 2);
        BOOST_CHECK_EQUAL(v[2], 3);
        ++count;
    });
    ts.exec_handler_(as);
    BOOST_CHECK_EQUAL(count, 4);
}

BOOST_AUTO_TEST_CASE(async_loop) // NOLINT
{
    TestSteps ts;
    IAsyncSteps& as = ts;

    int count = 0;

    as.loop([&](IAsyncSteps&) { --count; });
    as.loop([&](IAsyncSteps&) { ++count; }, "Some Label");
    BOOST_CHECK_EQUAL(count, 0);

    const auto max = 100;

    for (int i = max; i > 0; --i) {
        ts.exec_handler_(as);
    }

    BOOST_CHECK_EQUAL(count, max);
}

BOOST_AUTO_TEST_CASE(async_repeat) // NOLINT
{
    TestSteps ts;
    IAsyncSteps& as = ts;

    int count = 0;
    const auto max = 100;

    as.repeat(max, [&](IAsyncSteps&, std::size_t) { --count; });
    as.repeat(
            max,
            [&](IAsyncSteps&, std::size_t i) {
                BOOST_CHECK_EQUAL(count, i);
                ++count;
            },
            "Some Label");
    BOOST_CHECK_EQUAL(count, 0);

    for (int i = max; i > 0; --i) {
        BOOST_CHECK(ts.loop_state_.cond(ts.loop_state_));
        ts.exec_handler_(as);
    }

    BOOST_CHECK_EQUAL(count, max);
    BOOST_CHECK(!ts.loop_state_.cond(ts.loop_state_));
}

BOOST_AUTO_TEST_CASE(async_forEach_vector) // NOLINT
{
    TestSteps ts;
    IAsyncSteps& as = ts;

    int count = 0;

    const auto max = 100;
    std::vector<int> vec(max);

    for (int i = 0; i < max; ++i) {
        vec[i] = i;
    }

    as.forEach(
            vec,
            [&](IAsyncSteps&, std::size_t i, int&) {
                BOOST_CHECK_EQUAL(count, i);
                ++count;
            },
            "Some Label");

    for (int i = max; i > 0; --i) {
        BOOST_CHECK(ts.loop_state_.cond(ts.loop_state_));
        ts.exec_handler_(as);
    }

    BOOST_CHECK_EQUAL(count, max);
    BOOST_CHECK(!ts.loop_state_.cond(ts.loop_state_));
}

BOOST_AUTO_TEST_CASE(async_forEach_array) // NOLINT
{
    TestSteps ts;
    IAsyncSteps& as = ts;

    int count = 0;

    const auto max = 100;
    std::array<int, max> arr;

    for (int i = 0; i < max; ++i) {
        arr[i] = i;
    }

    const auto& carr = arr;

    as.forEach(
            carr,
            [&](IAsyncSteps&, std::size_t i, int) {
                BOOST_CHECK_EQUAL(count, i);
                ++count;
            },
            "Some Label");

    for (int i = max; i > 0; --i) {
        BOOST_CHECK(ts.loop_state_.cond(ts.loop_state_));
        ts.exec_handler_(as);
    }

    BOOST_CHECK_EQUAL(count, max);
    BOOST_CHECK(!ts.loop_state_.cond(ts.loop_state_));
}

BOOST_AUTO_TEST_CASE(async_forEach_map) // NOLINT
{
    TestSteps ts;
    IAsyncSteps& as = ts;

    int count = 0;

    const auto max = 100;
    std::map<std::string, int> map;

    for (int i = 0; i < max; ++i) {
        map[std::to_string(i)] = i;
    }

    auto iter = std::begin(map);

    as.forEach(
            map,
            [&](IAsyncSteps&, const std::string& k, const int& v) {
                BOOST_CHECK_EQUAL(k, iter->first);
                BOOST_CHECK_EQUAL(v, iter->second);
                ++iter;
                ++count;
            },
            "Some Label");

    for (int i = max; i > 0; --i) {
        BOOST_CHECK(ts.loop_state_.cond(ts.loop_state_));
        ts.exec_handler_(as);
    }

    BOOST_CHECK_EQUAL(count, max);
    BOOST_CHECK(!ts.loop_state_.cond(ts.loop_state_));
}

BOOST_AUTO_TEST_CASE(async_error) // NOLINT
{
    TestSteps ts;
    IAsyncSteps& as = ts;

    BOOST_CHECK_THROW(as.error("Some Code"), futoin::Error);
    BOOST_CHECK_THROW(as.error("Some Code", "Some message"), futoin::Error);
}

BOOST_AUTO_TEST_CASE(async_loop_control) // NOLINT
{
    TestSteps ts;
    IAsyncSteps& as = ts;

    BOOST_CHECK_THROW(as.breakLoop(), asyncsteps::LoopBreak);

    BOOST_CHECK_THROW(as.breakLoop("Some Label"), asyncsteps::LoopBreak);

    BOOST_CHECK_THROW(as.continueLoop(), asyncsteps::LoopContinue);

    BOOST_CHECK_THROW(as.continueLoop("Some Label"), asyncsteps::LoopContinue);
}

BOOST_AUTO_TEST_CASE(sync_obj) // NOLINT
{
    TestSteps ts;
    TestSync mtx;
    IAsyncSteps& as = ts;

    as.sync(mtx, [](IAsyncSteps&) {});
    as.sync(mtx, [](IAsyncSteps&) {}, [](IAsyncSteps&, ErrorCode) {});
    as.sync(mtx, [](IAsyncSteps&, int, double, std::string&&, bool) {});
    as.sync(mtx, [](IAsyncSteps&, std::vector<int>&&) {});
}
