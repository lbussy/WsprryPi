#include "privileged_network_startup_recovery.hpp"

#include <utility>

PrivilegedNetworkStartupRecovery::PrivilegedNetworkStartupRecovery(
    PrivilegedNetworkStartupRecoveryOperations operations)
    : operations_(std::move(operations)) {}

PrivilegedNetworkStartupRecoveryResult
PrivilegedNetworkStartupRecovery::attempt()
{
    if (ready_)
        return PrivilegedNetworkStartupRecoveryResult::ready;
    if (operations_.shutdown_requested())
        return PrivilegedNetworkStartupRecoveryResult::shutdown_requested;
    const auto reconciliation = operations_.reconcile_policy();
    if (reconciliation ==
        PrivilegedNetworkReconciliationAttemptResult::retryable_failure)
        return PrivilegedNetworkStartupRecoveryResult::retry_pending;
    if (reconciliation ==
        PrivilegedNetworkReconciliationAttemptResult::terminal_failure)
        return PrivilegedNetworkStartupRecoveryResult::reconciliation_failed;
    if (operations_.shutdown_requested())
        return PrivilegedNetworkStartupRecoveryResult::shutdown_requested;

    switch (operations_.start_listeners())
    {
    case PrivilegedNetworkListenerStartResult::started:
        ready_ = true;
        return PrivilegedNetworkStartupRecoveryResult::ready;
    case PrivilegedNetworkListenerStartResult::shutdown_requested:
        return PrivilegedNetworkStartupRecoveryResult::shutdown_requested;
    case PrivilegedNetworkListenerStartResult::failed:
        return PrivilegedNetworkStartupRecoveryResult::listener_start_failed;
    }
    return PrivilegedNetworkStartupRecoveryResult::listener_start_failed;
}

bool PrivilegedNetworkStartupRecovery::ready() const noexcept
{
    return ready_;
}
