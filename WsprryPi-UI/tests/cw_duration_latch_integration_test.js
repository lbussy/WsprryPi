"use strict";

const assert = require("node:assert/strict");
const http = require("node:http");
const net = require("node:net");
const path = require("node:path");
const { spawn } = require("node:child_process");
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
                try {
                    resolve(JSON.parse(body));
                } catch (error) {
                    reject(error);
                }
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

async function waitFor(check, description, timeoutMs = 10000) {
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

    close() {
        this.socket.close();
    }
}

async function browserTest() {
    const fail = (message) => { throw new Error(message); };
    const equal = (actual, expected, message) => {
        if (actual !== expected) {
            fail(`${message}: expected ${JSON.stringify(expected)}, got ${JSON.stringify(actual)}`);
        }
    };
    const ok = (condition, message) => { if (!condition) fail(message); };
    const includes = (actual, expected, message) => {
        ok(String(actual).includes(expected), `${message}: ${JSON.stringify(actual)}`);
    };

    class FakeClock {
        constructor() {
            this.now = 0;
            this.nextId = 1;
            this.tasks = new Map();
        }

        setTimeout(callback, delay = 0) {
            const id = this.nextId++;
            this.tasks.set(id, { callback, due: this.now + Number(delay || 0) });
            return id;
        }

        clearTimeout(id) {
            this.tasks.delete(id);
        }

        tick(milliseconds) {
            const end = this.now + milliseconds;
            while (true) {
                const ready = [...this.tasks.entries()]
                    .filter(([, task]) => task.due <= end)
                    .sort((a, b) => a[1].due - b[1].due || a[0] - b[0])[0];
                if (!ready) break;
                const [id, task] = ready;
                this.tasks.delete(id);
                this.now = task.due;
                task.callback();
            }
            this.now = end;
        }

        reset() {
            this.now = 0;
            this.tasks.clear();
        }
    }

    class FakeDeferred {
        constructor() {
            this.doneCallbacks = [];
            this.failCallbacks = [];
            this.alwaysCallbacks = [];
            this.settled = false;
            this.outcome = "";
            this.args = [];
        }

        done(callback) {
            if (this.outcome === "resolved") callback(...this.args);
            else if (!this.settled) this.doneCallbacks.push(callback);
            return this;
        }
        fail(callback) {
            if (this.outcome === "rejected") callback(...this.args);
            else if (!this.settled) this.failCallbacks.push(callback);
            return this;
        }
        always(callback) {
            if (this.settled) callback(...this.args);
            else this.alwaysCallbacks.push(callback);
            return this;
        }

        resolve() {
            if (this.settled) return;
            this.settled = true;
            this.outcome = "resolved";
            this.doneCallbacks.forEach((callback) => callback());
            this.alwaysCallbacks.forEach((callback) => callback());
        }

        reject(xhr, textStatus = "error") {
            if (this.settled) return;
            this.settled = true;
            this.outcome = "rejected";
            this.args = [xhr, textStatus];
            this.failCallbacks.forEach((callback) => callback(xhr, textStatus));
            this.alwaysCallbacks.forEach((callback) => callback());
        }
    }

    const clock = new FakeClock();
    window.setTimeout = clock.setTimeout.bind(clock);
    window.clearTimeout = clock.clearTimeout.bind(clock);

    const patches = [];
    let patchMode = "success";
    let pendingPatch = null;
    ajaxWithEndpointFallback = (endpoint, options) => {
        const deferred = new FakeDeferred();
        patches.push({ endpoint, options, deferred });
        if (patchMode === "success") deferred.resolve();
        else if (patchMode === "pending") pendingPatch = deferred;
        return deferred;
    };

    const statusTransitions = [];
    const realSetConfigSaveStatus = setConfigSaveStatus;
    setConfigSaveStatus = (state, message = "", detail = "", options = {}) => {
        statusTransitions.push({ state, message, detail });
        return realSetConfigSaveStatus(state, message, detail, options);
    };

    const dialogs = [];
    showMessageDialog = (options) => { dialogs.push(options); };
    showBackendStatus = () => {};
    clearBackendStatus = () => {};
    persistLocalConfigDraftIfPossible = () => {};
    removePersistedConfigDraft = () => {};

    const field = (id) => document.getElementById(id);
    const setValue = (id, value, eventType = "input") => {
        field(id).value = String(value);
        field(id).dispatchEvent(new Event(eventType, { bubbles: true }));
    };
    const selectMode = (mode) => {
        field("qrss_mode").checked = true;
        ["QRSS", "FSKCW", "DFCW"].forEach((value) => {
            field(`mode_${value.toLowerCase()}`).checked = value === mode;
        });
    };

    // Keep this focused on the real CW controls and their validation. Other
    // Setup panels are orthogonal and may be empty when the PHP fixture runs
    // without a controller backend.
    validatePage = () => {
        const mode = selectedConfigMode();
        let valid = validateCwMessage() &&
            validateCwDotSeconds() &&
            validateCwRepeatMinutes() &&
            validateCwStartSecond();
        const activeIds = mode === "DFCW"
            ? ["dfcw_intra_element_gap", "dfcw_inter_character_gap", "dfcw_inter_word_gap"]
            : ["cw_intra_element_gap", "cw_inter_character_gap", "cw_inter_word_gap"];
        activeIds.forEach((id) => {
            valid = validatePositiveCwField(id, "Enter a positive spacing value.") && valid;
        });
        return valid;
    };

    const reset = (mode = "QRSS") => {
        clock.reset();
        patches.length = 0;
        dialogs.length = 0;
        statusTransitions.length = 0;
        patchMode = "success";
        pendingPatch = null;
        configAutosaveTimer = null;
        configSaveStatusClearTimer = null;
        configAutosaveSuspended = false;
        configAutosaveInFlight = false;
        configAutosavePendingAfterFlight = false;
        configAutosaveDirty = false;
        lastSavedConfigPayload = "";
        persistedStationIdentity = null;
        lastFailedConfigPayload = "";
        lastFailedConfigMessage = "";
        cwDurationPolicyLatched = false;
        selectMode(mode);
        field("qrss_message").value = "E";
        field("dot_length").value = "1";
        field("tx_repeat_every").value = "1";
        field("tx_start_second").value = "5";
        field("cw_intra_element_gap").value = "1";
        field("cw_inter_character_gap").value = "3";
        field("cw_inter_word_gap").value = "7";
        field("dfcw_intra_element_gap").value = "0.333333";
        field("dfcw_inter_character_gap").value = "1";
        field("dfcw_inter_word_gap").value = "3";
        field("qrss_message").setCustomValidity("");
        clearFieldValidationState(field("qrss_message"));
        realSetConfigSaveStatus("", "", "");
        statusTransitions.length = 0;
    };

    const assertLatched = (draft) => {
        equal(field("qrss_message").value, draft, "draft must remain unchanged");
        includes(field("qrss_message").validationMessage, "calculated", "custom validity must explain duration");
        includes(field("qrss_message").validationMessage, "repeat interval", "custom validity must name repeat interval");
        ok(field("qrss_message").classList.contains("is-invalid"), "Message must have is-invalid");
        equal(field("qrss_message").getAttribute("aria-invalid"), "true", "Message aria-invalid");
        equal(field("configSaveStatus").textContent, "Save failed", "inline short status");
        includes(field("configSaveStatusDetail").textContent, "calculated", "inline detail must explain duration");
    };

    // 1 and 2: local overlong input and repeated overlong edits.
    reset();
    field("dot_length").value = "20";
    setValue("qrss_message", "EE");
    clock.tick(800);
    assertLatched("EE");
    equal(patches.length, 0, "overlong local edit must not PATCH");
    setValue("qrss_message", "EEE");
    clock.tick(800);
    assertLatched("EEE");
    equal(patches.length, 0, "repeated overlong edit must not PATCH");
    equal(dialogs.length, 0, "repeated overlong edit must not open modal");

    // 3: shortening EE to T produces exactly 60 seconds at a 20-second dot.
    statusTransitions.length = 0;
    setValue("qrss_message", "T");
    clock.tick(800);
    equal(field("qrss_message").validationMessage, "", "equality must clear duration validity");
    ok(!field("qrss_message").classList.contains("is-invalid"), "equality must clear invalid styling");
    equal(field("qrss_message").getAttribute("aria-invalid"), "false", "equality aria-invalid");
    equal(patches.length, 1, "shortening recovery must PATCH once");
    ok(statusTransitions.some(({ state }) => state === "saving"), "recovery must enter Saving");
    ok(statusTransitions.some(({ state }) => state === "saved"), "recovery must enter Saved");

    // 4: increasing repeat interval clears the latch and saves once.
    reset();
    field("dot_length").value = "61";
    setValue("qrss_message", "E");
    assertLatched("E");
    setValue("tx_repeat_every", "2");
    clock.tick(800);
    equal(patches.length, 1, "repeat interval recovery must PATCH once");
    equal(field("qrss_message").validationMessage, "", "repeat recovery validity");
    equal(field("qrss_message").getAttribute("aria-invalid"), "false", "repeat recovery aria-invalid");

    // 5: dot length and only the active spacing group control recovery.
    reset();
    field("dot_length").value = "61";
    setValue("qrss_message", "E");
    setValue("dot_length", "60");
    clock.tick(800);
    equal(patches.length, 1, "dot-length recovery must PATCH once");
    equal(field("qrss_message").validationMessage, "", "dot-length recovery validity");

    reset("QRSS");
    field("dot_length").value = "20";
    setValue("qrss_message", "EE");
    setValue("dfcw_inter_character_gap", "0.1");
    clock.tick(800);
    assertLatched("EE");
    equal(patches.length, 0, "inactive DFCW spacing must not recover QRSS");
    setValue("cw_inter_character_gap", "0.5");
    clock.tick(800);
    equal(patches.length, 1, "active QRSS spacing recovery must PATCH once");
    equal(field("qrss_message").validationMessage, "", "active spacing recovery validity");

    // 6: ordinary validation wins after duration-latch clearing.
    reset();
    field("dot_length").value = "61";
    setValue("qrss_message", "E");
    setValue("qrss_message", "");
    clock.tick(800);
    equal(field("qrss_message").validationMessage, "CW message is required.", "empty validation must survive");
    ok(field("qrss_message").classList.contains("is-invalid"), "empty Message remains invalid");
    equal(patches.length, 0, "empty Message must not PATCH");

    reset();
    field("dot_length").value = "61";
    setValue("qrss_message", "E");
    setValue("qrss_message", "E@");
    clock.tick(800);
    equal(field("qrss_message").validationMessage, "CW message contains unsupported character @.", "unsupported validation must survive");
    ok(field("qrss_message").classList.contains("is-invalid"), "unsupported Message remains invalid");
    equal(patches.length, 0, "unsupported Message must not PATCH");

    // 7: a structured rejection latches inline and completion cannot retry.
    reset();
    patchMode = "pending";
    setValue("qrss_message", "E");
    clock.tick(800);
    equal(patches.length, 1, "valid draft must begin one PATCH");
    field("dot_length").value = "61";
    validateCwMessage();
    pendingPatch.reject({
        status: 400,
        responseJSON: {
            error: "invalid_config",
            message: "Configured QRSS message duration of 1m 01s exceeds repeat_every interval of 1m 00s. Reduce the message length, shorten the unit length, or increase repeat_every.",
            policy: "cw_duration_repeat_interval",
            field: "CW.Message",
            mode: "QRSS",
            message_duration_seconds: 61,
            repeat_interval_seconds: 60,
        },
    });
    assertLatched("E");
    clock.tick(1600);
    equal(patches.length, 1, "structured rejection completion must not retry overlong payload");

    // 8: exercise the real connectWebSocket message routing.
    class FakeWebSocket {
        static CONNECTING = 0;
        static OPEN = 1;
        static CLOSED = 3;
        constructor(url) {
            this.url = url;
            this.readyState = FakeWebSocket.CONNECTING;
            this.listeners = new Map();
            FakeWebSocket.instances.push(this);
        }
        addEventListener(type, callback) {
            if (!this.listeners.has(type)) this.listeners.set(type, []);
            this.listeners.get(type).push(callback);
        }
        emit(type, event) {
            (this.listeners.get(type) || []).forEach((callback) => callback(event));
        }
        close() { this.readyState = FakeWebSocket.CLOSED; }
    }
    FakeWebSocket.instances = [];
    if (ws && typeof ws.close === "function") ws.close();
    ws = null;
    window.WebSocket = FakeWebSocket;
    connectWebSocket({ proxyUrl: "ws://test/socket", directUrl: "ws://test:8081", name: "test" }, 5000);
    const socket = FakeWebSocket.instances[0];
    ok(socket, "fake WebSocket must be created");

    reset();
    field("dot_length").value = "61";
    setValue("qrss_message", "E");
    const durationMessage = "Configured QRSS message duration of 1m 01s exceeds repeat_every interval of 1m 00s. Reduce the message length, shorten the unit length, or increase repeat_every.";
    socket.emit("message", { data: JSON.stringify({
        type: "configuration",
        state: "reload_failed",
        message: durationMessage,
    }) });
    assertLatched("E");
    equal(dialogs.length, 0, "mapped duration reload failure must not open modal");

    field("dot_length").value = "1";
    validateCwMessage();
    socket.emit("message", { data: JSON.stringify({
        type: "configuration",
        state: "reload_failed",
        message: durationMessage,
    }) });
    equal(dialogs.length, 1, "duration message with valid draft must use generic modal");
    equal(dialogs[0].title, "Configuration Reload Failed", "duration fallback modal title");

    socket.emit("message", { data: JSON.stringify({
        type: "configuration",
        state: "reload_failed",
        message: "Unrelated backend failure.",
    }) });
    equal(dialogs.length, 2, "unrelated reload failure must open generic modal");
    equal(dialogs[1].message, "Unrelated backend failure.", "unrelated fallback message");

    // 9: start-second drafts validate locally, preserve zero, and autosave after correction.
    reset("DFCW");
    equal(field("tx_start_second").type, "number", "start second input type");
    equal(field("tx_start_second").min, "0", "start second minimum");
    equal(field("tx_start_second").max, "59", "start second maximum");
    equal(field("tx_start_second").step, "1", "start second step");
    ok(field("tx_start_second").required, "start second must be required");
    equal(
        getConfigIntValue({}, "CW", "Start Second", 5),
        5,
        "missing start second must populate with fallback 5"
    );
    equal(
        getConfigIntValue({ "Start Second": 0 }, "CW", "Start Second", 5),
        0,
        "population helper must preserve explicit zero"
    );
    ["QRSS", "FSKCW", "DFCW"].forEach((mode) => {
        selectMode(mode);
        field("tx_start_second").value = "59";
        ok(validateCwStartSecond(), `${mode} must share valid start-second behavior`);
    });
    const durationBefore = field("cw_message_length_estimate").textContent;
    setValue("tx_start_second", "5.5");
    clock.tick(800);
    equal(patches.length, 0, "fractional start second must block autosave");
    equal(field("tx_start_second").value, "5.5", "invalid start-second draft must remain local");
    equal(field("tx_start_second").getAttribute("aria-invalid"), "true", "invalid start second aria state");
    setValue("tx_start_second", "0");
    clock.tick(800);
    equal(patches.length, 1, "valid start-second correction must resume autosave");
    const savedPayload = JSON.parse(patches[0].options.data);
    equal(savedPayload.CW["Start Second"], 0, "autosave payload must preserve explicit zero");
    equal(field("cw_message_length_estimate").textContent, durationBefore, "start second must not alter duration estimate");
    field("wspr_mode").checked = true;
    field("qrss_mode").checked = false;
    field("tx_start_second").value = "invalid";
    ok(validateCwStartSecond(), "WSPR must ignore CW start-second validation");
    ok(!field("tx_start_second").classList.contains("is-invalid"), "WSPR must clear start-second invalid styling");

    // 10: a new edit immediately revokes stale Saved, then progresses normally.
    reset();
    syncConfigAutosaveBaseline();
    equal(field("configSaveStatus").textContent, "Saved", "baseline must begin saved");
    setValue("qrss_message", "T");
    equal(field("configSaveStatus").textContent, "Changes pending", "edit must immediately revoke Saved");
    equal(field("configSaveStatus").dataset.state, "pending", "pending status state");
    ok(hasUnsavedLocalConfigChanges(), "debounced edit must be unsaved");
    clock.tick(800);
    ok(statusTransitions.some(({ message }) => message === "Saving..."), "request must enter Saving");
    equal(field("configSaveStatus").textContent, "Saved", "successful current payload must show Saved");
    equal(hasUnsavedLocalConfigChanges(), false, "successful current payload must be saved");

    // 11: completion of an older in-flight payload cannot claim the newer edit is saved.
    reset();
    syncConfigAutosaveBaseline();
    patchMode = "pending";
    setValue("qrss_message", "T");
    clock.tick(800);
    equal(patches.length, 1, "first payload must begin one PATCH");
    equal(field("configSaveStatus").textContent, "Saving...", "first payload must show Saving");
    const firstPatch = pendingPatch;
    setValue("qrss_message", "EE");
    equal(field("configSaveStatus").textContent, "Changes pending", "newer edit must replace Saving with pending");
    ok(hasUnsavedLocalConfigChanges(), "newer edit must remain unsaved while first request is in flight");
    clock.tick(800);
    equal(patches.length, 1, "newer payload must queue behind first request");
    statusTransitions.length = 0;
    patchMode = "success";
    firstPatch.resolve();
    equal(field("configSaveStatus").textContent, "Changes pending", "older completion must retain pending status");
    ok(!statusTransitions.some(({ state }) => state === "saved"), "older completion must not enter Saved");
    ok(hasUnsavedLocalConfigChanges(), "older completion must not clear unsaved state");
    clock.tick(800);
    equal(patches.length, 2, "newer payload must subsequently PATCH");
    equal(field("configSaveStatus").textContent, "Saved", "only newest completion may show Saved");
    equal(hasUnsavedLocalConfigChanges(), false, "newest completion must clear unsaved state");

    // 12: invalid edits revoke stale Saved, then retain actionable validation feedback.
    reset();
    syncConfigAutosaveBaseline();
    setValue("qrss_message", "");
    equal(field("configSaveStatus").textContent, "Changes pending", "invalid edit must immediately revoke Saved");
    clock.tick(800);
    equal(field("configSaveStatus").textContent, "Invalid - not saved", "validation must replace pending feedback");
    equal(field("configSaveStatus").dataset.state, "invalid", "invalid status state");
    ok(hasUnsavedLocalConfigChanges(), "invalid edit must remain unsaved");

    // 13: suspended initial population must not display pending feedback.
    reset();
    syncConfigAutosaveBaseline();
    statusTransitions.length = 0;
    suspendConfigAutosave(true);
    setValue("qrss_message", "T");
    equal(field("configSaveStatus").textContent, "Saved", "suspended population must preserve baseline status");
    ok(!statusTransitions.some(({ state }) => state === "pending"), "suspended population must not enter pending");
    suspendConfigAutosave(false);

    // 14: invalid station identity remains local while an unrelated valid
    // setting is persisted with explicit partial-save feedback.
    reset();
    const focusedValidatePage = validatePage;
    validatePage = (options = {}) => options.allowInvalidStationIdentity === true;
    field("wspr_mode").checked = true;
    field("qrss_mode").checked = false;
    field("callsign").value = "NXXX";
    field("gridsquare").value = "ZZ99";
    persistedStationIdentity = { callsign: "NXXX", gridsquare: "ZZ99" };
    lastSavedConfigPayload = JSON.stringify(buildConfigPayload());
    setValue("callsign", "BAD CALL");
    field("use_led").checked = !field("use_led").checked;
    field("use_led").dispatchEvent(new Event("change", { bubbles: true }));
    clock.tick(800);
    equal(patches.length, 1, "unrelated valid change must PATCH despite invalid identity draft");
    const partialSavePayload = JSON.parse(patches[0].options.data);
    equal(partialSavePayload.WSPR["Call Sign"], "NXXX", "PATCH must retain persisted callsign");
    equal(partialSavePayload.WSPR["Grid Square"], "ZZ99", "PATCH must retain persisted locator");
    equal(
        partialSavePayload.Operation["Use LED"],
        field("use_led").checked,
        "PATCH must include the unrelated LED change"
    );
    equal(field("configSaveStatus").textContent, "Other changes saved", "partial-save status");
    includes(
        field("configSaveStatusDetail").textContent,
        "not valid for transmission",
        "partial-save detail must preserve the transmission safety boundary"
    );
    ok(hasUnsavedLocalConfigChanges(), "invalid identity draft must remain visibly unsaved");
    validatePage = focusedValidatePage;

    return {
        scenarios: 14,
        assertions: "passed",
        finalPatchCount: patches.length,
        finalDialogCount: dialogs.length,
    };
}

