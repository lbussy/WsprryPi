#include "../privileged_network_startup_recovery.hpp"

#include <cassert>
#include <iostream>

namespace {
void immediate_success_is_idempotent()
{
    int reconciliations = 0;
    int listener_starts = 0;
    PrivilegedNetworkStartupRecovery recovery({
        [] { return false; },
        [&] {
            ++reconciliations;
            return PrivilegedNetworkReconciliationAttemptResult::ready;
        },
        [&] {
            ++listener_starts;
            return PrivilegedNetworkListenerStartResult::started;
        }});

    assert(recovery.attempt() == PrivilegedNetworkStartupRecoveryResult::ready);
    assert(recovery.attempt() == PrivilegedNetworkStartupRecoveryResult::ready);
    assert(recovery.ready());
    assert(reconciliations == 1);
    assert(listener_starts == 1);
}

void delayed_network_success_retries_without_starting_early()
{
    int reconciliations = 0;
    int listener_starts = 0;
    PrivilegedNetworkStartupRecovery recovery({
        [] { return false; },
        [&] {
            return ++reconciliations >= 3
                ? PrivilegedNetworkReconciliationAttemptResult::ready
                : PrivilegedNetworkReconciliationAttemptResult::retryable_failure;
        },
        [&] {
            ++listener_starts;
            return PrivilegedNetworkListenerStartResult::started;
        }});

    assert(recovery.attempt() ==
           PrivilegedNetworkStartupRecoveryResult::retry_pending);
    assert(recovery.attempt() ==
           PrivilegedNetworkStartupRecoveryResult::retry_pending);
    assert(listener_starts == 0);
    assert(recovery.attempt() == PrivilegedNetworkStartupRecoveryResult::ready);
    assert(reconciliations == 3);
    assert(listener_starts == 1);
}

void shutdown_prevents_reconciliation_and_listener_start()
{
    int reconciliations = 0;
    int listener_starts = 0;
    PrivilegedNetworkStartupRecovery recovery({
        [] { return true; },
        [&] {
            ++reconciliations;
            return PrivilegedNetworkReconciliationAttemptResult::ready;
        },
        [&] {
            ++listener_starts;
            return PrivilegedNetworkListenerStartResult::started;
        }});

    assert(recovery.attempt() ==
           PrivilegedNetworkStartupRecoveryResult::shutdown_requested);
    assert(reconciliations == 0);
    assert(listener_starts == 0);
}

void shutdown_after_reconciliation_prevents_listener_start()
{
    bool shutdown = false;
    int listener_starts = 0;
    PrivilegedNetworkStartupRecovery recovery({
        [&] { return shutdown; },
        [&] {
            shutdown = true;
            return PrivilegedNetworkReconciliationAttemptResult::ready;
        },
        [&] {
            ++listener_starts;
            return PrivilegedNetworkListenerStartResult::started;
        }});

    assert(recovery.attempt() ==
           PrivilegedNetworkStartupRecoveryResult::shutdown_requested);
    assert(listener_starts == 0);
}

void listener_failure_is_terminal_for_the_attempt()
{
    int listener_starts = 0;
    PrivilegedNetworkStartupRecovery recovery({
        [] { return false; },
        [] { return PrivilegedNetworkReconciliationAttemptResult::ready; },
        [&] {
            ++listener_starts;
            return PrivilegedNetworkListenerStartResult::failed;
        }});

    assert(recovery.attempt() ==
           PrivilegedNetworkStartupRecoveryResult::listener_start_failed);
    assert(!recovery.ready());
    assert(listener_starts == 1);
}

void shutdown_during_listener_start_is_preserved()
{
    PrivilegedNetworkStartupRecovery recovery({
        [] { return false; },
        [] { return PrivilegedNetworkReconciliationAttemptResult::ready; },
        [] { return PrivilegedNetworkListenerStartResult::shutdown_requested; }});

    assert(recovery.attempt() ==
           PrivilegedNetworkStartupRecoveryResult::shutdown_requested);
    assert(!recovery.ready());
}

void non_discovery_reconciliation_failure_does_not_retry()
{
    int listener_starts = 0;
    PrivilegedNetworkStartupRecovery recovery({
        [] { return false; },
        [] {
            return PrivilegedNetworkReconciliationAttemptResult::terminal_failure;
        },
        [&] {
            ++listener_starts;
            return PrivilegedNetworkListenerStartResult::started;
        }});

    assert(recovery.attempt() ==
           PrivilegedNetworkStartupRecoveryResult::reconciliation_failed);
    assert(!recovery.ready());
    assert(listener_starts == 0);
}
}

int main()
{
    immediate_success_is_idempotent();
    delayed_network_success_retries_without_starting_early();
    shutdown_prevents_reconciliation_and_listener_start();
    shutdown_after_reconciliation_prevents_listener_start();
    listener_failure_is_terminal_for_the_attempt();
    shutdown_during_listener_start_is_preserved();
    non_discovery_reconciliation_failure_does_not_retry();
    std::cout << "privileged_network_startup_recovery_test: PASS\n";
}
