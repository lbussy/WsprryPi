#include "support_bundle_intake_validation.hpp"
#include "json.hpp"

#include <openssl/evp.h>

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

using json = nlohmann::json;

namespace {

using Key = std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>;
using Context = std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>;
using KeygenContext = std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)>;

struct Fixture {
    Key key{nullptr, EVP_PKEY_free};
    SupportBundleIntakeSigningKey pinned;
};

Fixture make_fixture(const std::string &key_id = "wsprrypi-intake-2099-99") {
    KeygenContext context(EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, nullptr), EVP_PKEY_CTX_free);
    assert(context && EVP_PKEY_keygen_init(context.get()) == 1);
    EVP_PKEY *raw = nullptr;
    assert(EVP_PKEY_keygen(context.get(), &raw) == 1);
    Fixture fixture;
    fixture.key.reset(raw);
    fixture.pinned.key_id = key_id;
    std::size_t size = fixture.pinned.public_key.size();
    assert(EVP_PKEY_get_raw_public_key(fixture.key.get(), fixture.pinned.public_key.data(), &size) == 1);
    assert(size == fixture.pinned.public_key.size());
    return fixture;
}

std::string base64url(const std::vector<unsigned char> &bytes) {
    static constexpr char alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    std::string encoded;
    std::uint32_t accumulator = 0;
    unsigned bits = 0;
    for (const auto byte : bytes) {
        accumulator = (accumulator << 8) | byte;
        bits += 8;
        while (bits >= 6) {
            bits -= 6;
            encoded.push_back(alphabet[(accumulator >> bits) & 63]);
        }
    }
    if (bits) encoded.push_back(alphabet[(accumulator << (6 - bits)) & 63]);
    return encoded;
}

std::string envelope_for(EVP_PKEY *key, const std::string &key_id, const std::string &manifest) {
    Context context(EVP_MD_CTX_new(), EVP_MD_CTX_free);
    assert(context && EVP_DigestSignInit(context.get(), nullptr, nullptr, nullptr, key) == 1);
    std::size_t size = 0;
    assert(EVP_DigestSign(context.get(), nullptr, &size,
                          reinterpret_cast<const unsigned char *>(manifest.data()), manifest.size()) == 1);
    std::vector<unsigned char> signature(size);
    assert(EVP_DigestSign(context.get(), signature.data(), &size,
                          reinterpret_cast<const unsigned char *>(manifest.data()), manifest.size()) == 1);
    signature.resize(size);
    return json({{"schema_version", 1}, {"algorithm", "Ed25519"}, {"key_id", key_id},
                 {"signature", base64url(signature)}}).dump();
}

json valid_manifest() {
    return {
        {"schema_version", 1},
        {"project_id", "wsprrypi"},
        {"generation", 7},
        {"published_at", "2026-08-16T18:00:00Z"},
        {"expires_at", "2026-08-17T18:00:00Z"},
        {"status", "active"},
        {"minimum_client_protocol", 1},
        {"minimum_upload_version", "1.3.0"},
        {"request_url", "https://www.dropbox.com/request/TestOpaque_123"},
        {"release_url", "https://github.com/WsprryPi/WsprryPi/releases/latest"},
        {"user_message", nullptr},
        {"bundle_encryption_key_id", "wsprrypi-bundle-2099-99"},
    };
}

SupportBundleIntakeValidationRequest request_for(const Fixture &fixture, const std::string &manifest) {
    return {manifest, envelope_for(fixture.key.get(), fixture.pinned.key_id, manifest),
            {fixture.pinned}, {"wsprrypi-bundle-2099-99"}, 1786906800, 1, std::nullopt};
}

void expect(const Fixture &fixture, json manifest, SupportBundleIntakeFailure failure) {
    const auto bytes = manifest.dump();
    assert(validate_support_bundle_intake(request_for(fixture, bytes)).failure == failure);
}

