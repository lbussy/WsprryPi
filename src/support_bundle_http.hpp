#pragma once

#include "httplib.hpp"
#include "support_bundle_job_manager.hpp"
#include "support_request_guard.hpp"

#include <functional>

using SupportRequestGuardSnapshotProvider =
    std::function<SupportRequestGuardSnapshot()>;

// Registers only the guarded support-bundle creation/status API. Production
// server wiring is intentionally deferred to a later slice.
void register_support_bundle_http_routes(
    httplib::Server &server,
    SupportBundleJobManager &manager,
    SupportRequestGuardSnapshotProvider snapshot_provider);
