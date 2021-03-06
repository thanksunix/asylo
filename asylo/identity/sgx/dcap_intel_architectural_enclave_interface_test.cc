/*
 *
 * Copyright 2019 Asylo authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "asylo/identity/sgx/dcap_intel_architectural_enclave_interface.h"

#include <cstdint>
#include <cstring>
#include <numeric>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/memory/memory.h"
#include "asylo/crypto/algorithms.pb.h"
#include "asylo/crypto/util/byte_container_util.h"
#include "asylo/crypto/util/byte_container_view.h"
#include "asylo/crypto/util/bytes.h"
#include "asylo/crypto/util/trivial_object_util.h"
#include "asylo/identity/sgx/identity_key_management_structs.h"
#include "asylo/identity/sgx/pce_util.h"
#include "asylo/test/util/memory_matchers.h"
#include "asylo/test/util/status_matchers.h"
#include "asylo/util/status.h"
#include "QuoteGeneration/pce_wrapper/inc/sgx_pce.h"
#include "QuoteGeneration/pce_wrapper/inc/sgx_pce_constants.h"
#include "QuoteGeneration/quote_wrapper/common/inc/sgx_ql_lib_common.h"

namespace asylo {
namespace sgx {

// Overload resolution requires the overload be outside the anonymous namespace.
bool operator==(const Targetinfo &lhs, const Targetinfo &rhs) {
  return memcmp(&lhs, &rhs, sizeof(rhs)) == 0;
}

bool operator==(const Report &lhs, const Report &rhs) {
  return memcmp(&lhs, &rhs, sizeof(rhs)) == 0;
}

namespace {

using ::testing::_;
using ::testing::DoAll;
using ::testing::Eq;
using ::testing::Not;
using ::testing::NotNull;
using ::testing::Pointee;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::Test;

class MockDcapLibraryInterface : public DcapLibraryInterface {
 public:
  MOCK_METHOD(quote3_error_t, QeSetEnclaveDirpath, (const char *),
              (const, override));
  MOCK_METHOD(sgx_pce_error_t, PceGetTarget,
              (sgx_target_info_t * p_pce_target, sgx_isv_svn_t *p_pce_isv_svn),
              (const, override));
  MOCK_METHOD(sgx_pce_error_t, GetPceInfo,
              (const sgx_report_t *p_report, const uint8_t *p_pek,
               uint32_t pek_size, uint8_t crypto_suite,
               uint8_t *p_encrypted_ppid, uint32_t encrypted_ppid_size,
               uint32_t *p_encrypted_ppid_out_size, sgx_isv_svn_t *p_pce_isvsvn,
               uint16_t *p_pce_id, uint8_t *p_signature_scheme),
              (const, override));
  MOCK_METHOD(sgx_pce_error_t, PceSignReport,
              (const sgx_isv_svn_t *isv_svn, const sgx_cpu_svn_t *cpu_svn,
               const sgx_report_t *p_report, uint8_t *p_signature,
               uint32_t signature_buf_size, uint32_t *p_signature_out_size),
              (const, override));
  MOCK_METHOD(quote3_error_t, QeGetTargetInfo,
              (sgx_target_info_t * p_qe_target_info), (const, override));
  MOCK_METHOD(quote3_error_t, QeGetQuoteSize, (uint32_t * p_quote_size),
              (const, override));
  MOCK_METHOD(quote3_error_t, QeGetQuote,
              (const sgx_report_t *p_app_report, uint32_t quote_size,
               uint8_t *p_quote),
              (const, override));
};

// Copies |buffer| into buffer pointed to by arg number |k|.
ACTION_TEMPLATE(SetArgBuffer, HAS_1_TEMPLATE_PARAMS(int, k),
                AND_2_VALUE_PARAMS(buffer, size)) {
  memcpy(std::get<k>(args), buffer, size);
}

// Copies |container| into buffer pointed to by arg number |k|.
ACTION_TEMPLATE(SetArgContainer, HAS_1_TEMPLATE_PARAMS(int, k),
                AND_1_VALUE_PARAMS(container)) {
  static_assert(sizeof(typename container_type::value_type) == 1,
                "The container must contain bytes");
  memcpy(std::get<k>(args), container.data(), container.size());
}

class DcapIntelArchitecturalEnclaveInterfaceTests : public Test {
 protected:
  // |dcap_library_| is not owned by this test fixture - it's owned by |dcap_|.
  MockDcapLibraryInterface *dcap_library_ = new MockDcapLibraryInterface;
  DcapIntelArchitecturalEnclaveInterface dcap_{absl::WrapUnique(dcap_library_)};
};

TEST_F(DcapIntelArchitecturalEnclaveInterfaceTests, SetEnclaveDirSuccess) {
  const std::string kDir = "some directory";
  EXPECT_CALL(*dcap_library_, QeSetEnclaveDirpath(Eq(kDir)))
      .WillOnce(Return(SGX_QL_SUCCESS));
  EXPECT_THAT(dcap_.SetEnclaveDir(kDir), IsOk());
}

TEST_F(DcapIntelArchitecturalEnclaveInterfaceTests, SetEnclaveDirFailure) {
  EXPECT_CALL(*dcap_library_, QeSetEnclaveDirpath(_))
      .WillOnce(Return(SGX_QL_ERROR_INVALID_PRIVILEGE));
  EXPECT_THAT(dcap_.SetEnclaveDir("something"), Not(IsOk()));
}

TEST_F(DcapIntelArchitecturalEnclaveInterfaceTests, GetPceTargetInfoSuccess) {
  sgx_target_info_t expected_target = TrivialRandomObject<sgx_target_info_t>();
  sgx_isv_svn_t expected_svn = TrivialRandomObject<sgx_isv_svn_t>();

  EXPECT_CALL(*dcap_library_, PceGetTarget(_, _))
      .WillOnce(DoAll(SetArgPointee<0>(expected_target),
                      SetArgPointee<1>(expected_svn), Return(SGX_PCE_SUCCESS)));

  Targetinfo actual_target;
  uint16_t actual_svn;
  EXPECT_THAT(dcap_.GetPceTargetinfo(&actual_target, &actual_svn), IsOk());
  EXPECT_THAT(actual_target, TrivialObjectEq(expected_target));
  EXPECT_THAT(actual_svn, Eq(expected_svn));
}

TEST_F(DcapIntelArchitecturalEnclaveInterfaceTests, GetPceTargetInfoFailure) {
  EXPECT_CALL(*dcap_library_, PceGetTarget(_, _))
      .WillOnce(Return(SGX_PCE_INVALID_PARAMETER));

  Targetinfo target;
  uint16_t svn;
  EXPECT_THAT(dcap_.GetPceTargetinfo(&target, &svn), Not(IsOk()));
}

TEST_F(DcapIntelArchitecturalEnclaveInterfaceTests, GetPceInfoSuccess) {
  const Report kInputReport = TrivialRandomObject<Report>();
  constexpr uint8_t kInputPpidEk[] = "fake encryption key";

  const std::string kOutputPpidEncrypted = "super secret";
  constexpr uint16_t kOutputPceSvn = 42;
  constexpr uint16_t kOutputPceId = 11235;

  EXPECT_CALL(*dcap_library_,
              GetPceInfo(Pointee(TrivialObjectEq(kInputReport)),
                         MemEq(kInputPpidEk, sizeof(kInputPpidEk)),
                         sizeof(kInputPpidEk), PCE_ALG_RSA_OAEP_3072,
                         /*p_encrypted_ppid=*/_, kRsa3072ModulusSize,
                         /*p_encrypted_ppid_out_size=*/_, /*p_pce_isvsvn=*/_,
                         /*p_pce_id=*/_, /*p_signature_scheme=*/_))
      .WillOnce(DoAll(SetArgBuffer<4>(kOutputPpidEncrypted.data(),
                                      kOutputPpidEncrypted.size()),
                      SetArgPointee<6>(kOutputPpidEncrypted.size()),
                      SetArgPointee<7>(kOutputPceSvn),
                      SetArgPointee<8>(kOutputPceId),
                      SetArgPointee<9>(PCE_NIST_P256_ECDSA_SHA256),
                      Return(SGX_PCE_SUCCESS)));

  std::string ppid_encrypted;
  uint16_t pce_svn;
  uint16_t pce_id;
  SignatureScheme signature_scheme;
  ASYLO_ASSERT_OK(dcap_.GetPceInfo(
      kInputReport, kInputPpidEk, AsymmetricEncryptionScheme::RSA3072_OAEP,
      &ppid_encrypted, &pce_svn, &pce_id, &signature_scheme));

  EXPECT_THAT(ppid_encrypted, Eq(kOutputPpidEncrypted));
  EXPECT_THAT(pce_svn, Eq(kOutputPceSvn));
  EXPECT_THAT(pce_id, Eq(kOutputPceId));
  EXPECT_THAT(signature_scheme, Eq(SignatureScheme::ECDSA_P256_SHA256));
}

