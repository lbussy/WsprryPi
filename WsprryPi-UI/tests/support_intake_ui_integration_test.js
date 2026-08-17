"use strict";

const assert = require("node:assert/strict");
const fs = require("node:fs");
const http = require("node:http");
const net = require("node:net");
const path = require("node:path");
const { spawn, spawnSync } = require("node:child_process");
const WebSocket = require("ws");

const UI_ROOT = path.resolve(__dirname, "..");

function freePort() {
    return new Promise((resolve, reject) => {
        const server = net.createServer();
        server.once("error", reject);
        server.listen(0, "127.0.0.1", () => {
            const { port } = server.address();
            server.close((error) => error ? reject(error) : resolve(port));
        });
    });
}

function getJson(url) {
    return new Promise((resolve, reject) => {
        http.get(url, (response) => {
            let body = "";
            response.setEncoding("utf8");
            response.on("data", (chunk) => { body += chunk; });
            response.on("end", () => {
                try { resolve(JSON.parse(body)); } catch (error) { reject(error); }
            });
        }).on("error", reject);
    });
}

function getStatus(url) {
    return new Promise((resolve, reject) => {
        http.get(url, (response) => {
            response.resume();
            response.on("end", () => resolve(response.statusCode));
        }).on("error", reject);
    });
}

async function waitFor(check, description, timeoutMs = 12000) {
    const deadline = Date.now() + timeoutMs;
    let lastError;
    while (Date.now() < deadline) {
        try {
            const value = await check();
            if (value) return value;
        } catch (error) {
            lastError = error;
        }
        await new Promise((resolve) => setTimeout(resolve, 50));
    }
    throw new Error(`Timed out waiting for ${description}${lastError ? `: ${lastError.message}` : ""}`);
}

async function terminate(child) {
    if (!child || child.exitCode !== null) return;
    child.kill("SIGTERM");
    await new Promise((resolve) => {
        child.once("exit", resolve);
        setTimeout(resolve, 2000);
    });
}

function chromiumBinary() {
    if (process.env.CHROMIUM_BIN) return process.env.CHROMIUM_BIN;
    for (const candidate of ["chromium", "chromium-browser", "google-chrome"]) {
        const found = spawnSync("sh", ["-c", `command -v ${candidate}`], { encoding: "utf8" });
        if (found.status === 0 && found.stdout.trim()) return found.stdout.trim();
    }
    const macChrome = "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome";
    if (fs.existsSync(macChrome)) return macChrome;
    throw new Error("Chromium-compatible browser not found");
}

class CdpClient {
    constructor(url) {
        this.socket = new WebSocket(url);
        this.nextId = 1;
        this.pending = new Map();
        this.socket.on("message", (raw) => {
            const message = JSON.parse(raw);
            if (!message.id || !this.pending.has(message.id)) return;
            const { resolve, reject } = this.pending.get(message.id);
            this.pending.delete(message.id);
            if (message.error) reject(new Error(message.error.message));
            else resolve(message.result);
        });
    }
    async open() {
        if (this.socket.readyState === WebSocket.OPEN) return;
        await new Promise((resolve, reject) => {
            this.socket.once("open", resolve);
            this.socket.once("error", reject);
        });
    }
    send(method, params = {}) {
        const id = this.nextId++;
        return new Promise((resolve, reject) => {
            this.pending.set(id, { resolve, reject });
            this.socket.send(JSON.stringify({ id, method, params }));
        });
    }
    close() { this.socket.close(); }
}

