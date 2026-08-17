#include "support_bundle_http.hpp"

#include "json.hpp"
#include "support_bundle_download_preparation.hpp"
#include "support_bundle_download_file.hpp"
#include "support_bundle_encryption_production.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cctype>
#include <memory>
#include <optional>
#include <string>
#include <unistd.h>

namespace {
constexpr std::size_t kMaximumRequestBodyBytes = 8192;
constexpr std::size_t kDownloadReadBufferBytes = 64 * 1024;
std::string attachment_disposition(const std::string &basename);

void remove_permissive_cors_headers(httplib::Response &response) {
    response.headers.erase("Access-Control-Allow-Origin");
    response.headers.erase("Access-Control-Allow-Methods");
    response.headers.erase("Access-Control-Allow-Headers");
    response.headers.erase("Access-Control-Allow-Credentials");
}

void set_json(httplib::Response &response, int status, const nlohmann::json &body) {
    remove_permissive_cors_headers(response);
    response.status = status;
    response.set_content(body.dump(), "application/json");
}

void set_error(httplib::Response &response, int status, const char *error) {
    set_json(response, status, {{"error", error}});
}

void set_no_content(httplib::Response &response) {
    remove_permissive_cors_headers(response);
    response.status = 204;
}

void set_private_headers(httplib::Response &response) {
    response.set_header("Cache-Control", "no-store");
    response.set_header("X-Content-Type-Options", "nosniff");
}

void set_private_json(httplib::Response &response,
                      int status,
                      const nlohmann::json &body) {
    set_json(response, status, body);
    set_private_headers(response);
}

bool empty_identity(const SupportBundleIntakeProductionResult &result) {
    return result.generation == 0 && result.expires_at.empty() &&
           result.minimum_upload_version.empty() && result.signing_key_id.empty() &&
           result.bundle_key_id.empty() && !result.request_url &&
           result.release_url.empty() && !result.user_message;
}

bool active_intake_authorizes_encryption(const SupportBundleIntakeProductionResult &result) {
    return result.status == SupportBundleIntakeProductionStatus::active &&
           result.generation > 0 && !result.expires_at.empty() &&
           !result.minimum_upload_version.empty() && !result.signing_key_id.empty() &&
           result.bundle_key_id == kSupportBundleProductionKeyId && result.request_url &&
           !result.request_url->empty() && result.release_url.empty();
}

void set_intake_unavailable(httplib::Response &response) {
    set_private_json(response, 503, {{"status", "unavailable"}});
}

void set_intake_result(httplib::Response &response,
                       const SupportBundleIntakeProductionResult &result) {
    nlohmann::json body;
    switch (result.status) {
    case SupportBundleIntakeProductionStatus::active:
        if (result.generation == 0 || result.expires_at.empty() ||
            result.minimum_upload_version.empty() || result.signing_key_id.empty() ||
            result.bundle_key_id.empty() || !result.request_url ||
            result.request_url->empty() || !result.release_url.empty()) {
            set_intake_unavailable(response);
            return;
        }
        body = {{"status", "active"},
                {"generation", result.generation},
                {"expires_at", result.expires_at},
                {"minimum_upload_version", result.minimum_upload_version},
                {"signing_key_id", result.signing_key_id},
                {"bundle_key_id", result.bundle_key_id},
                {"request_url", *result.request_url}};
        break;
    case SupportBundleIntakeProductionStatus::disabled:
        if (result.generation == 0 || result.expires_at.empty() ||
            result.signing_key_id.empty() || result.bundle_key_id.empty() ||
            !result.minimum_upload_version.empty() || result.request_url ||
            !result.release_url.empty()) {
            set_intake_unavailable(response);
            return;
        }
        body = {{"status", "disabled"},
                {"generation", result.generation},
                {"expires_at", result.expires_at},
                {"signing_key_id", result.signing_key_id},
                {"bundle_key_id", result.bundle_key_id}};
        break;
    case SupportBundleIntakeProductionStatus::upgrade_required:
        if (result.minimum_upload_version.empty() || result.release_url.empty() ||
            result.generation != 0 || !result.expires_at.empty() ||
            !result.signing_key_id.empty() || !result.bundle_key_id.empty() ||
            result.request_url) {
            set_intake_unavailable(response);
            return;
        }
        body = {{"status", "upgrade_required"},
                {"minimum_upload_version", result.minimum_upload_version},
                {"release_url", result.release_url}};
        break;
    case SupportBundleIntakeProductionStatus::unavailable:
        if (!empty_identity(result)) {
            set_intake_unavailable(response);
            return;
        }
        set_intake_unavailable(response);
        return;
    }
    if (result.user_message) body["user_message"] = *result.user_message;
    set_private_json(response, 200, body);
}

void set_artifact_download(httplib::Response &response,
                           SupportBundleDownloadFile file,
                           std::string content_type,
                           std::function<void(bool, std::uint64_t)> completion = {}) {
    auto download = std::make_shared<SupportBundleDownloadFile>(std::move(file));
    response.status = 200;
    response.set_header("Content-Disposition", attachment_disposition(download->basename()));
    set_private_headers(response);
    response.set_content_provider(static_cast<std::size_t>(download->size()), content_type,
        [download](std::size_t offset, std::size_t length, httplib::DataSink &sink) {
            if (offset >= download->size() || length == 0) return false;
            std::array<char, kDownloadReadBufferBytes> buffer{};
            const auto count = std::min<std::uint64_t>({buffer.size(), length, download->size() - offset});
            const ssize_t read = pread(download->descriptor(), buffer.data(), count, offset);
            return read > 0 && sink.write(buffer.data(), static_cast<std::size_t>(read));
        }, [download, completion](bool success) {
            if (completion) completion(success, download->size());
        });
}

bool allow_support_request(const httplib::Request &request,
                           httplib::Response &response,
                           const SupportRequestGuardSnapshotProvider &snapshot_provider) {
    remove_permissive_cors_headers(response);
    const std::optional<std::string> origin = request.has_header("Origin")
                                                  ? std::optional<std::string>(request.get_header_value("Origin"))
                                                  : std::nullopt;
    const SupportRequestGuard guard(snapshot_provider());
    if (!guard.evaluate(request.remote_addr, request.get_header_value("Host"), origin).allowed()) {
        set_error(response, 403, "forbidden");
        return false;
    }
    return true;
}

std::string trim_ascii(std::string value) {
    const auto not_space = [](unsigned char character) {
        return character != ' ' && character != '\t' && character != '\r' && character != '\n';
    };
    const auto first = std::find_if(value.begin(), value.end(), not_space);
    const auto last = std::find_if(value.rbegin(), value.rend(), not_space).base();
    if (first >= last) {
        return {};
    }
    return {first, last};
}

bool is_json_content_type(const httplib::Request &request) {
    if (!request.has_header("Content-Type")) {
        return false;
    }
    const auto lowercase = [](std::string value) {
        std::transform(value.begin(), value.end(), value.begin(),
                       [](unsigned char character) {
                           return static_cast<char>(std::tolower(character));
                       });
        return value;
    };

    const std::string content_type = request.get_header_value("Content-Type");
    const auto separator = content_type.find(';');
    if (lowercase(trim_ascii(content_type.substr(0, separator))) != "application/json") {
        return false;
    }
    if (separator == std::string::npos) {
        return true;
    }

    const std::string parameter = trim_ascii(content_type.substr(separator + 1));
    const auto equals = parameter.find('=');
    if (parameter.empty() || parameter.find(';') != std::string::npos ||
        equals == std::string::npos || equals == 0 || equals + 1 == parameter.size()) {
        return false;
    }
    const std::string name = lowercase(trim_ascii(parameter.substr(0, equals)));
    const std::string value = lowercase(trim_ascii(parameter.substr(equals + 1)));
    return name == "charset" && value == "utf-8";
}

const char *state_name(SupportBundleJobState state) {
    switch (state) {
    case SupportBundleJobState::queued:
        return "queued";
    case SupportBundleJobState::running:
        return "running";
    case SupportBundleJobState::succeeded:
        return "succeeded";
    case SupportBundleJobState::failed:
        return "failed";
    }
    return "failed";
}

const char *private_state_name(SupportBundlePrivateLifecycle state) {
    switch (state) {
    case SupportBundlePrivateLifecycle::collecting: return "collecting";
    case SupportBundlePrivateLifecycle::candidate_ready: return "candidate_ready";
    case SupportBundlePrivateLifecycle::candidate_downloaded: return "candidate_downloaded";
    case SupportBundlePrivateLifecycle::finalized: return "finalized";
    case SupportBundlePrivateLifecycle::none: break;
    }
    return "";
}

nlohmann::json snapshot_json(const SupportBundleJobSnapshot &snapshot) {
    nlohmann::json output = {
        {"id", snapshot.id},
        {"state", state_name(snapshot.state)},
        {"probe_i2c_requested", snapshot.probe_i2c_requested},
        {"i2c_probe_status", snapshot.i2c_probe_status},
        {"failure_category", snapshot.failure_category},
        {"failure_message", snapshot.failure_message},
        {"download_available", snapshot.download_available},
    };
    if (snapshot.private_lifecycle != SupportBundlePrivateLifecycle::none) {
        output["case_id"] = snapshot.case_id;
        output["workflow_state"] = private_state_name(snapshot.private_lifecycle);
    }
    return output;
}

std::string attachment_disposition(const std::string &basename) {
    std::string value = "attachment; filename=\"";
    for (const char character : basename) {
        if (character == '\\' || character == '"') {
            value.push_back('\\');
        }
        value.push_back(character);
    }
    value.push_back('"');
    return value;
}

void set_download_error(httplib::Response &response,
                        SupportBundleDownloadPreparationFailure failure) {
    switch (failure) {
    case SupportBundleDownloadPreparationFailure::unavailable:
        set_error(response, 503, "artifact_unavailable");
        return;
    case SupportBundleDownloadPreparationFailure::unsafe:
        set_error(response, 409, "artifact_unsafe");
        return;
    case SupportBundleDownloadPreparationFailure::corrupt:
        set_error(response, 409, "artifact_corrupt");
        return;
    case SupportBundleDownloadPreparationFailure::internal:
        set_error(response, 500, "internal_error");
        return;
    case SupportBundleDownloadPreparationFailure::none:
        break;
    }
    set_error(response, 500, "internal_error");
}
}  // namespace

