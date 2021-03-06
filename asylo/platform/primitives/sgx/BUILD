#
# Copyright 2019 Asylo authors
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

load("//asylo/bazel:asylo.bzl", "cc_enclave_test")
load("//asylo/bazel:copts.bzl", "ASYLO_DEFAULT_COPTS")
load("@linux_sgx//:sgx_sdk.bzl", "sgx")

licenses(["notice"])

package(
    default_visibility = ["//asylo:implementation"],
)

exports_files([
    "enclave_loader.cc",
])

cc_library(
    name = "sgx_params",
    hdrs = ["sgx_params.h"],
    copts = ASYLO_DEFAULT_COPTS,
)

# Trusted runtime components for SGX.
_TRUSTED_SGX_BACKEND_DEPS = [
    ":sgx_error_space",
    ":trusted_sgx_bridge",
    "//asylo:enclave_cc_proto",
    "//asylo/util:logging",
    "//asylo/platform/posix/signal:signal_manager",
    "//asylo/platform/posix/threading:thread_manager",
    "//asylo/platform/primitives",
    "//asylo/platform/primitives/util:trusted_runtime_helper",
    "//asylo/util:error_codes",
    "//asylo/util:status",
    "@linux_sgx//:public",
    "@linux_sgx//:trts",
]

_TRUSTED_SGX_HW_SRCS = [
    "hardware_random_sgx_hw.cc",
    "register_signal_sgx_hw.cc",
]

_TRUSTED_SGX_SIM_SRCS = [
    "hardware_random_sgx_sim.cc",
    "register_signal_sgx_sim.cc",
]

cc_library(
    name = "trusted_sgx",
    srcs = [
        "exceptions.cc",
        "trusted_runtime.cc",
        "trusted_sgx.cc",
        "enclave_syscalls.cc",
        "untrusted_cache_malloc.cc",
    ] + select(
        {
            "@linux_sgx//:sgx_hw": _TRUSTED_SGX_HW_SRCS,
            "@linux_sgx//:sgx_sim": _TRUSTED_SGX_SIM_SRCS,
        },
        no_match_error = "Expected an SGX backend configuration (e.g., --config=sgx)",
    ),
    hdrs = [
        "trusted_sgx.h",
        "untrusted_cache_malloc.h",
    ],
    copts = select(
        {
            "@linux_sgx//:sgx_hw": [
                "-mrdrnd",  # All SGX chips also support RDRAND
            ],
            "@linux_sgx//:sgx_sim": [],
        },
        "Must be built for SGX hardware or simulation backend",
    ) + ASYLO_DEFAULT_COPTS + ["-faligned-new"],
    tags = [
        "asylo-sgx",
        "manual",
    ],
    deps = select(
        {
            "@com_google_asylo//asylo": [
                "//asylo/platform/primitives:trusted_primitives",
                "//asylo/platform/primitives:trusted_runtime",
                "//asylo/platform/system_call/type_conversions",
                "//asylo/platform/core:trusted_spin_lock",
            ],
        },
        no_match_error = "Must be built in the Asylo toolchain",
    ) + select(
        {
            "@linux_sgx//:sgx_hw": _TRUSTED_SGX_BACKEND_DEPS,
            "@linux_sgx//:sgx_sim": _TRUSTED_SGX_BACKEND_DEPS,
        },
        no_match_error = "Trusted SGX components must be built with --define=ASYLO_SGX=1",
    ) + [
        ":sgx_params",
        "@com_google_absl//absl/strings",
        "//asylo/platform/primitives/util:message_reader_writer",
        "//asylo/util:cleanup",
        "@com_google_absl//absl/memory",
    ],
    alwayslink = 1,
)