async function browserTest() {
    const fail = (message) => { throw new Error(message); };
    const ok = (condition, message) => { if (!condition) fail(message); };
    const equal = (actual, expected, message) => {
        if (actual !== expected) fail(`${message}: expected ${JSON.stringify(expected)}, got ${JSON.stringify(actual)}`);
    };
    const field = (id) => document.getElementById(id);
    const wait = async (check, message, timeout = 5000) => {
        const deadline = Date.now() + timeout;
        while (Date.now() < deadline) {
            if (check()) return;
            await new Promise((resolve) => setTimeout(resolve, 25));
        }
        fail(`Timed out: ${message}`);
    };
    const response = (body, status = 200) => ({
        ok: status >= 200 && status < 300,
        status,
        json: async () => body,
        blob: async () => new Blob(["readable candidate"]),
        headers: { get: () => 'attachment; filename="wsprrypi-support-test.tar.gz"' },
    });

    const calls = [];
    let intakeResponse = response({
        status: "active",
        generation: 32,
        expires_at: "2026-10-01T00:00:00Z",
        minimum_upload_version: "1.3.0",
        signing_key_id: "wsprrypi-intake-2026-01",
        bundle_key_id: "wsprrypi-bundle-2026-01",
        request_url: "https://www.dropbox.com/request/secret-capability",
        user_message: "Private intake is operating normally."
    });
    window.fetchWithEndpointFallback = async (endpoint, options = {}) => {
        calls.push({ name: endpoint.name, method: options.method || "GET" });
        if (endpoint.name === "support intake") return intakeResponse;
        if (endpoint.name === "support bundles" && options.method === "POST" &&
            endpoint.proxyUrl.endsWith("/api/support-bundles")) {
            return response({ id: "job-32", case_id: "A7K3-M9QF-X2DP" });
        }
        if (endpoint.name === "support bundles" && options.method === "DELETE") return response({});
        if (endpoint.name === "support bundles" && endpoint.proxyUrl.endsWith("/download")) return response({});
        if (endpoint.name === "support bundles" && endpoint.proxyUrl.endsWith("/finalize")) {
            return response({ workflow_state: "finalized" });
        }
        if (endpoint.name === "support bundles" && endpoint.proxyUrl.endsWith("/encrypt")) {
            return response({ workflow_state: "encrypted" });
        }
        if (endpoint.name === "support bundles" && endpoint.proxyUrl.endsWith("/encrypted")) {
            return response({});
        }
        if (endpoint.name === "support bundles" && endpoint.proxyUrl.endsWith("/receipt")) {
            return response({});
        }
        if (endpoint.name === "support bundles") {
            return response({ state: "succeeded", download_available: true, case_id: "A7K3-M9QF-X2DP" });
        }
        return response({}, 500);
    };
    URL.createObjectURL = () => "blob:test";
    URL.revokeObjectURL = () => {};
    HTMLAnchorElement.prototype.click = () => {};

    equal(calls.length, 0, "page load must not resolve intake");
    ok(field("supportIntakePanel").classList.contains("d-none"), "intake action starts hidden");
    field("createSupportBundleButton").click();
    field("supportBundleIssueNumber").value = "414";
    field("confirmCreateSupportBundleButton").click();
    await wait(() => calls.some((call) => call.name === "support bundles" && call.method === "POST"), "create call");
    equal(calls.filter((call) => call.name === "support intake").length, 0,
        "collection must not resolve intake");
    await wait(() => field("downloadSupportBundleButton").dataset.available === "true", "candidate ready", 6000);
    field("downloadSupportBundleButton").click();
    await wait(() => !field("supportBundleReviewConsent").classList.contains("d-none"), "review consent");
    equal(calls.filter((call) => call.name === "support intake").length, 0,
        "download must not resolve intake");
    field("supportBundleReviewed").checked = true;
    field("supportBundleReviewed").dispatchEvent(new Event("change", { bubbles: true }));
    await wait(() => field("finalizeSupportBundleButton").disabled === false, "finalize action enabled");
    field("finalizeSupportBundleButton").click();
    await wait(() => !field("supportIntakePanel").classList.contains("d-none"), "intake action after finalization");
    equal(calls.filter((call) => call.name === "support intake").length, 0,
        "finalization must not resolve intake without explicit check");

    field("checkSupportIntakeButton").click();
    field("checkSupportIntakeButton").click();
    await wait(() => field("supportIntakeMessage").textContent.includes("Private upload is available"), "active state");
    equal(calls.filter((call) => call.name === "support intake").length, 1,
        "duplicate clicks must produce one request");
    ok(!document.body.textContent.includes("secret-capability"), "Dropbox capability must not enter rendered text");
    ok(![...document.querySelectorAll("a")].some((link) => link.href.includes("secret-capability")),
        "Dropbox capability must not enter a link");
    ok(field("supportIntakeSignedMessage").textContent.includes("operating normally"),
        "active signed message must render as text");
    ok(!field("supportEncryptionPanel").classList.contains("d-none"),
        "active intake must reveal local encryption consent");
    field("supportEncryptionConsent").checked = true;
    field("supportEncryptionConsent").dispatchEvent(new Event("change", { bubbles: true }));
    field("encryptSupportBundleButton").click();
    await wait(() => !field("downloadEncryptedSupportBundleButton").classList.contains("d-none"),
        "encrypted download action");
    field("downloadEncryptedSupportBundleButton").click();
    await wait(() => !field("downloadSupportReceiptButton").classList.contains("d-none"),
        "receipt after encrypted download");
    ok(!field("supportDropboxHandoffPanel").classList.contains("d-none"),
        "completed encrypted download reveals Dropbox disclosure");
    equal(field("openSupportDropboxButton").getAttribute("href"),
        "/api/support-bundles/job-32/handoff",
        "handoff link exposes only the local fresh resolver");
    ok(field("openSupportDropboxButton").classList.contains("disabled"),
        "handoff starts disabled without consent");
    const blockedHandoff = new MouseEvent("click", { bubbles: true, cancelable: true });
    equal(field("openSupportDropboxButton").dispatchEvent(blockedHandoff), false,
        "handoff click is blocked before consent");
    field("supportDropboxHandoffConsent").checked = true;
    field("supportDropboxHandoffConsent").dispatchEvent(new Event("change", { bubbles: true }));
    ok(!field("openSupportDropboxButton").classList.contains("disabled"),
        "explicit disclosure consent enables handoff");
    field("downloadSupportReceiptButton").click();
    await wait(() => field("supportEncryptionMessage").textContent.includes("Receipt downloaded"),
        "receipt download state");

    intakeResponse = response({
        status: "disabled",
        generation: 33,
        expires_at: "2026-10-02T00:00:00Z",
        signing_key_id: "wsprrypi-intake-2026-01",
        bundle_key_id: "wsprrypi-bundle-2026-01",
        user_message: "Maintenance window."
    });
    field("checkSupportIntakeButton").click();
    await wait(() => field("supportIntakeMessage").textContent.includes("temporarily disabled"), "disabled state");
    ok(field("supportEncryptionPanel").classList.contains("d-none"),
        "non-active intake must revoke encryption authorization");
    ok(field("supportDropboxHandoffPanel").classList.contains("d-none") &&
        !field("supportDropboxHandoffConsent").checked,
        "non-active intake must revoke handoff authorization");
    ok(!field("openSupportDropboxButton").hasAttribute("href"),
        "non-active intake must remove the local handoff target");

    intakeResponse = response({
        status: "upgrade_required",
        minimum_upload_version: "1.4.0",
        release_url: "https://github.com/WsprryPi/WsprryPi/releases/latest",
        user_message: "A newer upload protocol is required."
    });
    field("checkSupportIntakeButton").click();
    await wait(() => field("supportIntakeMessage").textContent.includes("1.4.0"), "upgrade state");
    equal(field("supportIntakeUpgradeLink").getAttribute("href"),
        "https://github.com/WsprryPi/WsprryPi/releases/latest", "authenticated release link");

    intakeResponse = response({ status: "active", request_url: "https://evil.invalid/request/x" });
    field("checkSupportIntakeButton").click();
    await wait(() => field("checkSupportIntakeButton").textContent === "Try again", "malformed fallback");
    ok(field("supportIntakeUpgradeLink").classList.contains("d-none"), "failure clears prior release link");
    ok(field("supportIntakeSignedMessage").classList.contains("d-none"), "failure clears prior signed message");

    intakeResponse = response({
        status: "upgrade_required",
        minimum_upload_version: "01.4.0",
        release_url: "https://github.com/WsprryPi/WsprryPi/releases.evil/latest",
        unexpected: "must fail closed"
    });
    field("checkSupportIntakeButton").click();
    await wait(() => field("checkSupportIntakeButton").textContent === "Try again", "unsafe upgrade fallback");
    ok(!field("supportIntakeUpgradeLink").hasAttribute("href"), "unsafe release URL must be erased");

    intakeResponse = response({ status: "unavailable" }, 503);
    field("checkSupportIntakeButton").click();
    await wait(() => field("supportIntakeMessage").textContent.includes("could not be checked"), "503 unavailable state");
    field("deleteSupportBundleButton").click();
    await wait(() => field("supportIntakePanel").classList.contains("d-none"), "delete reset");
    equal(field("checkSupportIntakeButton").textContent, "Check private upload availability",
        "reset restores initial action");

    return { scenarios: 14, intakeCalls: 6, assertions: "passed" };
}

