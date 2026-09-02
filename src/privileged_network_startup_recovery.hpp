#pragma once

#include <functional>

enum class PrivilegedNetworkListenerStartResult
{
    started,
    shutdown_requested,
    failed
};

enum class PrivilegedNetworkReconciliationAttemptResult
{
    ready,
    retryable_failure,
    terminal_failure
};

enum class PrivilegedNetworkStartupRecoveryResult
{
    ready,
    retry_pending,
    reconciliation_failed,
    shutdown_requested,
    listener_start_failed
};

struct PrivilegedNetworkStartupRecoveryOperations
{
    std::function<bool()> shutdown_requested;
    std::function<PrivilegedNetworkReconciliationAttemptResult()>
        reconcile_policy;
    std::function<PrivilegedNetworkListenerStartResult()> start_listeners;
};

/**
 * @brief Coordinates fail-closed external-listener startup.
 *
 * A retryable policy-discovery failure leaves the coordinator pending. Once
 * the policy and listeners are ready, later calls are idempotent and do not
 * regenerate policy after an interface change. The caller owns terminal
 * reconciliation and listener-start failure policy.
 */
class PrivilegedNetworkStartupRecovery
{
public:
    explicit PrivilegedNetworkStartupRecovery(
        PrivilegedNetworkStartupRecoveryOperations operations);

    [[nodiscard]] PrivilegedNetworkStartupRecoveryResult attempt();
    [[nodiscard]] bool ready() const noexcept;

private:
    PrivilegedNetworkStartupRecoveryOperations operations_;
    bool ready_{false};
};
