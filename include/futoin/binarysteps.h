//-----------------------------------------------------------------------------
//   Copyright 2023 FutoIn Project
//   Copyright 2023 Andrey Galkin
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
//! @brief Universal native IAsyncSteps (FTN12) binary interface
//! @sa https://specs.futoin.org/final/preview/ftn12_async_api.html
//-----------------------------------------------------------------------------

#ifndef FUTOIN_BINARYSTEPS_H
#define FUTOIN_BINARYSTEPS_H

#include <stddef.h>
#include <stdint.h>

#include "./binaryval.h"

//---
#ifdef __cplusplus
extern "C" {
#endif

typedef struct FutoInAsyncStepsAPI_ FutoInAsyncStepsAPI;
typedef struct FutoInAsyncSteps_ FutoInAsyncSteps;
typedef struct FutoInSyncAPI_ FutoInSyncAPI;
typedef struct FutoInSync_ FutoInSync;
typedef struct FutoInArgs_ FutoInArgs;
typedef struct FutoInHandle_ FutoInHandle;

struct FutoInArgs_
{
    union
    {
        struct
        {
            FutoInBinaryValue arg0;
            FutoInBinaryValue arg1;
            FutoInBinaryValue arg2;
            FutoInBinaryValue arg3;
        };
        FutoInBinaryValue args[4];
    };
};

struct FutoInHandle_
{
    void* data1;
    void* data2;
    ptrdiff_t data3;
};

typedef void (*FutoInAsyncSteps_execute_callback)(
        FutoInAsyncSteps* bsi, void* data, const FutoInArgs* args);
typedef void (*FutoInAsyncSteps_error_callback)(
        FutoInAsyncSteps* bsi, void* data, const char* code);
typedef void (*FutoInAsyncSteps_cancel_callback)(
        FutoInAsyncSteps* bsi, void* data);

struct FutoInAsyncStepsAPI_
{
    union
    {
        struct
        {
            // Index 0
            void (*add)(
                    FutoInAsyncSteps* bsi,
                    void* data,
                    FutoInAsyncSteps_execute_callback f,
                    FutoInAsyncSteps_error_callback eh);
            // Index 1
            FutoInAsyncSteps* (*parallel)(
                    FutoInAsyncSteps* bsi,
                    void* data,
                    FutoInAsyncSteps_error_callback eh);
            // Index 2
            void* (*stateVariable)(
                    FutoInAsyncSteps* bsi,
                    void* data,
                    const char* name,
                    void* (*allocate)(void* data),
                    void (*cleanup)(void* data, void* value));
            // Index 3
            void* (*stack)(
                    FutoInAsyncSteps* bsi,
                    size_t data_size,
                    void (*cleanup)(void* value));
            // Index 4
            void (*success)(FutoInAsyncSteps* bsi, FutoInArgs* args);
            // Index 5
            void (*handle_error)(
                    FutoInAsyncSteps* bsi, const char* code, const char* info);
            // Index 6
            void (*setTimeout)(FutoInAsyncSteps* bsi, uint32_t timeout_ms);
            // Index 7
            void (*setCancel)(
                    FutoInAsyncSteps* bsi,
                    void* data,
                    FutoInAsyncSteps_cancel_callback ch);
            // Index 8
            void (*waitExternal)(FutoInAsyncSteps* bsi);
            // Index 9
            void (*loop)(
                    FutoInAsyncSteps* bsi,
                    void* data,
                    void (*f)(FutoInAsyncSteps* bsi, void* data),
                    const char* label);
            // Index 10
            void (*repeat)(
                    FutoInAsyncSteps* bsi,
                    void* data,
                    size_t count,
                    void (*f)(FutoInAsyncSteps* bsi, void* data, size_t i),
                    const char* label);
            // Index 11
            void (*breakLoop)(FutoInAsyncSteps* bsi, const char* label);
            // Index 12
            void (*continueLoop)(FutoInAsyncSteps* bsi, const char* label);
            // Index 13
            void (*execute)(
                    FutoInAsyncSteps* bsi,
                    void* data,
                    FutoInAsyncSteps_error_callback unhandled_error);
            // Index 14
            void (*cancel)(FutoInAsyncSteps* bsi);
            // Index 15
            void (*addSync)(
                    FutoInAsyncSteps* bsi,
                    FutoInSync* sync,
                    void* data,
                    FutoInAsyncSteps_execute_callback f,
                    FutoInAsyncSteps_error_callback eh);
            // Index 16
            ptrdiff_t (*rootId)(FutoInAsyncSteps* bsi);
            // Index 17
            int (*isValid)(FutoInAsyncSteps* bsi);
            // Index 18
            FutoInAsyncSteps* (*newInstance)(FutoInAsyncSteps* bsi);
            // Index 19
            void (*free)(FutoInAsyncSteps* bsi);
            // Index 20
            FutoInHandle (*sched_immediate)(
                    FutoInAsyncSteps* bsi, void* data, void (*cb)(void* data));
            // Index 21
            FutoInHandle (*sched_deferred)(
                    FutoInAsyncSteps* bsi,
                    uint32_t delay_ms,
                    void* data,
                    void (*cb)(void* data));
            // Index 22
            void (*sched_cancel)(FutoInAsyncSteps* bsi, FutoInHandle* handle);
            // Index 23
            int (*sched_is_valid)(FutoInAsyncSteps* bsi, FutoInHandle* handle);
            // Index 24
            int (*is_same_thread)(FutoInAsyncSteps* bsi);
        };
        void* funcs[25];
    };
    // NOTE: extendable by implementation
};
struct FutoInAsyncSteps_
{
#ifdef __cplusplus
    FutoInAsyncSteps_(const FutoInAsyncStepsAPI* api) noexcept : api(api) {}
#endif
    const FutoInAsyncStepsAPI* const api;
    // NOTE: extendable by implementation
};

struct FutoInSyncAPI_
{
    union
    {
        struct
        {
            // Index 0
            void (*lock)(FutoInAsyncSteps* bsi, FutoInSync* sync);
            // Index 1
            void (*unlock)(FutoInAsyncSteps* bsi, FutoInSync* sync);
        };
        void* funcs[2];
    };
    // NOTE: extendable by implementation
};
struct FutoInSync_
{
#ifdef __cplusplus
    FutoInSync_() noexcept : api(nullptr) {}
#endif
    const FutoInSyncAPI* const api;
    // NOTE: extendable by implementation
};

#ifdef __cplusplus
}
#endif
//---
#endif // FUTOIN_BINARYSTEPS_H