void assert_no_manifest_disclosure(const SupportBundleIntakeValidationResult &result) {
    assert(!result.valid());
    assert(result.manifest.generation == 0);
    assert(result.manifest.published_at.empty() && result.manifest.expires_at.empty());
    assert(result.manifest.status.empty() && result.manifest.minimum_upload_version.empty());
    assert(!result.manifest.request_url && result.manifest.release_url.empty());
    assert(!result.manifest.user_message && result.manifest.bundle_encryption_key_id.empty());
    assert(result.manifest.manifest_sha256.empty() && result.manifest.signing_key_id.empty());
}

void test_valid_active_and_disabled() {
    auto fixture = make_fixture();
    auto bytes = valid_manifest().dump();
    auto result = validate_support_bundle_intake(request_for(fixture, bytes));
    assert(result.valid());
    assert(result.manifest.generation == 7);
    assert(result.manifest.status == "active");
    assert(result.manifest.request_url == "https://www.dropbox.com/request/TestOpaque_123");
    assert(result.manifest.signing_key_id == fixture.pinned.key_id);
    assert(result.manifest.manifest_sha256.size() == 64);

    auto disabled = valid_manifest();
    disabled["status"] = "disabled";
    disabled.erase("request_url");
    disabled["user_message"] = "Uploads are temporarily unavailable.";
    result = validate_support_bundle_intake(request_for(fixture, disabled.dump()));
    assert(result.valid() && !result.manifest.request_url && result.manifest.user_message);
}

void test_signature_boundary() {
    auto fixture = make_fixture();
    auto unverified = valid_manifest();
    unverified["request_url"] = "https://www.dropbox.com/request/UNVERIFIED_ONLY";
    unverified["release_url"] = "https://github.com/WsprryPi/WsprryPi/releases/UNVERIFIED_ONLY";
    unverified["user_message"] = "UNVERIFIED MESSAGE MUST NOT ESCAPE";
    const auto manifest = unverified.dump();
    auto request = request_for(fixture, manifest);
    request.manifest_bytes.push_back('\n');
    auto result = validate_support_bundle_intake(request);
    assert(result.failure == SupportBundleIntakeFailure::invalid_signature);
    assert_no_manifest_disclosure(result);

    request = request_for(fixture, manifest);
    auto envelope = json::parse(request.signature_envelope_bytes);
    auto signature = envelope["signature"].get<std::string>();
    signature[0] = signature[0] == 'A' ? 'B' : 'A';
    envelope["signature"] = signature;
    request.signature_envelope_bytes = envelope.dump();
    assert(validate_support_bundle_intake(request).failure == SupportBundleIntakeFailure::invalid_signature);

    request = request_for(fixture, manifest);
    request.signature_envelope_bytes = request.signature_envelope_bytes.substr(
        0, request.signature_envelope_bytes.size() - 1) + ",\"key_id\":\"duplicate\"}";
    assert(validate_support_bundle_intake(request).failure == SupportBundleIntakeFailure::invalid_signature_envelope);

    auto other = make_fixture("wsprrypi-intake-2099-98");
    request = request_for(other, manifest);
    request.signing_keys = {fixture.pinned};
    assert(validate_support_bundle_intake(request).failure == SupportBundleIntakeFailure::unknown_signing_key);

    request = request_for(fixture, manifest);
    envelope = json::parse(request.signature_envelope_bytes);
    envelope["unexpected"] = true;
    request.signature_envelope_bytes = envelope.dump();
    assert(validate_support_bundle_intake(request).failure == SupportBundleIntakeFailure::invalid_signature_envelope);

    request = request_for(fixture, manifest);
    envelope = json::parse(request.signature_envelope_bytes);
    envelope["signature"] = "not_base64url";
    request.signature_envelope_bytes = envelope.dump();
    assert(validate_support_bundle_intake(request).failure == SupportBundleIntakeFailure::invalid_signature_envelope);

    auto wrong_public_key = make_fixture(fixture.pinned.key_id);
    request = request_for(fixture, manifest);
    request.signing_keys = {wrong_public_key.pinned};
    result = validate_support_bundle_intake(request);
    assert(result.failure == SupportBundleIntakeFailure::invalid_signature);
    assert_no_manifest_disclosure(result);

    request = request_for(fixture, manifest);
    envelope = json::parse(request.signature_envelope_bytes);
    envelope["signature"] = envelope["signature"].get<std::string>() + "=";
    request.signature_envelope_bytes = envelope.dump();
    assert(validate_support_bundle_intake(request).failure == SupportBundleIntakeFailure::invalid_signature_envelope);

    request = request_for(fixture, manifest);
    envelope = json::parse(request.signature_envelope_bytes);
    auto noncanonical = envelope["signature"].get<std::string>();
    static constexpr std::string_view alphabet =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    const auto tail = alphabet.find(noncanonical.back());
    assert(tail != std::string_view::npos && (tail & 0x0f) == 0);
    noncanonical.back() = alphabet[tail + 1];
    envelope["signature"] = noncanonical;
    request.signature_envelope_bytes = envelope.dump();
    assert(validate_support_bundle_intake(request).failure == SupportBundleIntakeFailure::invalid_signature_envelope);

    auto malformed_id = make_fixture("wsprrypi-intake-test-01");
    request = request_for(malformed_id, manifest);
    assert(validate_support_bundle_intake(request).failure == SupportBundleIntakeFailure::invalid_signature_envelope);
}