async function capture(client, outputPath, width, height, state) {
    await client.send("Emulation.setDeviceMetricsOverride", {
        width, height, deviceScaleFactor: 1, mobile: width < 600,
    });
    await client.send("Runtime.evaluate", {
        expression: `(() => {
            document.getElementById("supportBundleSetup").classList.add("d-none");
            document.getElementById("supportBundleReview").classList.remove("d-none");
            document.getElementById("supportBundleCaseId").textContent = "A7K3-M9QF-X2DP";
            document.getElementById("supportIntakePanel").classList.remove("d-none");
            document.getElementById("createSupportBundleButton").classList.add("d-none");
            document.getElementById("supportBundleStatus").dataset.state = "finalized";
            document.getElementById("supportBundleStatus").textContent = "Reviewed candidate finalized. No file has been uploaded.";
            const state = ${JSON.stringify(state)};
            const message = document.getElementById("supportIntakeMessage");
            const button = document.getElementById("checkSupportIntakeButton");
            const link = document.getElementById("supportIntakeUpgradeLink");
            button.disabled = false;
            link.classList.add("d-none");
            if (state === "active") {
                message.textContent = "Private upload is available until Oct 1, 2026, 12:00 AM UTC. No file has been uploaded.";
                button.textContent = "Check again";
                document.getElementById("supportEncryptionPanel").classList.remove("d-none");
                document.getElementById("supportEncryptionConsent").checked = true;
                document.getElementById("supportEncryptionConsent").disabled = false;
                document.getElementById("encryptSupportBundleButton").classList.remove("d-none");
                document.getElementById("encryptSupportBundleButton").disabled = false;
                document.getElementById("supportDropboxHandoffPanel").classList.remove("d-none");
                document.getElementById("supportDropboxHandoffConsent").checked = true;
                const handoff = document.getElementById("openSupportDropboxButton");
                handoff.classList.remove("disabled");
                handoff.setAttribute("aria-disabled", "false");
                handoff.tabIndex = 0;
            } else if (state === "upgrade") {
                document.getElementById("supportEncryptionPanel").classList.add("d-none");
                message.textContent = "Upgrade to WsprryPi 1.4.0 or later before uploading. Your local bundle is unchanged.";
                button.textContent = "Check again";
                link.href = "https://github.com/WsprryPi/WsprryPi/releases/latest";
                link.classList.remove("d-none");
            } else if (state === "loading") {
                document.getElementById("supportBundleReview").classList.add("d-none");
                message.textContent = "Checking private upload availability…";
                button.textContent = "Check private upload availability";
                button.disabled = true;
            } else {
                message.textContent = "Private upload availability could not be checked. Your local bundle is unchanged. Try again.";
                button.textContent = "Try again";
            }
            document.getElementById(state === "active" ? "supportDropboxHandoffPanel" :
                "supportIntakePanel").scrollIntoView({ block: "center" });
        })()`,
    });
    await new Promise((resolve) => setTimeout(resolve, 250));
    const screenshot = await client.send("Page.captureScreenshot", { format: "png", captureBeyondViewport: false });
    fs.writeFileSync(outputPath, screenshot.data, "base64");
}

