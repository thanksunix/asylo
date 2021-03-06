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

#include "asylo/platform/primitives/util/exit_log.h"

#include <functional>
#include <ostream>
#include <vector>

#include "absl/memory/memory.h"
#include "absl/time/clock.h"
#include "asylo/platform/primitives/util/dispatch_table.h"
#include "asylo/util/status.h"

namespace asylo {
namespace primitives {

ExitLogEntry::ExitLogEntry(absl::Time start, absl::Duration duration,
                           uint64_t untrusted_selector)
    : start_(start),
      duration_(duration),
      untrusted_selector_(untrusted_selector) {}

std::ostream& operator<<(std::ostream& os, const ExitLogEntry& entry) {
  os << "[Exit Call] Selector: " << entry.untrusted_selector_ << ": "
     << entry.start_ << " " << entry.duration_;
  return os;
}

ExitLogHook::ExitLogHook(std::function<void(ExitLogEntry)> store_log_entry)
    : store_log_entry_(store_log_entry) {}

Status ExitLogHook::PreExit(uint64_t untrusted_selector) {
  start_ = absl::Now();
  untrusted_selector_ = untrusted_selector;
  return Status::OkStatus();
}

Status ExitLogHook::PostExit(Status result) {
  auto duration = absl::Now() - start_;
  store_log_entry_(ExitLogEntry(start_, duration, untrusted_selector_));
  return result;
}

ExitLogHook::~ExitLogHook() {}

std::unique_ptr<DispatchTable::ExitHook> ExitLogHookFactory::CreateExitHook() {
  return absl::make_unique<ExitLogHook>(
      [](ExitLogEntry entry) { LOG(ERROR) << entry << std::endl; });
}

}  // namespace primitives
}  // namespace asylo
