#include "support_bundle_http.hpp"

#include "json.hpp"
#include "support_bundle_download_preparation.hpp"

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
    SupportRequestGuardSnapshotProvider snapshot_provider) {
    const auto guard = [snapshot_provider = std::move(snapshot_provider)](
                           const httplib::Request &request,
                           httplib::Response &response) {
        return allow_support_request(request, response, snapshot_provider);
    };

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
