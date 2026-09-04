/**
 * @file runtime_config_bridge.cpp
 * @brief Adapts JSON-backed configuration transactions to a narrow interface.
 */

#include "runtime_config_bridge.hpp"

#include "config_handler.hpp"

#include <stdexcept>
#include <utility>

class RuntimeConfigCandidateStorage
{
public:
    PreparedConfigCandidate prepared;
};

void prepare_runtime_config_candidate(
    const std::string &filename,
    RuntimeConfigCandidate &candidate_out)
{
    auto storage = std::make_shared<RuntimeConfigCandidateStorage>();
    prepare_ini_config_candidate(filename, storage->prepared);

    candidate_out.storage = storage;
    candidate_out.normalized_config = storage->prepared.normalized_config;
    candidate_out.valid = storage->prepared.valid;
    candidate_out.transmit_enabled = storage->prepared.transmit_enabled;
    candidate_out.error_reason = storage->prepared.error_reason;
    candidate_out.warnings = storage->prepared.warnings;
    candidate_out.migration_required = storage->prepared.migration_required;
}

void commit_runtime_config_candidate(const RuntimeConfigCandidate &candidate)
{
    if (!candidate.storage)
        throw std::logic_error("Runtime configuration candidate has no prepared storage.");
    commit_config_candidate(candidate.storage->prepared);
}
