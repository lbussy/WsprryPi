#include "support_bundle_http.hpp"

#include "json.hpp"

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>

namespace {
constexpr std::size_t kMaximumRequestBodyBytes = 1024;

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

nlohmann::json snapshot_json(const SupportBundleJobSnapshot &snapshot) {
    return {
        {"id", snapshot.id},
        {"state", state_name(snapshot.state)},
        {"probe_i2c_requested", snapshot.probe_i2c_requested},
        {"i2c_probe_status", snapshot.i2c_probe_status},
        {"failure_category", snapshot.failure_category},
        {"failure_message", snapshot.failure_message},
        {"download_available", false},
    };
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
        for (const auto &[key, value] : body.items()) {
            if (key != "probe_i2c" || !value.is_boolean()) {
                set_error(response, 400, "invalid_request");
                return;
            }
            probe_i2c = value.get<bool>();
        }

        std::string error;
        const auto snapshot = manager.create({probe_i2c}, error);
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
    server.Options(R"(/api/support-bundles/(.*))", [guard](const httplib::Request &request,
                                                              httplib::Response &response) {
        if (!guard(request, response)) {
            return;
        }
        remove_permissive_cors_headers(response);
        response.status = 204;
    });
}