void test_strict_manifest_and_policy() {
    auto fixture = make_fixture();
    auto manifest = valid_manifest();
    manifest["project_id"] = "other";
    expect(fixture, manifest, SupportBundleIntakeFailure::wrong_project);
    manifest = valid_manifest(); manifest["unexpected"] = true;
    expect(fixture, manifest, SupportBundleIntakeFailure::invalid_manifest);
    manifest = valid_manifest(); manifest.erase("expires_at");
    expect(fixture, manifest, SupportBundleIntakeFailure::invalid_manifest);
    manifest = valid_manifest(); manifest["generation"] = "7";
    expect(fixture, manifest, SupportBundleIntakeFailure::invalid_manifest);
    manifest = valid_manifest(); manifest["generation"] = 0;
    expect(fixture, manifest, SupportBundleIntakeFailure::invalid_generation);
    manifest = valid_manifest(); manifest["generation"] = -1;
    expect(fixture, manifest, SupportBundleIntakeFailure::invalid_manifest);
    manifest = valid_manifest(); manifest["published_at"] = "2026-02-30T00:00:00Z";
    expect(fixture, manifest, SupportBundleIntakeFailure::invalid_time);
    manifest = valid_manifest(); manifest["expires_at"] = "2026-08-16T17:00:00Z";
    expect(fixture, manifest, SupportBundleIntakeFailure::invalid_time);
    manifest = valid_manifest(); manifest["status"] = "paused";
    expect(fixture, manifest, SupportBundleIntakeFailure::unsupported_status);
    manifest = valid_manifest(); manifest["status"] = "disabled";
    expect(fixture, manifest, SupportBundleIntakeFailure::invalid_request_url);
    manifest = valid_manifest(); manifest.erase("request_url");
    expect(fixture, manifest, SupportBundleIntakeFailure::invalid_request_url);
    manifest = valid_manifest(); manifest["minimum_client_protocol"] = 2;
    expect(fixture, manifest, SupportBundleIntakeFailure::incompatible_client);
    manifest = valid_manifest(); manifest["request_url"] = "http://www.dropbox.com/request/id";
    expect(fixture, manifest, SupportBundleIntakeFailure::invalid_request_url);
    manifest = valid_manifest(); manifest["request_url"] = "https://evil.test/request/id";
    expect(fixture, manifest, SupportBundleIntakeFailure::invalid_request_url);
    manifest = valid_manifest(); manifest["request_url"] = "https://www.dropbox.com/request/id?x=1";
    expect(fixture, manifest, SupportBundleIntakeFailure::invalid_request_url);
    manifest = valid_manifest(); manifest["release_url"] = "https://example.com/release";
    expect(fixture, manifest, SupportBundleIntakeFailure::invalid_release_url);
    manifest = valid_manifest(); manifest["bundle_encryption_key_id"] = "unknown";
    expect(fixture, manifest, SupportBundleIntakeFailure::unknown_bundle_key);
    manifest = valid_manifest(); manifest["bundle_encryption_key_id"] = "wsprrypi-bundle-test-01";
    auto malformed_bundle_request = request_for(fixture, manifest.dump());
    malformed_bundle_request.recognized_bundle_key_ids = {"wsprrypi-bundle-test-01"};
    assert(validate_support_bundle_intake(malformed_bundle_request).failure ==
           SupportBundleIntakeFailure::unknown_bundle_key);

    const std::string duplicate = "{\"schema_version\":1,\"schema_version\":1,\"project_id\":\"wsprrypi\"}";
    assert(validate_support_bundle_intake(request_for(fixture, duplicate)).failure ==
           SupportBundleIntakeFailure::invalid_manifest);

    auto nested_duplicate = valid_manifest().dump();
    const auto position = nested_duplicate.find("\"user_message\":null");
    assert(position != std::string::npos);
    nested_duplicate.replace(position, std::string("\"user_message\":null").size(),
                             "\"user_message\":{\"value\":1,\"value\":2}");
    assert(validate_support_bundle_intake(request_for(fixture, nested_duplicate)).failure ==
           SupportBundleIntakeFailure::invalid_manifest);
}

