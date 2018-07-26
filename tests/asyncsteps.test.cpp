// //-----------------------------------------------------------------------------
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
#include "../include/futoin/asyncsteps.hpp"
#include <array>
#include <boost/test/unit_test.hpp>
#include <vector>

using namespace futoin;

struct TestSteps : AsyncSteps {
    TestSteps() : AsyncSteps(state_) {}

    virtual void add_step(
        asyncsteps::ExecHandler &&exec_h,
        asyncsteps::ErrorHandler &&error_h) noexcept override {
        exec_handler_ = std::move(exec_h);
        error_handler_ = std::move(error_h);
    };
    virtual AsyncSteps &
    parallel(asyncsteps::ErrorHandler error_h) noexcept override {
        error_handler_ = std::move(error_h);
        return *this;
    };
    virtual void success() noexcept override{};
    virtual void handle_error(ErrorCode) override{};

    virtual ~TestSteps() noexcept override{};
    virtual asyncsteps::NextArgs &nextargs() noexcept override {
        return next_args_;
    };
    virtual AsyncSteps &copyFrom(AsyncSteps &) noexcept override {
        return *this;
    }

    virtual void setTimeout(std::chrono::milliseconds) noexcept override {}
    virtual void setCancel(asyncsteps::CancelCallback) noexcept override {}
    virtual void waitExternal() noexcept override {}
    virtual void execute() noexcept override {}
    virtual void cancel() noexcept override {}
    virtual void loop_logic(asyncsteps::LoopState &&ls) noexcept override {
        asyncsteps::LoopState &this_ls = loop_state_;
        this_ls = std::forward<asyncsteps::LoopState>(ls);
        exec_handler_ = [&this_ls](AsyncSteps &asi) {
            this_ls.handler(this_ls, asi);
        };
    }

    asyncsteps::NextArgs next_args_;
    asyncsteps::ExecHandler exec_handler_;
    asyncsteps::ErrorHandler error_handler_;
    asyncsteps::State state_;
    asyncsteps::LoopState loop_state_;
};

BOOST_AUTO_TEST_CASE(success_with_args) {
    TestSteps ts;
    AsyncSteps &as = ts;

    as.success(1, 1.0, "str", true);
    as.success(1, 1.0, "str");
    as.success(1, 1.0);
    as.success(1);
    as.success();
    as.success(std::vector<int>());
}

BOOST_AUTO_TEST_CASE(add_with_args) {
    TestSteps ts;
    AsyncSteps &as = ts;

    as.add([](AsyncSteps &) {});

    as.add([](AsyncSteps &) {}, [](AsyncSteps &, ErrorCode) {});

    as.add([](AsyncSteps &, int, double, std::string &&, bool) {});

    as.add([](AsyncSteps &, std::vector<int> &&) {});
}

BOOST_AUTO_TEST_CASE(exec_handlers) {
    TestSteps ts;
    AsyncSteps &as = ts;

    int count = 0;

    as.add([&](AsyncSteps &) { ++count; });
    BOOST_CHECK_EQUAL(count, 0);

    ts.exec_handler_(as);
    BOOST_CHECK_EQUAL(count, 1);

    // Complex
    as.success(1, 1.0, "str", true);
    as.add([&](AsyncSteps &, int, double, std::string &&, bool) { ++count; });
    ts.exec_handler_(as);
    BOOST_CHECK_EQUAL(count, 2);

    // Small object
    as.success(std::vector<int>({1, 2, 3}));
    as.add([&](AsyncSteps &, std::vector<int> &&v) {
        BOOST_CHECK_EQUAL(v[0], 1);
        BOOST_CHECK_EQUAL(v[1], 2);
        BOOST_CHECK_EQUAL(v[2], 3);
        ++count;
    });
    ts.exec_handler_(as);
    BOOST_CHECK_EQUAL(count, 3);

    // Large object
    as.success(std::array<int, 1024>({1, 2, 3}));
    as.add([&](AsyncSteps &, std::array<int, 1024> &&v) {
        BOOST_CHECK_EQUAL(v[0], 1);
        BOOST_CHECK_EQUAL(v[1], 2);
        BOOST_CHECK_EQUAL(v[2], 3);
        ++count;
    });
    ts.exec_handler_(as);
    BOOST_CHECK_EQUAL(count, 4);
}

BOOST_AUTO_TEST_CASE(async_loop) {
    TestSteps ts;
    AsyncSteps &as = ts;

    int count = 0;

    as.loop([&](AsyncSteps &) { --count; });
    as.loop([&](AsyncSteps &) { ++count; }, "Some Label");
    BOOST_CHECK_EQUAL(count, 0);

    const auto max = 100;

    for (int i = max; i > 0; --i) {
        ts.exec_handler_(as);
    }

    BOOST_CHECK_EQUAL(count, max);
}

BOOST_AUTO_TEST_CASE(async_repeat) {
    TestSteps ts;
    AsyncSteps &as = ts;

    int count = 0;
    const auto max = 100;

    as.repeat(max, [&](AsyncSteps &, std::size_t) { --count; });
    as.repeat(
        max,
        [&](AsyncSteps &, std::size_t i) {
            BOOST_CHECK_EQUAL(count, i);
            ++count;
        },
        "Some Label");
    BOOST_CHECK_EQUAL(count, 0);

    for (int i = max; i > 0; --i) {
        ts.exec_handler_(as);
    }

    BOOST_CHECK_EQUAL(count, max);
}

BOOST_AUTO_TEST_CASE(async_forEach_vector) {
    TestSteps ts;
    AsyncSteps &as = ts;

    int count = 0;

    const auto max = 100;
    std::vector<int> vec(max);

    for (int i = 0; i < max; ++i) {
        vec[i] = i;
    }

    as.forEach(
        vec,
        [&](AsyncSteps &, std::size_t i, int &) {
            BOOST_CHECK_EQUAL(count, i);
            ++count;
        },
        "Some Label");

    for (int i = max; i > 0; --i) {
        ts.exec_handler_(as);
    }

    BOOST_CHECK_EQUAL(count, max);
}

BOOST_AUTO_TEST_CASE(async_forEach_array) {
    TestSteps ts;
    AsyncSteps &as = ts;

    int count = 0;

    const auto max = 100;
    std::array<int, max> arr;

    for (int i = 0; i < max; ++i) {
        arr[i] = i;
    }

    const auto &carr = arr;

    as.forEach(
        carr,
        [&](AsyncSteps &, std::size_t i, int) {
            BOOST_CHECK_EQUAL(count, i);
            ++count;
        },
        "Some Label");

    for (int i = max; i > 0; --i) {
        ts.exec_handler_(as);
    }

    BOOST_CHECK_EQUAL(count, max);
}

BOOST_AUTO_TEST_CASE(async_forEach_map) {
    TestSteps ts;
    AsyncSteps &as = ts;

    int count = 0;

    const auto max = 100;
    std::map<std::string, int> map;

    for (int i = 0; i < max; ++i) {
        map[std::to_string(i)] = i;
    }

    auto iter = std::begin(map);

    as.forEach(
        map,
        [&](AsyncSteps &, const std::string &k, const int &v) {
            BOOST_CHECK_EQUAL(k, iter->first);
            BOOST_CHECK_EQUAL(v, iter->second);
            ++iter;
            ++count;
        },
        "Some Label");

    for (int i = max; i > 0; --i) {
        ts.exec_handler_(as);
    }

    BOOST_CHECK_EQUAL(count, max);
}
