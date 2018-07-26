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

#ifndef FTN_ERRORS_HPP
#define FTN_ERRORS_HPP
//---

#include <stdexcept>
#include <string>

//---

namespace futoin {
    /**
     * @brief FutoIn error code type
     *
     * Unlike traditional integral error codes, FutoIn errors codes are designed
     * to be self-descriptive to be easily transfered over network.
     */
    using ErrorCode = const char *;

    /**
     * @brief FutoIn error message in UTF-8
     */
    using ErrorMessage = std::string;

    /**
     * @brief Canonical FutoIn Error
     */
    class Error : public std::runtime_error {
      public:
        Error(ErrorCode code, ErrorMessage message) noexcept
            : runtime_error(code), message(message) {}

        const ErrorMessage message;
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
        constexpr ErrorCode ConnectError = "ConnectError";

        /**
         * @brief Communication error at any stage after request is sent
         * and before response is received.
         *
         * @note Must be generated on Invoker side
         */
        constexpr ErrorCode CommError = "CommError";

        /**
         * @brief Unknown interface requested.
         *
         * @note Must be generated only on Executor side.
         */
        constexpr ErrorCode UnknownInterface = "UnknownInterface";

        /**
         * @brief Not supported interface version.
         * @note Must be generated only on Executor side
         */
        constexpr ErrorCode NotSupportedVersion = "NotSupportedVersion";

        /**
         * @brief In case interface function is not implemented on Executor side
         * @note Must be generated on Executor side
         */
        constexpr ErrorCode NotImplemented = "NotImplemented";

        /**
         * @brief Security policy on Executor side does not allow to
         * access interface or specific function.
         * @note Must be generated only on Executor side
         */
        constexpr ErrorCode Unauthorized = "Unauthorized";

        /**
         * @brief Unexpected internal error on Executor side, including internal
         * CommError.
         * @note Must be generated only on Executor side
         */
        constexpr ErrorCode InternalError = "InternalError";

        /**
         * @brief Unexpected internal error on Invoker side, not related to
         * CommError.
         * @note Must be generated only on Invoker side
         */
        constexpr ErrorCode InvokerError = "InvokerError";

        /**
         * @brief Invalid data is passed as FutoIn request.
         * @note Must be generated only on Executor side
         */
        constexpr ErrorCode InvalidRequest = "InvalidRequest";

        /**
         * @brief Defense system has triggered rejection
         * @note Must be generated on Executor side, but also possible to be
         * triggered on Invoker
         */
        constexpr ErrorCode DefenseRejected = "DefenseRejected";

        /**
         * @brief Executor requests re-authorization
         * @note Must be generated only on Executor side
         */
        constexpr ErrorCode PleaseReauth = "PleaseReauth";

        /**
         * @brief 'sec' request section has invalid data or not SecureChannel
         * @note Must be generated only on Executor side
         */
        constexpr ErrorCode SecurityError = "SecurityError";

        /**
         * @brief Timeout occurred in any stage
         * @note Must be used only internally and should never travel in request
         * message
         */
        constexpr ErrorCode Timeout = "Timeout";

        /**
         * @brief Loop Break called
         * @note Must not be used directly.
         */
        constexpr ErrorCode LoopBreak = "LoopBreak";

        /**
         * @brief Loop Continue called
         * @note Must not be used directly.
         */
        constexpr ErrorCode LoopCont = "LoopCont";
    }; // namespace errors
} // namespace futoin

//---
#endif // FTN_ERRORS_HPP