cc_library(
    name = "untrusted_sgx",
    srcs = [
        "signal_dispatcher.cc",
        "untrusted_sgx.cc",
    ],
    hdrs = [
        "signal_dispatcher.h",
        "untrusted_sgx.h",
    ],
    copts = ASYLO_DEFAULT_COPTS,
    deps = [
        ":exit_handlers",
        ":fork_cc_proto",
        ":sgx_error_space",
        ":sgx_params",
        "//asylo:enclave_cc_proto",
        "//asylo/platform/primitives:untrusted_primitives",
        "//asylo/platform/primitives/util:message_reader_writer",
        "//asylo/util:cleanup",
        "//asylo/util:elf_reader",
        "//asylo/util:file_mapping",
        "//asylo/util:status",
        "//asylo/util:status_macros",
        "@com_google_absl//absl/strings",
        "@linux_sgx//:public",
        "@linux_sgx//:urts",
    ],
)

# An error space for SGX.
cc_library(
    name = "sgx_error_space",
    srcs = ["sgx_error_space.cc"],
    hdrs = ["sgx_error_space.h"],
    copts = ASYLO_DEFAULT_COPTS,
    deps = [
        "//asylo/util:status",
        "@linux_sgx//:public",
    ],
)

# Message definitions for SGX enclave loader.
proto_library(
    name = "loader_proto",
    srcs = ["loader.proto"],
    visibility = ["//visibility:public"],
    deps = [
        "//asylo:enclave_proto",
    ],
)

cc_proto_library(
    name = "loader_cc_proto",
    visibility = ["//visibility:public"],
    deps = [":loader_proto"],
)

cc_library(
    name = "exit_handlers",
    srcs = ["exit_handlers.cc"],
    hdrs = ["exit_handlers.h"],
    copts = ASYLO_DEFAULT_COPTS,
    deps = [
        "//asylo/platform/primitives:untrusted_primitives",
        "//asylo/platform/primitives/util:message_reader_writer",
        "//asylo/util:logging",
        "//asylo/util:status",
        "//asylo/util:status_macros",
        "//asylo/util:thread",
    ],
)

cc_library(
    name = "sgx_remote_proxy_lib",
    srcs = ["sgx_remote_proxy.cc"],
    copts = ASYLO_DEFAULT_COPTS,
    deps = [
        ":loader_cc_proto",
        ":untrusted_sgx",
        "//asylo:enclave_cc_proto",
        "//asylo/platform/arch:untrusted_arch",
        "//asylo/platform/primitives",
        "//asylo/platform/primitives:untrusted_primitives",
        "//asylo/platform/primitives/remote/util:remote_proxy_lib",
        "//asylo/platform/primitives/util:message_reader_writer",
        "//asylo/util:error_codes",
        "//asylo/util:logging",
        "//asylo/util:status",
        "//asylo/util/remote:remote_loader_cc_proto",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
    ],
    alwayslink = 1,
)

# Protobuf used by fork.
proto_library(
    name = "fork_proto",
    srcs = ["fork.proto"],
    visibility = ["//visibility:public"],
    deps = ["//asylo:enclave_proto"],
)

cc_proto_library(
    name = "fork_cc_proto",
    visibility = ["//visibility:public"],
    deps = [":fork_proto"],
)

# Fork related runtime.
_TRUSTED_FORK_HW_DEPS = [
    ":trusted_sgx",
    "@com_google_absl//absl/base:core_headers",
    "//asylo/crypto:aead_cryptor",
    "//asylo/crypto/util:bssl_util",
    "//asylo/crypto/util:byte_container_view",
    "//asylo/crypto/util:trivial_object_util",
    "//asylo/platform/posix/memory:memory",
    "//asylo/platform/primitives/sgx:sgx_error_space",
    "//asylo/util:logging",
    "//asylo/grpc/auth/core:client_ekep_handshaker",
    "//asylo/grpc/auth/core:server_ekep_handshaker",
    "//asylo/identity:descriptions",
    "//asylo/identity:identity_acl_evaluator",
    "//asylo/identity/sgx:sgx_identity_expectation_matcher",
    "//asylo/identity/sgx:sgx_identity_util",
    "//asylo/identity/sgx:sgx_local_assertion_generator",
    "//asylo/identity/sgx:sgx_local_assertion_verifier",
    "//asylo/util:cleansing_types",
    "//asylo/util:cleanup",
    "@boringssl//:crypto",
]

