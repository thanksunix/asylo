#
# Copyright 2018 Asylo authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

load("//asylo/bazel:copts.bzl", "ASYLO_DEFAULT_COPTS")
load(
    "//asylo/bazel:dlopen_enclave.bzl",
    "dlopen_enclave_loader",
    "dlopen_enclave_test",
    "primitives_dlopen_enclave",
)
load("//asylo/bazel:sgx_rules.bzl", "sgx_enclave_test")
load("@linux_sgx//:sgx_sdk.bzl", "sgx")

licenses(["notice"])

package(
    default_visibility = ["//visibility:public"],
)

primitives_dlopen_enclave(
    name = "hello_dlopen_enclave.so",
    srcs = [
        "hello_enclave.cc",
        "hello_enclave.h",
    ],
    copts = ASYLO_DEFAULT_COPTS,
    deps = [
        "//asylo/platform/primitives",
        "//asylo/platform/primitives/util:message_reader_writer",
        "//asylo/util:status_macros",
        "@com_google_absl//absl/strings",
    ],
)

dlopen_enclave_loader(
    name = "hello_dlopen_driver",
    srcs = [
        "hello_driver.cc",
        "hello_enclave.h",
    ],
    copts = ASYLO_DEFAULT_COPTS,
    enclaves = {"hello": ":hello_dlopen_enclave.so"},
    deps = [
        "//asylo/platform/primitives",
        "//asylo/platform/primitives:untrusted_primitives",
        "//asylo/platform/primitives/test:dlopen_test_backend",
        "//asylo/platform/primitives/test:test_backend",
        "//asylo/platform/primitives/util:dispatch_table",
        "//asylo/platform/primitives/util:message_reader_writer",
        "//asylo/util:status",
        "@com_google_absl//absl/flags:parse",
    ],
)

dlopen_enclave_test(
    name = "hello_dlopen_test",
    size = "small",
    srcs = [
        "hello_enclave.h",
        "hello_test.cc",
    ],
    copts = ASYLO_DEFAULT_COPTS,
    enclaves = {"hello": ":hello_dlopen_enclave.so"},
    test_args = [
        "--enclave_binary='{hello}'",
    ],
    deps = [
        "//asylo:enclave_client",
        "//asylo/platform/host_call:host_call_handlers_initializer",
        "//asylo/platform/primitives",
        "//asylo/platform/primitives:trusted_primitives",
        "//asylo/platform/primitives:untrusted_primitives",
        "//asylo/platform/primitives/test:dlopen_test_backend",
        "//asylo/platform/primitives/test:test_backend",
        "//asylo/platform/primitives/util:message_reader_writer",
        "//asylo/test/util:status_matchers",
        "//asylo/test/util:test_main",
        "//asylo/util:status",
        "@com_google_absl//absl/memory",
        "@com_google_googletest//:gtest",
    ],
)

sgx.unsigned_enclave(
    name = "hello_sgx_enclave_unsigned.so",
    srcs = [
        "hello_enclave.cc",
        "hello_enclave.h",
    ],
    copts = ASYLO_DEFAULT_COPTS,
    deps = [
        "//asylo/platform/crypto/gcmlib:trusted_gcmlib",
        "//asylo/platform/posix:trusted_posix",
        "//asylo/platform/primitives",
        "//asylo/platform/primitives:trusted_primitives",
        "//asylo/platform/primitives:trusted_runtime",
        "//asylo/platform/primitives/sgx:trusted_sgx",
        "//asylo/platform/primitives/util:message_reader_writer",
        "//asylo/platform/storage/secure:trusted_secure",
        "//asylo/platform/storage/utils:trusted_utils",
        "//asylo/platform/system",
        "//asylo/util:status_macros",
        "@com_google_absl//absl/strings",
    ],
)

sgx.debug_enclave(
    name = "hello_sgx_enclave.so",
    unsigned = "hello_sgx_enclave_unsigned.so",
)

sgx_enclave_test(
    name = "hello_sgx_test",
    size = "small",
    srcs = [
        "hello_enclave.h",
        "hello_test.cc",
    ],
    copts = ASYLO_DEFAULT_COPTS,
    enclaves = {"hello": ":hello_sgx_enclave.so"},
    test_args = [
        "--enclave_binary='{hello}'",
    ],
    deps = [
        "//asylo:enclave_client",
        "//asylo/platform/arch:untrusted_arch",  # ocall_table_bridge symbol linkage
        "//asylo/platform/host_call:host_call_handlers_initializer",
        "//asylo/platform/primitives",
        "//asylo/platform/primitives:untrusted_primitives",
        "//asylo/platform/primitives/sgx:untrusted_sgx",
        "//asylo/platform/primitives/test:sgx_test_backend",
        "//asylo/platform/primitives/test:test_backend",
        "//asylo/platform/primitives/util:message_reader_writer",
        "//asylo/test/util:status_matchers",
        "//asylo/test/util:test_main",
        "//asylo/util:status",
        "@com_google_absl//absl/memory",
        "@com_google_googletest//:gtest",
    ],
)
