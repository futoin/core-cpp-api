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
//! @brief Universal native IAsyncSteps (FTN12) binary value move operations
//! @sa https://specs.futoin.org/final/preview/ftn12_async_api.html
//-----------------------------------------------------------------------------

#ifndef FUTOIN_DETAILS_BINARYMOVE_HPP
#define FUTOIN_DETAILS_BINARYMOVE_HPP

#include <vector>
// ---
#include "../binaryval.h"
#include "../fatalmsg.hpp"
#include "../string.hpp"
// ---

namespace futoin {
    namespace details {
        template<typename T, typename Any>
        struct moveHelper;

        // String helpers
        // ---
        template<typename T, FutoInTypeFlags FT>
        struct moveStringHelper
        {
            // constexpr lambdas are C++17 :(
            static void cleanup(FutoInBinaryValue* v)
            {
                delete reinterpret_cast<T*>(v->custom_data);
            }

            static constexpr FutoInType FTN_TYPE = {FT, &cleanup};

            template<typename Any>
            static void moveTo(FutoInBinaryValue& d, Any&& /*a*/, T&& v)
            {
                auto* s = new T(std::move(v));
                d.custom_data = s;
                d.p = s->data();
                d.length = s->length();
                d.type = &FTN_TYPE;
            }
            template<typename Any>
            static void moveFrom(FutoInBinaryValue& d, Any& a)
            {
                if (d.type == &FTN_TYPE) {
                    a = std::move(*reinterpret_cast<T*>(d.custom_data));
                } else {
                    a = T{reinterpret_cast<const typename T::value_type*>(d.p)};
                }
            }
        };
        template<typename T, FutoInTypeFlags FT>
        constexpr FutoInType moveStringHelper<T, FT>::FTN_TYPE;
        template<typename Any>
        struct moveHelper<futoin::string, Any>
            : moveStringHelper<futoin::string, FTN_TYPE_STRING>
        {};
        template<typename Any>
        struct moveHelper<futoin::u16string, Any>
            : moveStringHelper<futoin::u16string, FTN_TYPE_STRING16>
        {};
        template<typename Any>
        struct moveHelper<futoin::u32string, Any>
            : moveStringHelper<futoin::u32string, FTN_TYPE_STRING32>
        {};

        // Fundamental types
        // ---
        template<typename T, FutoInTypeFlags FT, T FutoInBinaryValue::*M>
        struct moveFundamentalHelper
        {
            static constexpr FutoInType FTN_TYPE = {FT, nullptr};

            template<typename A>
            static void moveTo(FutoInBinaryValue& d, A&& /*a*/, T v)
            {
                d.*M = v;
                d.type = &FTN_TYPE;
            }
            template<typename A>
            static void moveFrom(FutoInBinaryValue& d, A& v)
            {
                v = d.*M;
            }
        };
        template<typename T, FutoInTypeFlags FT, T FutoInBinaryValue::*M>
        constexpr FutoInType moveFundamentalHelper<T, FT, M>::FTN_TYPE;
        template<typename Any>
        struct moveHelper<bool, Any>
            : moveFundamentalHelper<bool, FTN_TYPE_BOOL, &FutoInBinaryValue::b>
        {};
        template<typename Any>
        struct moveHelper<int8_t, Any> : moveFundamentalHelper<
                                                 int8_t,
                                                 FTN_TYPE_INT8,
                                                 &FutoInBinaryValue::i8>
        {};
        template<typename Any>
        struct moveHelper<int16_t, Any> : moveFundamentalHelper<
                                                  int16_t,
                                                  FTN_TYPE_INT16,
                                                  &FutoInBinaryValue::i16>
        {};
        template<typename Any>
        struct moveHelper<int32_t, Any> : moveFundamentalHelper<
                                                  int32_t,
                                                  FTN_TYPE_INT32,
                                                  &FutoInBinaryValue::i32>
        {};
        template<typename Any>
        struct moveHelper<int64_t, Any> : moveFundamentalHelper<
                                                  int64_t,
                                                  FTN_TYPE_INT64,
                                                  &FutoInBinaryValue::i64>
        {};
        template<typename Any>
        struct moveHelper<uint8_t, Any> : moveFundamentalHelper<
                                                  uint8_t,
                                                  FTN_TYPE_UINT8,
                                                  &FutoInBinaryValue::u8>
        {};
        template<typename Any>
        struct moveHelper<uint16_t, Any> : moveFundamentalHelper<
                                                   uint16_t,
                                                   FTN_TYPE_UINT16,
                                                   &FutoInBinaryValue::u16>
        {};
        template<typename Any>
        struct moveHelper<uint32_t, Any> : moveFundamentalHelper<
                                                   uint32_t,
                                                   FTN_TYPE_UINT32,
                                                   &FutoInBinaryValue::u32>
        {};
        template<typename Any>
        struct moveHelper<uint64_t, Any> : moveFundamentalHelper<
                                                   uint64_t,
                                                   FTN_TYPE_UINT64,
                                                   &FutoInBinaryValue::u64>
        {};
        template<typename Any>
        struct moveHelper<float, Any> : moveFundamentalHelper<
                                                float,
                                                FTN_TYPE_FLOAT,
                                                &FutoInBinaryValue::f>
        {};
        template<typename Any>
        struct moveHelper<double, Any> : moveFundamentalHelper<
                                                 double,
                                                 FTN_TYPE_DOUBLE,
                                                 &FutoInBinaryValue::d>
        {};