async function main() {
    const phpPort = await freePort();
    const debugPort = await freePort();
    const php = spawn("php", ["-S", `127.0.0.1:${phpPort}`, "-t", "data"], {
        cwd: UI_ROOT, stdio: "ignore",
    });
    const profileDir = `/tmp/wsprrypi-support-intake-ui-${process.pid}`;
    let chromium;
    let client;
    try {
        await waitFor(async () => await getStatus(
            `http://127.0.0.1:${phpPort}/index.php?page=maintenance`) === 200, "Maintenance fixture");
        chromium = spawn(chromiumBinary(), [
            "--headless", "--no-sandbox", "--disable-gpu",
            `--remote-debugging-port=${debugPort}`,
            `--user-data-dir=${profileDir}`,
            `http://127.0.0.1:${phpPort}/index.php?page=maintenance`,
        ], { stdio: "ignore" });
        const page = await waitFor(async () => {
            const pages = await getJson(`http://127.0.0.1:${debugPort}/json`);
            return pages.find((item) => item.type === "page");
        }, "Maintenance page in Chromium");
        client = new CdpClient(page.webSocketDebuggerUrl);
        await client.open();
        await waitFor(async () => {
            const result = await client.send("Runtime.evaluate", {
                expression: "document.readyState === 'complete' && typeof SUPPORT_INTAKE_ENDPOINT === 'object'",
                returnByValue: true,
            });
            return result.result.value === true;
        }, "Maintenance scripts");
        const result = await client.send("Runtime.evaluate", {
            expression: `(${browserTest.toString()})()`, awaitPromise: true, returnByValue: true,
        });
        if (result.exceptionDetails) {
            const detail = result.exceptionDetails.exception && result.exceptionDetails.exception.description;
            throw new Error(detail || result.exceptionDetails.text || "Browser test failed");
        }
        assert.deepEqual(result.result.value, { scenarios: 14, intakeCalls: 6, assertions: "passed" });
        if (process.env.WSPRRYPI_SUPPORT_INTAKE_SCREENSHOT_DIR) {
            fs.mkdirSync(process.env.WSPRRYPI_SUPPORT_INTAKE_SCREENSHOT_DIR, { recursive: true });
            await capture(client, path.join(process.env.WSPRRYPI_SUPPORT_INTAKE_SCREENSHOT_DIR,
                "support-intake-active-desktop.png"), 1440, 1100, "active");
            await capture(client, path.join(process.env.WSPRRYPI_SUPPORT_INTAKE_SCREENSHOT_DIR,
                "support-intake-handoff-mobile.png"), 390, 844, "active");
            await capture(client, path.join(process.env.WSPRRYPI_SUPPORT_INTAKE_SCREENSHOT_DIR,
                "support-intake-upgrade-desktop.png"), 1440, 1100, "upgrade");
            await capture(client, path.join(process.env.WSPRRYPI_SUPPORT_INTAKE_SCREENSHOT_DIR,
                "support-intake-loading-mobile.png"), 390, 844, "loading");
            await capture(client, path.join(process.env.WSPRRYPI_SUPPORT_INTAKE_SCREENSHOT_DIR,
                "support-intake-unavailable-mobile.png"), 390, 844, "unavailable");
        }
        console.log("support_intake_ui_integration_test passed");
    } finally {
        if (client) client.close();
        await terminate(chromium);
        await terminate(php);
        fs.rmSync(profileDir, { recursive: true, force: true, maxRetries: 5, retryDelay: 100 });
    }
}

main().catch((error) => {
    console.error(error.stack || error.message);
    process.exitCode = 1;
});
