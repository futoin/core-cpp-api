
### FutoIn C++ API

This is a primary FutoIn library for loose coupling of FutoIn users and
actual implementations of FutoIn concepts.

Current focus is still on C++11 to rely on well established and tested versions
of toolchain.

#### Concepts

* [**FTN12: AsyncSteps**](https://futoin.org/docs/asyncsteps/)
    - See `futoin::IAsyncSteps` interface and helpers.
    - Alternative to coroutines.
    - `FutoInAsyncSteps` and `FutoInAsyncStepsAPI` is binary C/Assembly-level
        AsyncSteps interface for true cross-technology support.
    - `FutoInSync` and `FutoInSyncAPI` is counterpart for AsyncSteps sync objects.
* [**FTN15: Native Event API**](https://specs.futoin.org/final/preview/ftn15_native_event.html)
    - See `futoin::IEventEmitter` interface and helpers.
    - Type-erased alternative to signal and slot systems.
    - Ensures listener execution outside of emitter stack.

#### Helpers

* `futoin::any` - C++17 std::any optimized for use case in FutoIn & C++11
* `futoin::FatalMsg` - fatal error stream with std::terminate on d-tor
* `futoin::Error` & `futoin::errors`
* `futoin::IAsyncTool` - interface of event loop
* `futoin::IMemPool` - concept of memory pools for C++
* `FutoInBinaryValue` - universal Plain-Old-Data as an intermediate storage for
    Binary AsyncSteps argument
* `FutoInArgs` - a collection of FutoInBinaryValue arguments

#### Private API helpers

* `futoin::details::nextargs::smart_forward<T>` - like `std::forward`, but with conversion
    of C-strings to `futoin::string`.
* `futoin::details::nextargs::NextArgs` - type erasure of function call parameters.
* `futoin::details::ErasedFunc` - complete type erasure of function signature.
* `futoin::details::functor_pass` - namespace of strict efficient functor type erasure.
    - `Storage<Size, Alignment>` - type-agnostic storage for Functor
    - `Function<Signature, ...>` - lighter `std::function` replacement
    - `Simple<Signature,Size,FunctionTpl=std::function,Alignment>` - proxy object to
        capture raw function pointers, pointers to members and functors without
        any copy or move operations.
* `futoin::details::StripFunctorClass<T>` - extracts plain function signature.
* `futoin::details::moveHelper<T,Any>` - helpers for conversion from `FutoInBinaryValue`
    to `futoin::any` and vice-versa.

### Usage

The primary purpose for standalone API project is allow creating implementation-agnostic
business logic in C++ which can run on various FutoIn implementations. Therefore,
please check actual reference implementations for complete usage examples.

#### Usage of `IAsyncSteps` interface

`IAsyncSteps` is native C++ interface of FutoIn AsyncSteps ([FTN12][]) concept. It's the only
all-in interface required to define business logic with asynchronous steps which may
run as coroutines, native threads or any other implementation-defined way.

Unlike various coroutine for C++ implementations, FutoIn AsyncSteps require no compiler-side
support and/or architecture-specific Assembler code. It's friendly to any kind of dynamic
security system.


```c++

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

    // 14. Allocation with lifetime of the step
    {
        struct MyType {
            MyType() = default;
            MyType(int a) : a(a) {};
            int a{0};
        };

        // Allocated Async Stack memory
        auto& mytype = asi.stack<MyType>();
        auto& mytype2 = asi.stack<MyType>(2);

        asi.add([&](IAsyncSteps& asi) {
            mytype.a = 1; // It's safe!
            assert(mytype2.a == 2);
        });
    }
}
```

#### Usage of `IEventEmitter` interface

Unlike more known ECMAScript EventEmitter interfaces, C++ version is much more strict:

1. Events must be pre-registered with strict parameter types.
    - Event name is an arbitrary string with ID-based runtime optimization.
2. All handlers must be represented by `IEventEmitter::EventHandler` objects for whole lifetime
    of active handler. Otherwise, it gets automatically disconnected.
    - Handlers can be either of persistent or "once" type.
    - The same handler instance cannot listen to more than one event at a time.
    - Freed handler can be re-used after `off()` or `once()+emit()` calls.
3. `IEventEmitter::EventType` object used during registration should be copied or used further
    for performance reasones to avoid name-based event lookup.
4. Handler's parameter signature must match event signature, but handler is allowed to omit last parameters.
    - This provides future compatibility for extended event parameters.
    - `NextArgs` feature is used internally for argument checking and passing.
    - Language casts are implicitly allowed (e.g. `futoin::string` -> `const futoin::string&`)
5. Max listeners warning per event and per handler type.

Primary benefits over more traditional signal & slot systems in C++:

- It's possible to add new events without changing public ABI.
- Implementation may be backed by distributed event system.
- Handler signature is quite flexible and does not need to strictly match event signature.
- All handlers are called after emitting business logic completes - no deadlocks and/or
    surprising callbacks from handlers.

Example:

```cpp
#include <futoin/ieventemitter.hpp>

void emitter_example() {
    // NOTE: some implementation must be used by fact
    class MyClass : public futoin::IEventEmitter
    {
    public:
        MyClass() {
            register_event<int, futoin::string, bool, float>(first_event);
            register_event<int, int, std::vector<int>>(second_event);
            setMaxListeners(*this, 100);
        }
        
        void fire_events() {
            emit(first_event, 1, "str", true, 1.0);
            emit(second_event, 1, 2, {3, 4, 5});
        }
        
        // Some suggested variant for exposure of event
        // to use in efficient calls without name-based lookup.
        const EventType& FIRST_EVENT() const {
            return first_event;
        }
        
    private:
        EventType first_event{"FirstEvent"};
        EventType second_event{"SecondEvent"};
    };
}

void listen_example() {
    using futoin::IEventEmitter;

    IEventEmitter &ee = get_some_way();
    
    // 1. Persistent handler with full parameters.
    IEventEmitter::EventHandler first_handler(
            [](int, const futoin::string&, bool, float){});
    ee.on("FirstEvent", first_handler);
    
    // 2. Once handler with shorter list of parameters.
    IEventEmitter::EventHandler first_short_handler(
            [](int, futoin::string){});
    ee.once("FirstEvent", first_short_handler);
    
    // 3. Unlisten example
    
    // 3.1. A bit redundant, but follow FTN15 signature
    ee.off("FirstEvent", first_handler);
    
    // 3.2. Automatic on end of lifetime
    {
        IEventEmitter::EventHandler tmp;
        ee.on("FirstEvent", tmp);
        // implicit ee.off() from d-tor
    }
}
```


#### `futoin::any`

This is implementation of C++17 `std::any` with ensured optimization for small objects
and integration with `IMemPool`.

#### `futoin::FatalMsg`

```cpp
FatalMsg() << "Any std::ostream stuff." << std::endl << "And more";
// std::terminate() on destruct
```

#### `futoin::string`

`IMemPool`-aware instances of `std::basic_string<>`:

- `futoin::string` with `char` type
- `futoin::u16string` with `char16_t` type
- `futoin::u32string` with `char32_t` type

#### `FutoInAsyncSteps`, `FutoInSync`, `FutoInArgs` and `FutoInBinaryValue`

The low-level binary interface has been designed and implemented with the idea to close the gap
for cross-technology support where sophisticated C++ approach may not be convenient, especially
when argument passing types may not be known in advance or may not map so easily to specific
technology types.

It is strongly encouraged to use C++ interface because this binary interface is designed mostly as a
technology-agnostic wrapper, which can be easily used even in plain Assembly. It has some
performance and type safety drawbacks therefore.

`FutoInBinaryValue` can be relatively easily converted to and from `futoin::any`
using `futoin::details::moveHelper`. It is designed to represent data both as Plain-Old-Data (POD), but
also to keep efficient technology-specific representation. Non-fundamentals types, which cannot be
converted to POD are kept only as custom object pointers. Each instance supports a cleanup handler,
which gets automatically invoked from `futoin_reset_binval()` helper. Reference implementation
of binary AsyncSteps interface use that internally.

Technology-agnostic interface must use only fundamental types: signed and unsigned integers of 8,
16, 32 and 64 bits in width, floats, doubles, booleans, strings of 8, 16 and 32 bit in character width,
and dynamic arrays with the same fundamental element types (std::vector in C++ case).

#### Memory Management

Both modern glibc and gperftools malloc (tcmalloc) implementations are quite fast and may show even better
results for some use cases. However, FutoIn API provides `IMemPool` interface to support
implementation-defined way to optimize object allocation and/or control memory usage. In some cases, it
can help to completely eliminate synchronization of independent threads in large and complex projects.

`IMemPool::Allocator<T>` is compatible with standard C++ library allocator interface. It can be
used in any container and standalone. Unless explicitly provided, a global thread-local pointer
to `IMemPool` instance is used. It is set to use standard heap implementation by default.

General idea is that `IAsyncTool` instance can set own custom memory pool for its event loop thread.
Such memory pool may disable any synchronization overhead (~20-30% boost in some tests) and/or
use dedicated pools per object size to minimize fragmentation of heap and improve performance.

It's possible to create static object of `IMemPool::Allocator<T>::EnsureOptimized` type in
any part of the program. This will trigger special logic to try to use special memory pools
optimize for `sizeof(T)`, if supported by implementation.

The minor drawback is that each container instance grows for `sizeof(void*)` as allocator
instance holds pointer to associated memory pool instance. That's not critical in most cases.

See https://futoin.org/docs/misccpp/mempool/ for more info.


[FTN12]: https://specs.futoin.org/final/preview/ftn12_async_api.html