TEST_F(DcapIntelArchitecturalEnclaveInterfaceTests,
       GetPceInfoInvalidPpidEncryptionScheme) {
  std::string ppid_encrypted;
  uint16_t pce_svn;
  uint16_t pce_id;
  SignatureScheme signature_scheme;

  // Only RSA3072-OAEP keys are allowed.
  EXPECT_THAT(
      dcap_.GetPceInfo(Report{}, /*ppid_encryption_key=*/{1},
                       AsymmetricEncryptionScheme::RSA2048_OAEP,
                       &ppid_encrypted, &pce_svn, &pce_id, &signature_scheme),
      StatusIs(error::GoogleError::INVALID_ARGUMENT));
  EXPECT_TRUE(ppid_encrypted.empty());
}

TEST_F(DcapIntelArchitecturalEnclaveInterfaceTests, GetPceInfoFailure) {
  EXPECT_CALL(*dcap_library_, GetPceInfo(_, _, _, _, _, _, _, _, _, _))
      .WillOnce(Return(SGX_PCE_INVALID_PRIVILEGE));

  std::string ppid_encrypted;
  uint16_t pce_svn;
  uint16_t pce_id;
  SignatureScheme signature_scheme;
  EXPECT_THAT(
      dcap_.GetPceInfo(Report{}, /*ppid_encryption_key=*/{42},
                       AsymmetricEncryptionScheme::RSA3072_OAEP,
                       &ppid_encrypted, &pce_svn, &pce_id, &signature_scheme),
      Not(IsOk()));
  EXPECT_TRUE(ppid_encrypted.empty());
}