async function main() {
    const phpPort = await freePort();
    const debugPort = await freePort();
    const php = spawn("php", ["-S", `127.0.0.1:${phpPort}`, "-t", "data"], {
        cwd: UI_ROOT,
        stdio: "ignore",
    });
    const profileDir = `/tmp/wsprrypi-cw-duration-test-${process.pid}`;
    let chromium;

    let client;
    try {
        await waitFor(
            async () => await getStatus(
                `http://127.0.0.1:${phpPort}/index.php?page=config`
            ) === 200,
            "local PHP fixture"
        );
        chromium = spawn("chromium", [
            "--headless",
            "--no-sandbox",
            "--disable-gpu",
            `--remote-debugging-port=${debugPort}`,
            `--user-data-dir=${profileDir}`,
            `http://127.0.0.1:${phpPort}/index.php?page=config`,
        ], { stdio: "ignore" });

        const page = await waitFor(async () => {
            const pages = await getJson(`http://127.0.0.1:${debugPort}/json`);
            return pages.find((item) => item.type === "page");
        }, "Setup page in Chromium");

        client = new CdpClient(page.webSocketDebuggerUrl);
        await client.open();
        await client.send("Page.navigate", {
            url: `http://127.0.0.1:${phpPort}/index.php?page=config`,
        });
        await waitFor(async () => {
            const result = await client.send("Runtime.evaluate", {
                expression: "({ ready: typeof scheduleAutosave === 'function' && document.readyState === 'complete', state: document.readyState, title: document.title, url: location.href, scheduleType: typeof scheduleAutosave, scripts: [...document.scripts].map((script) => script.src) })",
                returnByValue: true,
            });
            if (!result.result.value.ready) {
                throw new Error(JSON.stringify(result.result.value));
            }
            return true;
        }, "Setup production scripts");

        const result = await client.send("Runtime.evaluate", {
            expression: `(${browserTest.toString()})()`,
            awaitPromise: true,
            returnByValue: true,
        });
        if (result.exceptionDetails) {
            const detail = result.exceptionDetails.exception &&
                result.exceptionDetails.exception.description;
            throw new Error(detail || result.exceptionDetails.text || "Browser test failed");
        }
        assert.deepEqual(result.result.value, {
            scenarios: 14,
            assertions: "passed",
            finalPatchCount: 1,
            finalDialogCount: 0,
        });
        console.log("cw_duration_latch_integration_test passed");
    } finally {
        if (client) client.close();
        if (chromium) chromium.kill("SIGTERM");
        php.kill("SIGTERM");
    }
}

main().catch((error) => {
    console.error(error.stack || error.message);
    process.exitCode = 1;
});
