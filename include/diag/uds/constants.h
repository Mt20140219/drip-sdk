#pragma once

#include <cstdint>

namespace diag::uds {

constexpr uint8_t kResponseSidOffset = 0x40;

namespace sid {
constexpr uint8_t kDiagnosticSessionControl = 0x10;
constexpr uint8_t kEcuReset = 0x11;
constexpr uint8_t kClearDiagnosticInformation = 0x14;
constexpr uint8_t kReadDtcInformation = 0x19;
constexpr uint8_t kReadDataByIdentifier = 0x22;
constexpr uint8_t kReadMemoryByAddress = 0x23;
constexpr uint8_t kReadScalingDataByIdentifier = 0x24;
constexpr uint8_t kSecurityAccess = 0x27;
constexpr uint8_t kCommunicationControl = 0x28;
constexpr uint8_t kAuthentication = 0x29;
constexpr uint8_t kReadDataByPeriodicIdentifier = 0x2A;
constexpr uint8_t kDynamicallyDefineDataIdentifier = 0x2C;
constexpr uint8_t kWriteDataByIdentifier = 0x2E;
constexpr uint8_t kInputOutputControlByIdentifier = 0x2F;
constexpr uint8_t kRoutineControl = 0x31;
constexpr uint8_t kRequestDownload = 0x34;
constexpr uint8_t kRequestUpload = 0x35;
constexpr uint8_t kTransferData = 0x36;
constexpr uint8_t kRequestTransferExit = 0x37;
constexpr uint8_t kTesterPresent = 0x3E;
constexpr uint8_t kWriteMemoryByAddress = 0x3D;
constexpr uint8_t kControlDtcSetting = 0x85;
constexpr uint8_t kResponseOnEvent = 0x86;
constexpr uint8_t kLinkControl = 0x87;
}  // namespace sid

namespace nrc {
constexpr uint8_t kServiceNotSupported = 0x11;
constexpr uint8_t kSubFunctionNotSupported = 0x12;
constexpr uint8_t kIncorrectMessageLengthOrInvalidFormat = 0x13;
constexpr uint8_t kConditionsNotCorrect = 0x22;
constexpr uint8_t kRequestSequenceError = 0x24;
constexpr uint8_t kRequestOutOfRange = 0x31;
constexpr uint8_t kSecurityAccessDenied = 0x33;
constexpr uint8_t kInvalidKey = 0x35;
constexpr uint8_t kExceedNumberOfAttempts = 0x36;
constexpr uint8_t kRequiredTimeDelayNotExpired = 0x37;
constexpr uint8_t kServiceNotSupportedInActiveSession = 0x7F;
constexpr uint8_t kRequestCorrectlyReceivedResponsePending = 0x78;
}  // namespace nrc

}  // namespace diag::uds