        // Custom object type
        // ---
        template<typename T, typename Any>
        struct moveObjectHelper
        {
            static void cleanup(FutoInBinaryValue* v)
            {
                delete reinterpret_cast<Any*>(v->custom_data);
            }
            static constexpr FutoInType FTN_TYPE = {
                    FTN_TYPE_CUSTOM_OBJECT, &cleanup};

            static void moveFrom(FutoInBinaryValue& d, Any& a)
            {
                if (d.type == &FTN_TYPE) {
                    a = std::move(*reinterpret_cast<Any*>(d.custom_data));
                }
            }
        };
        template<typename T, typename Any>
        constexpr FutoInType moveObjectHelper<T, Any>::FTN_TYPE;

        template<typename T, typename Any>
        struct moveHelper : moveObjectHelper<const void*, Any>
        {
            static void moveTo(FutoInBinaryValue& d, Any&& /*a*/, T&& v)
            {
                d.type = &moveHelper<const void*, Any>::FTN_TYPE;
                d.custom_data = new Any{std::forward<T>(v)};
            }
        };

        // Array types
        // ---
        template<
                typename T,
                FutoInTypeFlags FT,
                typename Allocator,
                typename Any>
        struct moveArrayHelper
        {
            static void cleanup(FutoInBinaryValue* v)
            {
                delete reinterpret_cast<Any*>(v->custom_data);
            }
            static constexpr FutoInType FTN_TYPE = {
                    FTN_TYPE_ARRAY | FT, &cleanup};
            using Vector = std::vector<T, Allocator>;

            static void moveTo(FutoInBinaryValue& d, Any&& /*a*/, Vector&& v)
            {
                d.p = v.data();
                d.length = v.size();
                d.type = &moveHelper<const void*, Any>::FTN_TYPE;
                d.custom_data = new Any{std::forward<Vector>(v)};
            }

            static void moveFrom(FutoInBinaryValue& d, Any& a)
            {
                if (d.type == &FTN_TYPE) {
                    a = std::move(*reinterpret_cast<Any*>(d.custom_data));
                } else {
                    T* p = reinterpret_cast<T*>(const_cast<void*>(d.p));
                    a = Any{Vector{p, p + d.length}};
                }
            }
        };
        template<
                typename T,
                FutoInTypeFlags FT,
                typename Allocator,
                typename Any>
        constexpr FutoInType moveArrayHelper<T, FT, Allocator, Any>::FTN_TYPE;

