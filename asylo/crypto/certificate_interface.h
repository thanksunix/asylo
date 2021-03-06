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

#ifndef ASYLO_CRYPTO_CERTIFICATE_INTERFACE_H_
#define ASYLO_CRYPTO_CERTIFICATE_INTERFACE_H_

#include <cstdint>
#include <string>

#include "absl/types/optional.h"
#include "asylo/util/status.h"
#include "asylo/util/statusor.h"

namespace asylo {

// Options for additional verification logic. Even if a field is set, if the
// concept does not apply for a CertificateInterface implementation, the
// implementation may ignore that check.
struct VerificationConfig {
  // If the issuer states information about whether it is a CA, checks that the
  // issuer is a CA certificate.
  bool issuer_ca;
  // Checks that the distance in a chain between a certificate and the user
  // certificate is at most the pathlength allowed by the certificate, if a
  // limit is given. When checking distance, certficates with an IsCa() value of
  // absl::nullopt are treated as CAs.
  bool max_pathlen;
  // Checks the key usage of the issuer certificate is the type expected for the
  // subject certificate's format, if the key usage is given.
  bool issuer_key_usage;

  VerificationConfig() = default;

  // Initializes the fields with the value of |all_fields|.
  explicit VerificationConfig(bool all_fields)
      : issuer_ca(all_fields),
        max_pathlen(all_fields),
        issuer_key_usage(all_fields) {}
};

// The certificate-defined allowed uses of the certificate's subject key.
struct KeyUsageInformation {
  // The certificate's subject key may be used to verify certificates.
  bool certificate_signing;
  // The certificate's subject key may be used to verify CRLs.
  bool crl_signing;
  // The certificate's subject key is used to verify digital signatures other
  // than those of certificates or CRLs.
  bool digital_signature;
};

// CertificateInterface defines an interface for operations on certificates.
class CertificateInterface {
 public:
  virtual ~CertificateInterface() = default;

  // Checks if this object can be verified by |issuer_certificate|, with the
  // additional requirements set by |config| and used as relevant by the
  // different certificate interface implementations. Returns an error if a
  // required check failed.
  virtual Status Verify(const CertificateInterface &issuer_certificate,
                        const VerificationConfig &config) const = 0;

  // Returns the DER-encoded public key certified by this object. Returns a
  // non-OK Status if there was an error.
  virtual StatusOr<std::string> SubjectKeyDer() const = 0;

  // Returns whether this object is a CA certificate. Returns absl::nullopt if
  // the question is not relevant for this object or is unknown.
  virtual absl::optional<bool> IsCa() const = 0;

  // Returns the maximum number of CA certificates allowed in a path starting
  // with this object. Returns absl::nullopt if the path length is not set.
  virtual absl::optional<int64_t> CertPathLength() const = 0;

  // Returns the allowed uses of a key certified by this object.
  virtual absl::optional<KeyUsageInformation> KeyUsage() const = 0;
};

}  // namespace asylo

#endif  // ASYLO_CRYPTO_CERTIFICATE_INTERFACE_H_
