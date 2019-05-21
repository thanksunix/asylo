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

#include "asylo/platform/primitives/sgx/trusted_sgx.h"

#include "absl/strings/str_cat.h"
#include "asylo/util/logging.h"
#include "asylo/platform/arch/sgx/trusted/generated_bridge_t.h"
#include "asylo/platform/core/entry_points.h"
#include "asylo/platform/primitives/extent.h"
#include "asylo/platform/primitives/primitive_status.h"
#include "asylo/platform/primitives/primitives.h"
#include "asylo/platform/primitives/sgx/sgx_error_space.h"
#include "asylo/platform/primitives/trusted_primitives.h"
#include "asylo/platform/primitives/trusted_runtime.h"
#include "asylo/platform/primitives/util/primitive_locks.h"
#include "asylo/platform/primitives/util/trusted_runtime_helper.h"
#include "asylo/platform/primitives/x86/spin_lock.h"
#include "asylo/util/error_codes.h"
#include "asylo/util/status.h"
#include "include/sgx_trts.h"

extern "C" void *enc_untrusted_malloc(size_t size);
extern "C" void enc_untrusted_free(void *ptr);

namespace asylo {
namespace primitives {

namespace {

#define CHECK_OCALL(status_)                                                 \
  do {                                                                       \
    sgx_status_t status##__COUNTER__ = status_;                              \
    if (status##__COUNTER__ != SGX_SUCCESS) {                                \
      TrustedPrimitives::DebugPuts(                                          \
          absl::StrCat(                                                      \
              __FILE__, ":", __LINE__, ": ",                                 \
              asylo::Status(status##__COUNTER__, "ocall failed").ToString()) \
              .c_str());                                                     \
      abort();                                                               \
    }                                                                        \
  } while (0)

// Handler installed by the runtime to initialize the enclave.
PrimitiveStatus Initialize(void *context, TrustedParameterStack *params) {
  auto output_len = params->Pop();
  auto output = params->Pop();
  const auto input = params->Pop();
  const auto enclave_name = params->Pop();
  int result = 0;
  try {
    result = asylo::__asylo_user_init(enclave_name->As<const char>(),
                                      /*config=*/input->As<const char>(),
                                      /*config_len=*/input->size(),
                                      output->As<char *>(),
                                      output_len->As<size_t>());
  } catch (...) {
    TrustedPrimitives::BestEffortAbort("Uncaught exception in enclave");
  }
  return PrimitiveStatus(result);
}

// Handler installed by the runtime to invoke the enclave run entry point.
PrimitiveStatus Run(void *context, TrustedParameterStack *params) {
  size_t *output_len = params->Pop()->As<size_t>();
  char **output = params->Pop()->As<char *>();
  auto input_extent = params->Pop();

  const char *input = input_extent->As<const char>();
  size_t input_len = input_extent->size();
  if (!enc_is_outside_enclave(*output, *output_len)
      || !enc_is_outside_enclave(input, input_len)) {
    Status(SGX_ERROR_INVALID_PARAMETER,
        "Unexpected reference to resource inside the enclave.");
  }
  int result = 0;
  try {
    result = asylo::__asylo_user_run(input, input_len, output, output_len);
  } catch (...) {
    TrustedPrimitives::BestEffortAbort("Uncaught exception in enclave");
  }
  return PrimitiveStatus(result);
}

}  // namespace

// Register SGX backend entry handlers.
void RegisterInternalHandlers() {
  // Register the enclave initialization entry handler.
  EntryHandler init_handler(Initialize);
  if (!TrustedPrimitives::RegisterEntryHandler(kSelectorAsyloInit, init_handler)
            .ok()) {
    TrustedPrimitives::BestEffortAbort("Could not register entry handler");
  }

  // Register the enclave run entry handler.
  EntryHandler run_handler(Run);
  if (!TrustedPrimitives::RegisterEntryHandler(kSelectorAsyloRun, run_handler)
           .ok()) {
    TrustedPrimitives::BestEffortAbort("Could not register entry handler");
  }
}

void TrustedPrimitives::BestEffortAbort(const char *message) {
  enc_block_ecalls();
  MarkEnclaveAborted();
  abort();
}

PrimitiveStatus TrustedPrimitives::RegisterEntryHandler(
    uint64_t selector, const EntryHandler &handler) {
  return asylo::primitives::RegisterEntryHandler(selector, handler);
}

int asylo_enclave_call(uint64_t selector, void *params) {
  PrimitiveStatus status = InvokeEntryHandler(
      selector, reinterpret_cast<TrustedParameterStack *>(params));
  return !status.ok();
}

void *TrustedPrimitives::UntrustedLocalAlloc(size_t size) {
  return enc_untrusted_malloc(size);
}

void TrustedPrimitives::UntrustedLocalFree(void *ptr) {
  enc_untrusted_free(ptr);
}

void TrustedPrimitives::DebugPuts(const char *message) {
  abort();
}

PrimitiveStatus TrustedPrimitives::UntrustedCall(
    uint64_t untrusted_selector, TrustedParameterStack *params) {
  int ret;

  if (enc_is_within_enclave(params, sizeof(TrustedParameterStack))) {
    abort();
  }

  CHECK_OCALL(ocall_dispatch_untrusted_call(&ret, untrusted_selector,
                                            reinterpret_cast<void *>(params)));
  return PrimitiveStatus(ret);
}

}  // namespace primitives
}  // namespace asylo