        template<FutoInTypeFlags FT, typename Allocator, typename Any>
        struct moveArrayHelper<bool, FT, Allocator, Any>
        {
            static void cleanup(FutoInBinaryValue* v)
            {
                delete[] reinterpret_cast<bool*>(const_cast<void*>(v->p));
                delete reinterpret_cast<Any*>(v->custom_data);
            }
            static constexpr FutoInType FTN_TYPE = {
                    FTN_TYPE_ARRAY | FT,
                    &cleanup,
            };
            using Vector = std::vector<bool, Allocator>;

            static void moveTo(FutoInBinaryValue& d, Any&& /*a*/, Vector&& v)
            {
                auto size = v.size();
                auto* p = new bool[size];
                d.p = p;
                d.length = size;

                for (auto b : v) {
                    *(p++) = b;
                }

                d.type = &moveHelper<const void*, Any>::FTN_TYPE;
                d.custom_data = new Any{std::forward<Vector>(v)};
            }

            static void moveFrom(FutoInBinaryValue& d, Any& a)
            {
                if (d.type == &FTN_TYPE) {
                    a = std::move(*reinterpret_cast<Any*>(d.custom_data));
                } else {
                    bool* p = reinterpret_cast<bool*>(const_cast<void*>(d.p));
                    a = Any{Vector(p, (p + d.length))};
                }
            }
        };
        template<FutoInTypeFlags FT, typename Allocator, typename Any>
        constexpr FutoInType
                moveArrayHelper<bool, FT, Allocator, Any>::FTN_TYPE;

        template<typename Allocator, typename Any>
        struct moveHelper<std::vector<bool, Allocator>, Any>
            : moveArrayHelper<bool, FTN_TYPE_BOOL, Allocator, Any>
        {};
        template<typename Allocator, typename Any>
        struct moveHelper<std::vector<int8_t, Allocator>, Any>
            : moveArrayHelper<int8_t, FTN_TYPE_INT8, Allocator, Any>
        {};
        template<typename Allocator, typename Any>
        struct moveHelper<std::vector<int16_t, Allocator>, Any>
            : moveArrayHelper<int16_t, FTN_TYPE_INT16, Allocator, Any>
        {};
        template<typename Allocator, typename Any>
        struct moveHelper<std::vector<int32_t, Allocator>, Any>
            : moveArrayHelper<int32_t, FTN_TYPE_INT32, Allocator, Any>
        {};
        template<typename Allocator, typename Any>
        struct moveHelper<std::vector<int64_t, Allocator>, Any>
            : moveArrayHelper<int64_t, FTN_TYPE_INT64, Allocator, Any>
        {};
        template<typename Allocator, typename Any>
        struct moveHelper<std::vector<uint8_t, Allocator>, Any>
            : moveArrayHelper<uint8_t, FTN_TYPE_UINT8, Allocator, Any>
        {};
        template<typename Allocator, typename Any>
        struct moveHelper<std::vector<uint16_t, Allocator>, Any>
            : moveArrayHelper<uint16_t, FTN_TYPE_UINT16, Allocator, Any>
        {};
        template<typename Allocator, typename Any>
        struct moveHelper<std::vector<uint32_t, Allocator>, Any>
            : moveArrayHelper<uint32_t, FTN_TYPE_UINT32, Allocator, Any>
        {};
        template<typename Allocator, typename Any>
        struct moveHelper<std::vector<uint64_t, Allocator>, Any>
            : moveArrayHelper<uint64_t, FTN_TYPE_UINT64, Allocator, Any>
        {};
        template<typename Allocator, typename Any>
        struct moveHelper<std::vector<float, Allocator>, Any>
            : moveArrayHelper<float, FTN_TYPE_FLOAT, Allocator, Any>
        {};
        template<typename Allocator, typename Any>
        struct moveHelper<std::vector<double, Allocator>, Any>
            : moveArrayHelper<double, FTN_TYPE_DOUBLE, Allocator, Any>
        {};