cc_library(
    name = "trusted_fork",
    srcs = ["fork.cc"] + select({
        "@linux_sgx//:sgx_hw": ["fork_sgx_hw.cc"],
        "//conditions:default": ["fork_sgx_sim.cc"],
    }),
    hdrs = [
        "fork.h",
        "fork_internal.h",
    ],
    copts = ASYLO_DEFAULT_COPTS,
    linkstatic = 1,
    tags = [
        "asylo-sgx",
        "manual",
    ],
    deps = [
        ":fork_cc_proto",
        "@com_google_absl//absl/strings",
        "//asylo:enclave_cc_proto",
        "//asylo/platform/core:status_serializer",
        "//asylo/platform/core:trusted_core",
        "//asylo/platform/host_call",
        "//asylo/platform/posix/threading:thread_manager",
        "//asylo/util:status",
    ] + select(
        {"@com_google_asylo//asylo": [
            "//asylo/platform/primitives:trusted_primitives",
            "//asylo/platform/primitives:trusted_runtime",
        ]},
        no_match_error = "Must be built in Asylo toolchain",
    ) + select({
        "@linux_sgx//:sgx_hw": _TRUSTED_FORK_HW_DEPS,
        "@linux_sgx//:sgx_sim": [":trusted_sgx"],
        "//conditions:default": [],
    }),
)

sgx.enclave_configuration(
    name = "many_threads_enclave_config",
    tcs_num = "1000",
)

cc_enclave_test(
    name = "untrusted_cache_malloc_test",
    srcs = ["untrusted_cache_malloc_test.cc"],
    backends = sgx.backend_labels,
    copts = ASYLO_DEFAULT_COPTS,
    enclave_config = ":many_threads_enclave_config",
    deps = [
        ":trusted_sgx",
        "@com_google_googletest//:gtest",
    ],
)

# Command to invoke the sgx edger8r tool to generate the bridge code.

ASYLO_PREFIX = "external/com_google_asylo/"

SGX_EDGER8R_COMMAND = (
    "./$(location @linux_sgx//:sgx_edger8r) " +
    "$(location bridge.edl) " +
    "--search-path " + ASYLO_PREFIX + "asylo/platform/primitives/sgx " +
    "--search-path asylo/platform/primitives/sgx " +
    "--search-path $(@D) " +  # Reference previously generated files
    "--trusted --trusted-dir $(@D)/ " +
    "--untrusted --untrusted-dir $(@D)/"
)

# Command to update the generated bridge files and header paths for the
# SDK dependencies.

SGX_PREFIX = ""

UPDATE_SDK_DEPS_COMMAND = (
    "for file in $$(ls $(@D)/bridge_*.[ch]); do " +
    "sed -i " +
    "-e '1s!^!// Generated file. See bridge.edl\\n!' " +
    "-e 's!bridge_u.h!generated_bridge_u.h!' " +
    "-e 's!bridge_t.h!generated_bridge_t.h!' " +
    "-e 's!sgx_trts.h!" + SGX_PREFIX + "include/sgx_trts.h!' " +
    "-e 's!sgx_edger8r.h!" + SGX_PREFIX + "include/sgx_edger8r.h!' " +
    "-e 's!sgx_lfence.h!" + SGX_PREFIX + "include/sgx_lfence.h!' " +
    "-e 's!<mbusafecrt.h>!\"" + SGX_PREFIX + "include/se_memcpy.h\"!' " +
    "$$file && " +
    "mv $$file $$(dirname $$file)/generated_$$(basename $$file); " +
    "done"
)

