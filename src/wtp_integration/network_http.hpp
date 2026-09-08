// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Lee Bussy
#pragma once
#include "tls.hpp"
#include <string>
namespace wsprrypi {
struct PicoHttpResponse {
  unsigned status{503};
  std::string body{R"({"error":{"code":"network_unavailable"}})"}, etag;
};
// One bounded request, already authorized/serialized by the application owner.
// Only standalone management resources; never forwards browser job operations.
PicoHttpResponse pico_http_request(TlsStream &, const TlsSelection &,
    std::shared_ptr<TlsCredentials>, TlsStream::Clock,
    const std::string &resource, const std::string &method,
    const std::string &body = {}, const std::string &revision = {});
} // namespace wsprrypi