TEST_F(DcapIntelArchitecturalEnclaveInterfaceTests, SignReportSuccess) {
  constexpr uint16_t kPceSvn = 206;
  const auto kCpuSvn = TrivialRandomObject<UnsafeBytes<kCpusvnSize>>();
  const Report kReport = TrivialRandomObject<Report>();
  const std::string kOutputSignature = "cheese";

  EXPECT_CALL(
      *dcap_library_,
      PceSignReport(Pointee(kPceSvn), MemEq(kCpuSvn.data(), kCpusvnSize),
                    Pointee(TrivialObjectEq(kReport)),
                    /*p_signature=*/_, kEcdsaP256SignatureSize,
                    /*p_signature_out_size=*/_))
      .WillOnce(DoAll(
          SetArgBuffer<3>(kOutputSignature.data(), kOutputSignature.size()),
          SetArgPointee<5>(kOutputSignature.size()), Return(SGX_PCE_SUCCESS)));

  std::string signature;
  ASYLO_ASSERT_OK(dcap_.PceSignReport(kReport, kPceSvn, kCpuSvn, &signature));
  EXPECT_THAT(signature, Eq(kOutputSignature));
}

TEST_F(DcapIntelArchitecturalEnclaveInterfaceTests, SignReportFailure) {
  EXPECT_CALL(*dcap_library_, PceSignReport(_, _, _, _, _, _))
      .WillOnce(Return(SGX_PCE_INVALID_PARAMETER));

  const auto kCpuSvn = TrivialRandomObject<UnsafeBytes<kCpusvnSize>>();
  std::string signature;
  EXPECT_THAT(
      dcap_.PceSignReport(Report{}, /*target_pce_svn=*/0, kCpuSvn, &signature),
      Not(IsOk()));
}

