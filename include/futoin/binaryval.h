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
//! @brief Universal native IAsyncSteps (FTN12) binary value
//! @sa https://specs.futoin.org/final/preview/ftn12_async_api.html
//-----------------------------------------------------------------------------

#ifndef FUTOIN_BINARYVAL_H
#define FUTOIN_BINARYVAL_H

#include <stddef.h>
#include <stdint.h>

//---
#ifdef __cplusplus
extern "C" {
#endif

typedef struct FutoInBinaryValue_ FutoInBinaryValue;
typedef struct FutoInType_ FutoInType;
typedef uint8_t FutoInTypeFlags;

enum
{
    FTN_TYPE_CUSTOM_OBJECT = 0x01,
    FTN_TYPE_STRING = 0x02,
    FTN_TYPE_STRING16 = 0x03,
    FTN_TYPE_STRING32 = 0x04,
    FTN_TYPE_BOOL = 0x05,
    FTN_TYPE_INT8 = 0x06,
    FTN_TYPE_INT16 = 0x07,
    FTN_TYPE_INT32 = 0x08,
    FTN_TYPE_INT64 = 0x09,
    FTN_TYPE_UINT8 = 0x0A,
    FTN_TYPE_UINT16 = 0x0B,
    FTN_TYPE_UINT32 = 0x0C,
    FTN_TYPE_UINT64 = 0x0D,
    FTN_TYPE_FLOAT = 0x0E,
    FTN_TYPE_DOUBLE = 0x0F,
    FTN_BASE_TYPE_MASK = 0x0F,
    // --
    FTN_TYPE_ARRAY = 0x10,
    FTN_COMPLEX_TYPE_MASK = 0xF0,
};

struct FutoInType_
{
    const FutoInTypeFlags flags;
    void (*const cleanup)(FutoInBinaryValue* v);
    // NOTE: extendable by implementation
};

struct FutoInBinaryValue_
{
    const FutoInType* type;
    union
    {
        const void* p;
        const char* cstr;
        const char16_t* cstr16;
        const char32_t* cstr32;
        bool b;
        int8_t i8;
        int16_t i16;
        int32_t i32;
        int64_t i64;
        uint8_t u8;
        uint16_t u16;
        uint32_t u32;
        uint64_t u64;
        float f;
        double d;
    };
    void* custom_data;
    uint32_t length;
};

static inline void futoin_reset_binval(FutoInBinaryValue* v)
{
    auto tp = v->type;
    if (tp) {
        auto* f = tp->cleanup;
        if (f) {
            f(v);
        }
    }

    v->type = 0;
    v->u64 = 0;
    v->custom_data = 0;
    v->length = 0;
}

#ifdef __cplusplus
}
#endif
//---
#endif // FUTOIN_BINARYVAL_H
