#pragma once

#include <cstdint>

namespace diag::uds {

constexpr uint8_t kResponseSidOffset = 0x40;

namespace sid {
constexpr uint8_t kDiagnosticSessionControl = 0x10;
constexpr uint8_t kEcuReset = 0x11;
constexpr uint8_t kReadDtcInformation = 0x19;
constexpr uint8_t kReadDataByIdentifier = 0x22;
constexpr uint8_t kSecurityAccess = 0x27;
constexpr uint8_t kTesterPresent = 0x3E;
}  // namespace sid

namespace nrc {
constexpr uint8_t kServiceNotSupported = 0x11;
constexpr uint8_t kSubFunctionNotSupported = 0x12;
constexpr uint8_t kIncorrectMessageLengthOrInvalidFormat = 0x13;
constexpr uint8_t kConditionsNotCorrect = 0x22;
constexpr uint8_t kRequestOutOfRange = 0x31;
constexpr uint8_t kSecurityAccessDenied = 0x33;
constexpr uint8_t kInvalidKey = 0x35;
constexpr uint8_t kExceedNumberOfAttempts = 0x36;
constexpr uint8_t kRequiredTimeDelayNotExpired = 0x37;
constexpr uint8_t kRequestCorrectlyReceivedResponsePending = 0x78;
}  // namespace nrc

}  // namespace diag::uds