TEST_F(DcapIntelArchitecturalEnclaveInterfaceTests, QeGetTargetinfoSuccess) {
  const Targetinfo kExpectedTargetinfo = TrivialRandomObject<Targetinfo>();

  EXPECT_CALL(*dcap_library_, QeGetTargetInfo(NotNull()))
      .WillOnce(DoAll(
          SetArgBuffer<0>(&kExpectedTargetinfo, sizeof(kExpectedTargetinfo)),
          Return(SGX_QL_SUCCESS)));

  EXPECT_THAT(dcap_.GetQeTargetinfo(), IsOkAndHolds(kExpectedTargetinfo));
}

TEST_F(DcapIntelArchitecturalEnclaveInterfaceTests, QeGetTargetinfoFailure) {
  EXPECT_CALL(*dcap_library_, QeGetTargetInfo(NotNull()))
      .WillOnce(Return(SGX_QL_ERROR_UNEXPECTED));
  EXPECT_THAT(dcap_.GetQeTargetinfo(), StatusIs(error::GoogleError::INTERNAL));
}

TEST_F(DcapIntelArchitecturalEnclaveInterfaceTests,
       GetQeQuoteSucceedsWithCompleteQuoteData) {
  const Report kReport = TrivialRandomObject<Report>();
  std::vector<uint8_t> quote(4321);  // arbitrary quote size
  std::iota(quote.begin(), quote.end(), 0);
  EXPECT_CALL(*dcap_library_, QeGetQuoteSize(NotNull()))
      .WillOnce(DoAll(SetArgPointee<0>(quote.size()), Return(SGX_QL_SUCCESS)));
  EXPECT_CALL(*dcap_library_, QeGetQuote(Pointee(TrivialObjectEq(kReport)),
                                         quote.size(), NotNull()))
      .WillOnce(DoAll(SetArgContainer<2>(quote), Return(SGX_QL_SUCCESS)));

  EXPECT_THAT(dcap_.GetQeQuote(kReport), IsOkAndHolds(quote));
}

TEST_F(DcapIntelArchitecturalEnclaveInterfaceTests, GetQeQuoteSizeFailure) {
  EXPECT_CALL(*dcap_library_, QeGetQuoteSize(NotNull()))
      .WillOnce(Return(SGX_QL_ERROR_INVALID_PRIVILEGE));
  EXPECT_THAT(dcap_.GetQeQuote(Report{}),
              StatusIs(error::GoogleError::PERMISSION_DENIED));
}

TEST_F(DcapIntelArchitecturalEnclaveInterfaceTests, GetQeQuoteFailure) {
  constexpr uint32_t kFakeQuoteSize = 32;  // size is arbitrary
  EXPECT_CALL(*dcap_library_, QeGetQuoteSize(NotNull()))
      .WillOnce(
          DoAll(SetArgPointee<0>(kFakeQuoteSize), Return(SGX_QL_SUCCESS)));
  EXPECT_CALL(*dcap_library_, QeGetQuote(NotNull(), kFakeQuoteSize, NotNull()))
      .WillOnce(Return(SGX_QL_ERROR_INVALID_PRIVILEGE));
  EXPECT_THAT(dcap_.GetQeQuote(Report{}),
              StatusIs(error::GoogleError::PERMISSION_DENIED));
}

}  // namespace
}  // namespace sgx
}  // namespace asylo