void register_support_bundle_http_routes(
    httplib::Server &server,
    SupportBundleJobManager &manager,
    SupportRequestGuardSnapshotProvider snapshot_provider,
    SupportBundleIntakeProductionProvider intake_provider) {
    const auto guard = [snapshot_provider = std::move(snapshot_provider)](
                           const httplib::Request &request,
                           httplib::Response &response) {
        return allow_support_request(request, response, snapshot_provider);
    };

    server.Get("/api/support-intake",
               [guard, intake_provider](
                   const httplib::Request &request,
                   httplib::Response &response) {
        if (!guard(request, response)) {
            set_private_headers(response);
            return;
        }
        if (!request.body.empty()) {
            set_error(response, 400, "invalid_request");
            set_private_headers(response);
            return;
        }
        try {
            if (!intake_provider) {
                set_intake_unavailable(response);
                return;
            }
            set_intake_result(response, intake_provider());
        } catch (...) {
            set_intake_unavailable(response);
        }
    });

    server.Options("/api/support-intake",
                   [guard](const httplib::Request &request,
                           httplib::Response &response) {
        if (!guard(request, response)) {
            set_private_headers(response);
            return;
        }
        set_no_content(response);
        set_private_headers(response);
    });

    server.Post("/api/support-bundles", [&manager, guard](const httplib::Request &request,
                                                            httplib::Response &response) {
        if (!guard(request, response)) {
            return;
        }
        if (request.body.size() > kMaximumRequestBodyBytes) {
            set_error(response, 413, "invalid_request");
            return;
        }
        if (!is_json_content_type(request)) {
            set_error(response, 415, "invalid_request");
            return;
        }

        nlohmann::json body;
        try {
            body = nlohmann::json::parse(request.body);
        } catch (...) {
            set_error(response, 400, "invalid_request");
            return;
        }
        if (!body.is_object()) {
            set_error(response, 400, "invalid_request");
            return;
        }

        bool probe_i2c = false;
        std::optional<SupportBundlePrivateRequest> private_request;
        for (const auto &[key, value] : body.items()) {
            if (key == "probe_i2c" && value.is_boolean()) {
                probe_i2c = value.get<bool>();
                continue;
            }
            if (key != "support_context" || !value.is_object() || private_request) {
                set_error(response, 400, "invalid_request");
                return;
            }
            SupportBundleContext context;
            const auto &context_json = value;
            if (!context_json.contains("kind") || !context_json["kind"].is_string()) {
                set_error(response, 400, "invalid_request"); return;
            }
            const std::string kind = context_json["kind"].get<std::string>();
            if (kind == "existing_github_issue") {
                context.kind = SupportBundleContextKind::existing_github_issue;
                if (context_json.size() != 2 || !context_json.contains("issue_url") ||
                    !context_json["issue_url"].is_string()) {
                    set_error(response, 400, "invalid_request"); return;
                }
                context.issue_url = context_json["issue_url"].get<std::string>();
            } else if (kind == "new_github_issue" || kind == "no_github") {
                context.kind = kind == "new_github_issue"
                                   ? SupportBundleContextKind::new_github_issue
                                   : SupportBundleContextKind::no_github;
                if (context_json.size() != 3 ||
                    !context_json.contains("problem_description") ||
                    !context_json["problem_description"].is_string() ||
                    !context_json.contains("contact") ||
                    !context_json["contact"].is_string()) {
                    set_error(response, 400, "invalid_request"); return;
                }
                context.problem_description =
                    context_json["problem_description"].get<std::string>();
                context.contact = context_json["contact"].get<std::string>();
            } else {
                set_error(response, 400, "invalid_request"); return;
            }
            if (!valid_support_bundle_context(context)) {
                set_error(response, 400, "invalid_request"); return;
            }
            private_request = SupportBundlePrivateRequest{std::move(context)};
        }

        std::string error;
        const auto snapshot = manager.create({probe_i2c, std::move(private_request)}, error);
        if (!snapshot) {
            if (error == "job_active") {
                set_error(response, 409, "job_active");
            } else if (error == "storage_unavailable") {
                set_error(response, 503, "unavailable");
            } else {
                set_error(response, 500, "unavailable");
            }
            return;
        }
        set_json(response, 202, snapshot_json(*snapshot));
    });

    server.Post(R"(/api/support-bundles/(.*)/finalize)",
                [&manager, guard](const httplib::Request &request,
                                  httplib::Response &response) {
        if (!guard(request, response)) return;
        if (!request.body.empty()) { set_error(response, 400, "invalid_request"); return; }
        const std::string id = request.matches.size() > 1 ? request.matches[1].str() : "";
        if (!SupportBundleJobManager::valid_id(id)) {
            set_error(response, 404, "not_found"); return;
        }
        const auto outcome = manager.finalize_candidate(id);
        switch (outcome.status) {
        case SupportBundleFinalizationStatus::finalized:
        case SupportBundleFinalizationStatus::already_finalized:
            set_json(response, 200, snapshot_json(*outcome.snapshot)); return;
        case SupportBundleFinalizationStatus::malformed_or_unknown_id:
            set_error(response, 404, "not_found"); return;
        case SupportBundleFinalizationStatus::not_private:
            set_error(response, 409, "not_private"); return;
        case SupportBundleFinalizationStatus::not_ready:
            set_error(response, 409, "not_ready"); return;
        case SupportBundleFinalizationStatus::download_required:
            set_error(response, 409, "download_required"); return;
        case SupportBundleFinalizationStatus::artifact_invalid:
            set_error(response, 409, "artifact_invalid"); return;
        }
        set_error(response, 500, "internal_error");
    });

    server.Post(R"(/api/support-bundles/(.*)/encrypt)",
        [&manager, guard, intake_provider](const httplib::Request &request, httplib::Response &response) {
        if (!guard(request, response)) return;
        if (!request.body.empty()) { set_private_json(response, 400, {{"error", "invalid_request"}}); return; }
        const std::string id = request.matches.size() > 1 ? request.matches[1].str() : "";
        if (!SupportBundleJobManager::valid_id(id)) { set_private_json(response, 404, {{"error", "not_found"}}); return; }
        SupportBundleIntakeProductionResult intake;
        try { intake = intake_provider(); } catch (...) {
            set_private_json(response, 503, {{"error", "intake_unavailable"}}); return;
        }
        if (!active_intake_authorizes_encryption(intake)) {
            set_private_json(response, 409, {{"error", "intake_not_active"}}); return;
        }
        const auto outcome = manager.encrypt_candidate(id, std::string(kSupportBundleProductionKeyId),
                                                        std::string(kSupportBundleProductionAgeRecipient));
        switch (outcome.status) {
        case SupportBundleEncryptionStatus::encrypted:
        case SupportBundleEncryptionStatus::already_encrypted:
            set_private_json(response, 200, {{"workflow_state", "encrypted"}}); return;
        case SupportBundleEncryptionStatus::malformed_or_unknown_id: set_private_json(response, 404, {{"error", "not_found"}}); return;
        case SupportBundleEncryptionStatus::not_finalized: set_private_json(response, 409, {{"error", "not_finalized"}}); return;
        case SupportBundleEncryptionStatus::key_mismatch: set_private_json(response, 409, {{"error", "key_mismatch"}}); return;
        default: set_private_json(response, 503, {{"error", "encryption_failed"}}); return;
        }
    });

    server.Get(R"(/api/support-bundles/(.*)/encrypted)",
        [&manager, guard](const httplib::Request &request, httplib::Response &response) {
        if (!guard(request, response)) { set_private_headers(response); return; }
        const std::string id = request.matches.size() > 1 ? request.matches[1].str() : "";
        auto opened = open_support_bundle_download_file(manager.encrypted_reference(id));
        if (!opened.available()) { set_private_json(response, 409, {{"error", "not_ready"}}); return; }
        set_artifact_download(response, std::move(opened.file), "application/octet-stream",
            [&manager, id](bool success, std::uint64_t size) {
                if (success) (void)manager.mark_encrypted_downloaded(id, size);
            });
    });

    server.Get(R"(/api/support-bundles/(.*)/receipt)",
        [&manager, guard](const httplib::Request &request, httplib::Response &response) {
        if (!guard(request, response)) { set_private_headers(response); return; }
        const std::string id = request.matches.size() > 1 ? request.matches[1].str() : "";
        auto opened = open_support_bundle_download_file(manager.receipt_reference(id), 16 * 1024);
        if (!opened.available()) { set_private_json(response, 409, {{"error", "not_ready"}}); return; }
        set_artifact_download(response, std::move(opened.file), "application/json");
    });

    server.Get(R"(/api/support-bundles/(.*)/download)",
               [&manager, guard](const httplib::Request &request,
                                 httplib::Response &response) {
        if (!guard(request, response)) {
            return;
        }

        const std::string id = request.matches.size() > 1 ? request.matches[1].str() : "";
        if (!SupportBundleJobManager::valid_id(id)) {
            set_error(response, 404, "not_found");
            return;
        }

        const auto reference = manager.download_reference(id);
        switch (reference.status) {
        case SupportBundleDownloadReferenceStatus::malformed_or_unknown_id:
            set_error(response, 404, "not_found");
            return;
        case SupportBundleDownloadReferenceStatus::not_ready:
            set_error(response, 409, "not_ready");
            return;
        case SupportBundleDownloadReferenceStatus::no_download:
            set_error(response, 409, "no_download");
            return;
        case SupportBundleDownloadReferenceStatus::available:
            break;
        }

        auto prepared = prepare_support_bundle_download(reference);
        if (!prepared.available()) {
            set_download_error(response, prepared.failure);
            return;
        }

        auto download = std::make_shared<SupportBundlePreparedDownload>(
            std::move(prepared.download));
        response.status = 200;
        response.set_header("Content-Disposition", attachment_disposition(download->basename()));
        response.set_header("Cache-Control", "no-store");
        response.set_header("X-Content-Type-Options", "nosniff");
        response.set_content_provider(
            static_cast<std::size_t>(download->size()), "application/gzip",
            [download](std::size_t offset, std::size_t length, httplib::DataSink &sink) {
                if (offset >= download->size() || length == 0) {
                    return false;
                }

                const std::uint64_t remaining = download->size() - offset;
                const std::size_t read_size = static_cast<std::size_t>(
                    std::min<std::uint64_t>({kDownloadReadBufferBytes, length, remaining}));
                std::array<char, kDownloadReadBufferBytes> buffer {};
                ssize_t read_count = 0;
                do {
                    read_count = pread(download->descriptor(), buffer.data(), read_size,
                                       static_cast<off_t>(offset));
                } while (read_count < 0 && errno == EINTR);

                return read_count > 0 &&
                       sink.write(buffer.data(), static_cast<std::size_t>(read_count));
            },
            [&manager, id, download](bool success) {
                if (success) (void)manager.mark_candidate_downloaded(id, download->size());
            });
    });

    server.Delete(R"(/api/support-bundles/(.*))", [&manager, guard](const httplib::Request &request,
                                                                       httplib::Response &response) {
        if (!guard(request, response)) {
            return;
        }
        const std::string id = request.matches.size() > 1 ? request.matches[1].str() : "";
        if (!SupportBundleJobManager::valid_id(id)) {
            set_error(response, 404, "not_found");
            return;
        }

        switch (manager.delete_download(id).status) {
        case SupportBundleDownloadDeletionStatus::removed:
        case SupportBundleDownloadDeletionStatus::already_removed:
            set_no_content(response);
            return;
        case SupportBundleDownloadDeletionStatus::malformed_or_unknown_id:
            set_error(response, 404, "not_found");
            return;
        case SupportBundleDownloadDeletionStatus::not_terminal:
            set_error(response, 409, "not_terminal");
            return;
        case SupportBundleDownloadDeletionStatus::no_retained_download:
            set_error(response, 409, "no_download");
            return;
        case SupportBundleDownloadDeletionStatus::cleanup_failed:
            set_error(response, 503, "cleanup_failed");
            return;
        }
        set_error(response, 500, "internal_error");
    });

    server.Get(R"(/api/support-bundles/(.*))", [&manager, guard](const httplib::Request &request,
                                                                    httplib::Response &response) {
        if (!guard(request, response)) {
            return;
        }
        const std::string id = request.matches.size() > 1 ? request.matches[1].str() : "";
        if (!SupportBundleJobManager::valid_id(id)) {
            set_error(response, 404, "not_found");
            return;
        }
        const auto snapshot = manager.lookup(id);
        if (!snapshot) {
            set_error(response, 404, "not_found");
            return;
        }
        set_json(response, 200, snapshot_json(*snapshot));
    });

    // This negative handler prevents a server-wide default CORS policy from
    // turning the collection root into a permissive listing endpoint.
    server.Get("/api/support-bundles", [guard](const httplib::Request &request,
                                                httplib::Response &response) {
        if (!guard(request, response)) {
            return;
        }
        set_error(response, 404, "not_found");
    });

    server.Options("/api/support-bundles", [guard](const httplib::Request &request,
                                                      httplib::Response &response) {
        if (!guard(request, response)) {
            return;
        }
        remove_permissive_cors_headers(response);
        response.status = 204;
    });
    server.Options(R"(/api/support-bundles/(.*)/download)",
                   [guard](const httplib::Request &request,
                           httplib::Response &response) {
        if (!guard(request, response)) {
            return;
        }
        remove_permissive_cors_headers(response);
        response.status = 204;
    });
    server.Options(R"(/api/support-bundles/(.*))", [guard](const httplib::Request &request,
                                                              httplib::Response &response) {
        if (!guard(request, response)) {
            return;
        }
        remove_permissive_cors_headers(response);
        response.set_header("Allow", "GET, DELETE, OPTIONS");
        response.status = 204;
    });
}
