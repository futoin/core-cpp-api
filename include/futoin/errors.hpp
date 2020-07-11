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
//! @brief List of standard FutoIn error codes
//-----------------------------------------------------------------------------

#ifndef FUTOIN_ERRORS_HPP
#define FUTOIN_ERRORS_HPP
//---

#include "./string.hpp"
#include <cstring>
#include <stdexcept>

//---

namespace futoin {
    /**
     * @brief FutoIn error code type
     *
     * Unlike traditional integral error codes, FutoIn errors codes are designed
     * to be self-descriptive to be easily transfered over network.
     */
    using RawErrorCode = const char*;

    /**
     * @brief RawErrorCode wrapper
     */
    struct ErrorCode
    {
        ErrorCode(RawErrorCode raw_code) noexcept : raw_code(raw_code) {}

        inline operator RawErrorCode() const noexcept
        {
            return raw_code;
        }

        inline bool operator==(RawErrorCode other_code) const noexcept
        {
            return strcmp(raw_code, other_code) == 0;
        }

        inline bool operator!=(RawErrorCode other_code) const noexcept
        {
            return strcmp(raw_code, other_code) != 0;
        }

    private:
        RawErrorCode raw_code;
    };

    /**
     * @brief FutoIn error message in UTF-8
     */
    using ErrorMessage = futoin::string;

    /**
     * @brief Canonical FutoIn Error
     */
    class Error : public std::runtime_error
    {
    public:
        Error(RawErrorCode code) noexcept : runtime_error(code) {}
    };

    /**
     * @brief Externded error to throw without AsyncSteps instance
     */
    class ExtError : public std::runtime_error
    {
    public:
        ExtError(RawErrorCode code, ErrorMessage&& error_info) noexcept :
            runtime_error(code),
            error_info_(error_info)
        {}

        const ErrorMessage& error_info() const
        {
            return error_info_;
        }

    protected:
        ErrorMessage error_info_;
    };

    /**
     * @brief Extendable namespace to hold error code definitions.
     */
    namespace errors {
        /**
         * @brief Connection error before request is sent.
         *
         * @note Must be generated on Invoker side
         */
        constexpr RawErrorCode ConnectError = "ConnectError";

        /**
         * @brief Communication error at any stage after request is sent
         * and before response is received.
         *
         * @note Must be generated on Invoker side
         */
        constexpr RawErrorCode CommError = "CommError";

        /**
         * @brief Unknown interface requested.
         *
         * @note Must be generated only on Executor side.
         */
        constexpr RawErrorCode UnknownInterface = "UnknownInterface";

        /**
         * @brief Not supported interface version.
         * @note Must be generated only on Executor side
         */
        constexpr RawErrorCode NotSupportedVersion = "NotSupportedVersion";

        /**
         * @brief In case interface function is not implemented on Executor side
         * @note Must be generated on Executor side
         */
        constexpr RawErrorCode NotImplemented = "NotImplemented";

        /**
         * @brief Security policy on Executor side does not allow to
         * access interface or specific function.
         * @note Must be generated only on Executor side
         */
        constexpr RawErrorCode Unauthorized = "Unauthorized";

        /**
         * @brief Unexpected internal error on Executor side, including internal
         * CommError.
         * @note Must be generated only on Executor side
         */
        constexpr RawErrorCode InternalError = "InternalError";

        /**
         * @brief Unexpected internal error on Invoker side, not related to
         * CommError.
         * @note Must be generated only on Invoker side
         */
        constexpr RawErrorCode InvokerError = "InvokerError";

        /**
         * @brief Invalid data is passed as FutoIn request.
         * @note Must be generated only on Executor side
         */
        constexpr RawErrorCode InvalidRequest = "InvalidRequest";

        /**
         * @brief Defense system has triggered rejection
         * @note Must be generated on Executor side, but also possible to be
         * triggered on Invoker
         */
        constexpr RawErrorCode DefenseRejected = "DefenseRejected";

        /**
         * @brief Executor requests re-authorization
         * @note Must be generated only on Executor side
         */
        constexpr RawErrorCode PleaseReauth = "PleaseReauth";

        /**
         * @brief 'sec' request section has invalid data or not SecureChannel
         * @note Must be generated only on Executor side
         */
        constexpr RawErrorCode SecurityError = "SecurityError";

        /**
         * @brief Timeout occurred in any stage
         * @note Must be used only internally and should never travel in request
         * message
         */
        constexpr RawErrorCode Timeout = "Timeout";

        /**
         * @brief Loop Break called
         * @note Must not be used directly.
         */
        constexpr RawErrorCode LoopBreak = "LoopBreak";

        /**
         * @brief Loop Continue called
         * @note Must not be used directly.
         */
        constexpr RawErrorCode LoopCont = "LoopCont";
    }; // namespace errors
} // namespace futoin

//---
#endif // FUTOIN_ERRORS_HPP