void test_time_and_generation_state() {
    auto fixture = make_fixture();
    const auto bytes = valid_manifest().dump();
    auto request = request_for(fixture, bytes);
    request.now_utc_seconds = 1786902899;
    assert(validate_support_bundle_intake(request).failure == SupportBundleIntakeFailure::outside_validity_window);
    request.now_utc_seconds = 1786902900;
    assert(validate_support_bundle_intake(request).valid());
    request.now_utc_seconds = 1786989900;
    assert(validate_support_bundle_intake(request).valid());
    request.now_utc_seconds = 1786989901;
    assert(validate_support_bundle_intake(request).failure == SupportBundleIntakeFailure::outside_validity_window);

    request = request_for(fixture, bytes);
    request.previous = SupportBundleIntakePreviousState{8, std::string(64, '0')};
    assert(validate_support_bundle_intake(request).failure == SupportBundleIntakeFailure::rollback);
    request.previous = SupportBundleIntakePreviousState{7, std::string(64, '0')};
    assert(validate_support_bundle_intake(request).failure == SupportBundleIntakeFailure::same_generation_mutated);
    const auto accepted = validate_support_bundle_intake(request_for(fixture, bytes));
    request.previous = SupportBundleIntakePreviousState{7, accepted.manifest.manifest_sha256};
    assert(validate_support_bundle_intake(request).valid());
}

void test_bounds_and_failure_non_disclosure() {
    auto fixture = make_fixture();
    auto request = request_for(fixture, valid_manifest().dump());
    request.manifest_bytes.assign(16 * 1024 + 1, 'x');
    auto result = validate_support_bundle_intake(request);
    assert(result.failure == SupportBundleIntakeFailure::manifest_oversized);
    assert(result.manifest.request_url == std::nullopt && result.manifest.release_url.empty());
    request = request_for(fixture, valid_manifest().dump());
    request.signature_envelope_bytes.assign(2049, 'x');
    result = validate_support_bundle_intake(request);
    assert(result.failure == SupportBundleIntakeFailure::signature_envelope_oversized);
    assert(result.manifest.request_url == std::nullopt);
}

} // namespace

int main() {
    test_valid_active_and_disabled();
    test_signature_boundary();
    test_strict_manifest_and_policy();
    test_time_and_generation_state();
    test_bounds_and_failure_non_disclosure();
    std::cout << "support_bundle_intake_validation_test: PASS\n";
}