# The bridge code generated by the Intel SGX SDK edger8r tool.
genrule(
    name = "generate_bridge",
    srcs = [
        "bridge.edl",
        "errno.edl",
        "//asylo/third_party/intel:sgx_tstdc.edl",
    ],
    outs = [
        "generated_bridge_t.c",
        "generated_bridge_t.h",
        "generated_bridge_u.c",
        "generated_bridge_u.h",
    ],
    cmd = (SGX_EDGER8R_COMMAND + " && " + UPDATE_SDK_DEPS_COMMAND),
    tools = ["@linux_sgx//:sgx_edger8r"],
)

cc_library(
    name = "untrusted_sgx_ocalls",
    srcs = [
        "generated_bridge_u.c",
        "generated_bridge_u.h",
        "ocalls.cc",
    ],
    copts = ASYLO_DEFAULT_COPTS,
    # This option prevents the linker from discarding the definition of
    # ocall_table_bridge before it encounters a reference to it.
    linkopts = ["-Wl,--undefined=ocall_table_bridge"],
    deps = [
        ":fork_cc_proto",
        ":loader_cc_proto",
        ":sgx_error_space",
        ":sgx_params",
        ":untrusted_sgx",
        "//asylo/platform/common:futex",
        "//asylo/platform/common:memory",
        "//asylo/platform/primitives:untrusted_primitives",
        "//asylo/platform/primitives/util:dispatch_table",
        "//asylo/platform/primitives/util:message_reader_writer",
        "//asylo/platform/storage/utils:fd_closer",
        "//asylo/platform/system_call/type_conversions",
        "//asylo/util:elf_reader",
        "//asylo/util:file_mapping",
        "//asylo/util:logging",
        "//asylo/util:status",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/strings",
        "@linux_sgx//:public",
        "@linux_sgx//:urts",
        "@sgx_dcap//:pce_wrapper",
        "@sgx_dcap//:quote_wrapper",
    ],
)

cc_library(
    name = "trusted_sgx_ecalls",
    srcs = [
        "ecalls.cc",
    ],
    copts = ASYLO_DEFAULT_COPTS,
    linkstatic = 1,
    tags = [
        "asylo-sgx",
        "manual",
    ],
    deps = [
        ":trusted_fork",
        ":trusted_sgx",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
        "//asylo/platform/common:memory",
        "//asylo/platform/core:entry_points",
        "//asylo/platform/core:shared_name",
        "//asylo/platform/host_call",
        "//asylo/platform/primitives:trusted_primitives",
    ] + select(
        {
            "@linux_sgx//:sgx_hw": _TRUSTED_SGX_BACKEND_DEPS,
            "@linux_sgx//:sgx_sim": _TRUSTED_SGX_BACKEND_DEPS,
        },
        no_match_error = "Must be built in SGX backend",
    ),
    alwayslink = 1,
)

# Trusted side of the SGX SDK generated bridge.
_TRUSTED_SGX_BRIDGE_BACKEND_DEPS = ["@linux_sgx//:public"]

cc_library(
    name = "trusted_sgx_bridge",
    srcs = ["generated_bridge_t.c"],
    hdrs = [
        "generated_bridge_t.h",
    ],
    copts = ASYLO_DEFAULT_COPTS,
    linkstatic = 1,
    tags = [
        "asylo-sgx",
        "manual",
    ],
    deps = select(
        {
            "@linux_sgx//:sgx_hw": _TRUSTED_SGX_BRIDGE_BACKEND_DEPS,
            "@linux_sgx//:sgx_sim": _TRUSTED_SGX_BRIDGE_BACKEND_DEPS,
        },
        no_match_error = "Must be built in SGX backend",
    ) + select(
        {"@com_google_asylo//asylo": ["@linux_sgx//:trts"]},
        no_match_error = "Must be built in Asylo toolchain",
    ),
)