        // Type erased API
        // ---
        template<typename T, typename Any>
        void moveTo(FutoInBinaryValue& d, Any&& a, T&& v)
        {
            moveHelper<T, Any>::moveTo(
                    d, std::forward<Any>(a), std::forward<T>(v));
        };
        template<typename Any>
        void moveFrom(FutoInBinaryValue& d, Any& a)
        {
            auto* t = d.type;

            if (!t) {
                return;
            }

            uint8_t f = t->flags;

            switch (f) {
            case FTN_TYPE_CUSTOM_OBJECT:
                moveHelper<const void*, Any>::moveFrom(d, a);
                break;
            case FTN_TYPE_STRING:
                moveHelper<futoin::string, Any>::moveFrom(d, a);
                break;
            case FTN_TYPE_STRING16:
                moveHelper<futoin::u16string, Any>::moveFrom(d, a);
                break;
            case FTN_TYPE_STRING32:
                moveHelper<futoin::u32string, Any>::moveFrom(d, a);
                break;
            case FTN_TYPE_BOOL: moveHelper<bool, Any>::moveFrom(d, a); break;
            case FTN_TYPE_INT8: moveHelper<int8_t, Any>::moveFrom(d, a); break;
            case FTN_TYPE_INT16:
                moveHelper<int16_t, Any>::moveFrom(d, a);
                break;
            case FTN_TYPE_INT32:
                moveHelper<int32_t, Any>::moveFrom(d, a);
                break;
            case FTN_TYPE_INT64:
                moveHelper<int64_t, Any>::moveFrom(d, a);
                break;
            case FTN_TYPE_UINT8:
                moveHelper<uint8_t, Any>::moveFrom(d, a);
                break;
            case FTN_TYPE_UINT16:
                moveHelper<uint16_t, Any>::moveFrom(d, a);
                break;
            case FTN_TYPE_UINT32:
                moveHelper<uint32_t, Any>::moveFrom(d, a);
                break;
            case FTN_TYPE_UINT64:
                moveHelper<uint64_t, Any>::moveFrom(d, a);
                break;
            case FTN_TYPE_FLOAT: moveHelper<float, Any>::moveFrom(d, a); break;
            case FTN_TYPE_DOUBLE:
                moveHelper<double, Any>::moveFrom(d, a);
                break;
            default:
                switch (f & FTN_COMPLEX_TYPE_MASK) {
                case FTN_TYPE_ARRAY:
                    switch (f & FTN_BASE_TYPE_MASK) {
                    case FTN_TYPE_BOOL:
                        moveHelper<std::vector<bool>, Any>::moveFrom(d, a);
                        break;
                    case FTN_TYPE_INT8:
                        moveHelper<std::vector<int8_t>, Any>::moveFrom(d, a);
                        break;
                    case FTN_TYPE_INT16:
                        moveHelper<std::vector<int16_t>, Any>::moveFrom(d, a);
                        break;
                    case FTN_TYPE_INT32:
                        moveHelper<std::vector<int32_t>, Any>::moveFrom(d, a);
                        break;
                    case FTN_TYPE_INT64:
                        moveHelper<std::vector<int64_t>, Any>::moveFrom(d, a);
                        break;
                    case FTN_TYPE_UINT8:
                        moveHelper<std::vector<uint8_t>, Any>::moveFrom(d, a);
                        break;
                    case FTN_TYPE_UINT16:
                        moveHelper<std::vector<uint16_t>, Any>::moveFrom(d, a);
                        break;
                    case FTN_TYPE_UINT32:
                        moveHelper<std::vector<uint32_t>, Any>::moveFrom(d, a);
                        break;
                    case FTN_TYPE_UINT64:
                        moveHelper<std::vector<uint64_t>, Any>::moveFrom(d, a);
                        break;
                    case FTN_TYPE_FLOAT:
                        moveHelper<std::vector<float>, Any>::moveFrom(d, a);
                        break;
                    case FTN_TYPE_DOUBLE:
                        moveHelper<std::vector<double>, Any>::moveFrom(d, a);
                        break;
                        FatalMsg() << "Unsupported binary array type: " << f;
                    }
                    break;
                default: FatalMsg() << "Unsupported binary type: " << f;
                }
            }

            ::futoin_reset_binval(&d);
        };
    } // namespace details
} // namespace futoin

//---
#endif // FUTOIN_DETAILS_BINARYMOVE_HPP
