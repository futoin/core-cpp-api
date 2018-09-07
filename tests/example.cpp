
// The only header needed.
#include <futoin/iasyncsteps.hpp>

// Just for examples
#include <list>
#include <vector>

extern void do_some_non_blocking_stuff();
extern void this_code_is_not_executed();
extern void this_code_IS_executed();
extern futoin::ISync& get_some_synchronization_object();
extern int schedule_external_callback(std::function<void(bool)>);
extern void external_cancel(int);

// Just for shorter code
using futoin::ErrorCode;
using futoin::IAsyncSteps;

// Example of entry point
void example_business_logic(IAsyncSteps& asi)
{
    // 1. regular step example
    //
    // Prototypes:
    // - asi.add(func(asi));
    // - asi.add(func(asi), on_error(asi, code));
    // - asi.add(func(asi, [A [,B [,C [,D]]]]));
    // - asi.add(func(asi, [A [,B [,C [,D]]]]), on_error(asi, code));
    //
    asi.add([](IAsyncSteps& asi) { do_some_non_blocking_stuff(); });

    // 2. try {} catch {} block example
    asi.add(
            [](IAsyncSteps& asi) {
                // regular step example
                do_some_non_blocking_stuff();

                asi.error("MyError"); // throw error

                this_code_is_not_executed();
            },
            [](IAsyncSteps& asi, ErrorCode code) {
                if (code == "MyError") {
                    // Override error unwind
                    asi.success(); // or just asi()
                } else {
                    asi.error("OverrideErrorCode");
                }
            });

    // 3. Inner steps
    asi.add(
            [](IAsyncSteps& asi) {
                // regular step example
                do_some_non_blocking_stuff();

                asi.add([](IAsyncSteps& asi) {
                    asi.error("MyError"); // throw error
                });

                // NOTE: inner steps are executed AFTER outer step body
                this_code_IS_executed();
            },
            [](IAsyncSteps& asi, ErrorCode code) {
                if (code == "MyError") {
                    asi();
                }
            });

    // 4. Passing arbitrary parameters
    asi.add([](IAsyncSteps& asi) {
        asi.success(123, true, "SomeString", std::vector<int>({1, 2, 3}));
        // or just asi(...);
    });
    asi.add([](IAsyncSteps& asi,
               int a,
               bool b,
               futoin::string&& c,
               std::vector<int>&& d) {
        // NOTE: Maximum of 4 arguments is supported based on best practices
        assert(a == 123);
        assert(b);
        assert(c == "SomeString");
        assert(d[0] == 1);
    });
    asi.add([](IAsyncSteps& asi, int a, bool b) {
        // Only C++-specific - result variables can be re-used in following
        // steps. NOTE: it's undefined behavior in canonical specification.
        assert(a == 123);
        assert(b);
    });

    // 5. state() - Thread Local Storage emulation
    //
    // Some predefined properties are set directly on State object for
    // performance reasons.
    //
    // Business logic can use custom dynamic items as associative key-value map.
    // Key is string, but value is of futoin::any type.
    {
        // get reference to state object
        auto& state = asi.state();

        // Get set-default variable
        auto some_var = asi.state("SomeVar", 123);
        asi.state("SomeVar", 234);

        // Check the var is saved only on first attempt
        assert(some_var == futoin::any_cast<int>(state["SomeVar"]));
        // Same, but more clean way
        assert(some_var == asi.state<int>("SomeVar"));

        // Set something more complex
        using V = std::vector<int>;
        state["SomeVector"] = V({1, 2, 3});
        auto& v = asi.state<V>("SomeVector");

        asi.add([](IAsyncSteps& asi) {
            asi.state<V>("SomeVector");
            asi.state<int>("SomeVar");

            try {
                asi.state<V>("SomeVar");
            } catch (const std::bad_cast& e) {
                //...
            }
        });
    }

    // 6. Advanced error handling
    asi.state().unhandled_error = [](ErrorCode code) {
        // This handler would be called instead of default
        // std::terminate(), if unhandled error is thrown.
    };

    asi.add(
            [](IAsyncSteps& asi) {
                // NOTE: error codes are associative, but not regular integers
                // to be
                //       more network-friendly.
                asi.error("MyError", "Some arbitrary description of the error");
            },
            [](IAsyncSteps& asi, ErrorCode code) {
                // ErrorCode is wrapper around const char*
                assert(code == "MyError");
                // Error info is stored in state
                assert(asi.state().error_info
                       == "Some arbitrary description of the error");
                // Last exception thrown is also available in state
                std::exception_ptr e = asi.state().last_exception;
            });

    // 7. Synchronization
    //
    // Unlike hardware race conditions, AsyncSteps synchronization serves
    // logical purposes to limit concurrency of execution or rate of calls or
    // both.
    //
    // FTN12 concept defines Mutex, Throttle and Limiter primitives which
    // implement a single ISync interface.

    futoin::ISync& obj = get_some_synchronization_object();

    // The same interface as asi.add(), but with extra synchronization object
    // parameter.
    asi.sync(
            obj,
            [](IAsyncSteps& asi) {},
            [](IAsyncSteps& asi, ErrorCode code) {} // optional
    );

    // 8. Loops
    asi.loop([](IAsyncSteps& asi) {
        // infinite loop
    });
    asi.repeat(10, [](IAsyncSteps& asi, size_t i) {
        // range loop from i=0 till i=9 (inclusive)
    });
    asi.forEach(
            std::vector<int>{1, 2, 3}, [](IAsyncSteps& asi, size_t i, int v) {
                // Iteration of vector-like and list-like objects
            });
    asi.forEach(
            std::list<futoin::string>{"1", "2", "3"},
            [](IAsyncSteps& asi, size_t i, const futoin::string& v) {
                // Iteration of vector-like and list-like objects
            });
    asi.forEach(
            std::map<futoin::string, futoin::string>(),
            [](IAsyncSteps& asi,
               const futoin::string& key,
               const futoin::string& v) {
                // Iteration of map-like objects
            });

    std::map<futoin::string, futoin::string> non_const_map;
    asi.forEach(
            non_const_map,
            [](IAsyncSteps& asi, const futoin::string& key, futoin::string& v) {
                // Iteration of map-like objects, note the value reference type
            });

    // 9. Timeout support
    asi.add(
            [](IAsyncSteps& asi) {
                // Raises Timeout error after specified period
                asi.setTimeout(std::chrono::seconds{10});

                asi.loop([](IAsyncSteps& asi) {
                    // infinite loop
                });
            },
            [](IAsyncSteps& asi, ErrorCode code) {
                if (code == futoin::errors::Timeout) {
                    asi();
                }
            });

    // 10. External event integration
    asi.add([](IAsyncSteps& asi) {
        auto handle = schedule_external_callback([&](bool err) {
            if (err) {
                try {
                    asi.error("ExternalError");
                } catch (...) {
                    // pass
                }
            } else {
                asi.success();
            }
        });

        asi.setCancel([=](IAsyncSteps& asi) { external_cancel(handle); });
    });
    asi.add([](IAsyncSteps& asi) {
        auto handle = schedule_external_callback([&](bool err) {
            if (!asi) {
                // AsyncSteps object is invalidated due to external cancel.
                //
                // However, most likely this would lead to corrupted
                // memory read.
                //
                // Such approach makes sense only for technologies with
                // Garbage Collection without explicit heap management.
                //
                // Scheduled external callback must be manually canceled
                // before AsyncSteps flow is canceled.
            } else if (err) {
                try {
                    asi.error("ExternalError");
                } catch (...) {
                    // pass
                }
            } else {
                asi.success();
            }
        });

        // alias for setCancel() with noop handler
        asi.waitExternal();
    });

    // 11. Standard Promise/Await integration
    asi.add([](IAsyncSteps& asi) {
        // Proper way to create new AsyncSteps instances
        // without hard dependency on implementation.
        auto new_steps = asi.newInstance();
        new_steps->add([](IAsyncSteps& asi) {});

        // Can be called outside of AsyncSteps event loop
        // new_steps.promise().wait();
        //  or
        // new_steps.promise<int>().get();

        // Proper way to wait for standard std::future
        asi.await(new_steps->promise());

        // Ensure instance lifetime
        asi.state()["some_obj"] = std::move(new_steps);
    });

    // 12. Parallel execution
    //
    // It's designed for concurrent execution of sub-flows
    // with shared state().
    //
    // Unhandled error in sub-flows lead to abort of all non-executed
    // parallel steps.
    using OrderVector = std::vector<int>;
    asi.state("order", OrderVector{});

    auto& p = asi.parallel([](IAsyncSteps& asi, ErrorCode) {
        // Overall error handler
        asi.success();
    });
    p.add([](IAsyncSteps& asi) {
        // regular flow
        asi.state<OrderVector>("order").push_back(1);

        asi.add([](IAsyncSteps& asi) {
            asi.state<OrderVector>("order").push_back(4);
        });
    });
    p.add([](IAsyncSteps& asi) {
        asi.state<OrderVector>("order").push_back(2);

        asi.add([](IAsyncSteps& asi) {
            asi.state<OrderVector>("order").push_back(5);
            asi.error("SomeError");
        });
    });
    p.add([](IAsyncSteps& asi) {
        asi.state<OrderVector>("order").push_back(3);

        asi.add([](IAsyncSteps& asi) {
            asi.state<OrderVector>("order").push_back(6);
        });
    });

    asi.add([](IAsyncSteps& asi) {
        asi.state<OrderVector>("order"); // 1, 2, 3, 4, 5
    });

    // 13. Control of AsyncSteps flow
    {
        // std::unique_ptr<IAsyncSteps> with newly allocated instance
        auto new_steps = asi.newInstance();

        // Add steps
        new_steps->add([](IAsyncSteps& asi) {});
        new_steps->loop([](IAsyncSteps& asi) {});

        // Schedule execution of AsyncSteps flow
        new_steps->execute();

        // Cancel execution of AsyncSteps flow
        new_steps->cancel();
    }
}
