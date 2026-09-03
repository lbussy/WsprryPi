"use strict";

const assert = require("node:assert/strict");
const fs = require("node:fs");
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

function waitForExit(child, timeoutMs) {
    if (child.exitCode !== null) return Promise.resolve(true);
    return new Promise((resolve) => {
        let timer;
        const finish = (exited) => {
            clearTimeout(timer);
            child.off("exit", onExit);
            resolve(exited);
        };
        const onExit = () => finish(true);
        child.once("exit", onExit);
        timer = setTimeout(() => finish(child.exitCode !== null), timeoutMs);
    });
}

async function terminate(child) {
    if (!child || child.exitCode !== null) return;
    child.kill("SIGTERM");
    if (await waitForExit(child, 2000)) return;
    child.kill("SIGKILL");
    if (!await waitForExit(child, 2000)) {
        throw new Error(`Child process ${child.pid} did not terminate`);
    }
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

async function captureConflictScreenshot(client, outputPath, tabId, selector) {
    await client.send("Runtime.evaluate", {
        expression: `(() => {
            document.getElementById("transmit_backend").value = "gpio";
            document.getElementById("tx_pin").value = "4";
            setLEDPin(4);
            document.getElementById("use_led").checked = true;
            refreshGpioConflictOptions();
            validateGpioConflictFields();
            const tab = document.getElementById(${JSON.stringify(tabId)});
            document.querySelectorAll("#configTabs .nav-link").forEach((item) => {
                item.classList.toggle("active", item === tab);
                item.setAttribute("aria-selected", item === tab ? "true" : "false");
            });
            document.querySelectorAll("#configTabsContent > .tab-pane").forEach((pane) => {
                const selected = "#" + pane.id === tab.getAttribute("data-bs-target");
                pane.classList.toggle("active", selected);
                pane.classList.toggle("show", selected);
            });
            document.querySelectorAll(".toast.show").forEach((toast) => toast.classList.remove("show"));
        })()`,
    });
    await new Promise((resolve) => setTimeout(resolve, 350));
    await client.send("Runtime.evaluate", { expression: "window.scrollTo(0, 0)" });
    const screenshot = await client.send("Page.captureScreenshot", {
        format: "png",
        captureBeyondViewport: true,
    });
    fs.writeFileSync(outputPath, screenshot.data, "base64");
}

async function captureRp1DriveScreenshot(client, outputPath, theme) {
    await client.send("Runtime.evaluate", {
        expression: `(() => {
            document.documentElement.setAttribute("data-bs-theme", ${JSON.stringify(theme)});
            window.WSPRRYPI_PLATFORM = {
                ...(window.WSPRRYPI_PLATFORM || {}),
                raspberryPiGeneration: 5,
                model: "Raspberry Pi 5 Model B Rev 1.0",
                gpioClockTransmissionSupported: true,
                rp1GpioOperatorVisible: true,
            };
            document.getElementById("transmit_backend").value = "gpio";
            document.getElementById("rp1_gpio_drive_ma").value = "2";
            document.getElementById("use_led").checked = false;
            document.getElementById("use_shutdown").checked = false;
            document.getElementById("use_amp").checked = false;
            document.querySelectorAll(".band-gpio-enabled").forEach((field) => {
                field.checked = false;
            });
            clickTransmitBackend();
            refreshGpioConflictOptions();
            validateGpioConflictFields();
            const tab = document.getElementById("transmitter-hardware-tab");
            document.querySelectorAll("#configTabs .nav-link").forEach((item) => {
                item.classList.toggle("active", item === tab);
                item.setAttribute("aria-selected", item === tab ? "true" : "false");
            });
            document.querySelectorAll("#configTabsContent > .tab-pane").forEach((pane) => {
                const selected = "#" + pane.id === tab.getAttribute("data-bs-target");
                pane.classList.toggle("active", selected);
                pane.classList.toggle("show", selected);
            });
            document.querySelectorAll(".toast.show").forEach((toast) => toast.classList.remove("show"));
            const target = document.getElementById("rp1-gpio-drive-group");
            target.scrollIntoView({ block: "center" });
            document.scrollingElement.scrollTop = Math.max(0, target.offsetTop - 300);
        })()`,
    });
    await new Promise((resolve) => setTimeout(resolve, 350));
    const screenshot = await client.send("Page.captureScreenshot", {
        format: "png",
        captureBeyondViewport: false,
    });
    fs.writeFileSync(outputPath, screenshot.data, "base64");
}

async function captureRouteRequiredRp1Screenshot(client, outputPath, theme) {
    await client.send("Runtime.evaluate", {
        expression: `(() => {
            document.documentElement.setAttribute("data-bs-theme", ${JSON.stringify(theme)});
            window.WSPRRYPI_PLATFORM = {
                ...(window.WSPRRYPI_PLATFORM || {}),
                raspberryPiGeneration: 5,
                gpioClockTransmissionSupported: false,
                gpioClockTransmissionError: "The canonical RP1 GPCLK provider is unavailable. Review the RP1 clock route status below; transmission remains disabled.",
                rp1GpioOperatorVisible: true,
                si5351Detected: true,
            };
            document.getElementById("transmit_backend").value = "gpio";
            document.getElementById("rp1-route-panel").hidden = true;
            document.getElementById("rp1-route-apply").hidden = false;
            document.getElementById("rp1-route-state").hidden = false;
            document.querySelector('#tx_pin option[value=""]').hidden = false;
            document.querySelector('#tx_pin option[value=""]').disabled = false;
            updateBackendPlatformSupportUi();
            clickTransmitBackend();
            rp1RouteUi.render({
                profile: "runtime",
                ok: true,
                state: "runtime_inhibited",
                requested: null,
                persisted: null,
                configured: null,
                active: null,
                moduleRoute: null,
                reconciled: false,
                bootOwnership: "runtime controller",
                journal: "none",
                services: { "wsprrypi.service": "restored" },
                endpointOwned: false,
                endpointOpen: false,
                outputInhibited: "Disabled",
                operationalReady: "Ready",
                developmentPolicy: "Disabled",
                compatible: true,
                generation: 0,
            });
            const tab = document.getElementById("transmitter-hardware-tab");
            document.querySelectorAll("#configTabs .nav-link").forEach((item) => {
                item.classList.toggle("active", item === tab);
                item.setAttribute("aria-selected", item === tab ? "true" : "false");
            });
            document.querySelectorAll("#configTabsContent > .tab-pane").forEach((pane) => {
                const selected = "#" + pane.id === tab.getAttribute("data-bs-target");
                pane.classList.toggle("active", selected);
                pane.classList.toggle("show", selected);
            });
            document.documentElement.style.scrollBehavior = "auto";
            const outputPanel = document.getElementById("transmit_backend").closest("fieldset");
            window.scrollTo(0, Math.max(0, outputPanel.offsetTop - 100));
        })()`,
    });
    await new Promise((resolve) => setTimeout(resolve, 350));
    const screenshot = await client.send("Page.captureScreenshot", {
        format: "png",
        captureBeyondViewport: false,
    });
    fs.writeFileSync(outputPath, screenshot.data, "base64");
}

async function captureRouteProgressModalScreenshot(client, outputPath, theme) {
    await client.send("Runtime.evaluate", {
        expression: `(() => {
            document.documentElement.setAttribute("data-bs-theme", ${JSON.stringify(theme)});
            window.WSPRRYPI_PLATFORM = {
                ...(window.WSPRRYPI_PLATFORM || {}),
                raspberryPiGeneration: 5,
                gpioClockTransmissionSupported: false,
                rp1GpioOperatorVisible: true,
            };
            document.getElementById("rp1-route-apply").hidden = false;
            document.getElementById("rp1-route-state").hidden = false;
            rp1RouteUi.beginProgress("switch", "GPIO20");
            rp1RouteUi.renderProgress("runtime_unknown", "Wsprry Pi is restarting in idle mode. The route change is still being checked; no further action is needed.");
            document.getElementById("rp1-route-progress-retry").textContent = "Checking again in 5 s";
        })()`,
    });
    await new Promise((resolve) => setTimeout(resolve, 350));
    const screenshot = await client.send("Page.captureScreenshot", {
        format: "png",
        captureBeyondViewport: false,
    });
    fs.writeFileSync(outputPath, screenshot.data, "base64");
    await client.send("Runtime.evaluate", {
        expression: `bootstrap.Modal.getOrCreateInstance(document.getElementById("rp1-route-progress-modal")).hide()`,
    });
    await new Promise((resolve) => setTimeout(resolve, 350));
}

async function captureBandPreferencesScreenshot(client, outputPath) {
    await client.send("Runtime.evaluate", {
        expression: `(() => {
            document.documentElement.setAttribute("data-bs-theme", "light");
            applyConfigModeSelection("WSPR");
            const tab = document.getElementById("radio-tab");
            document.querySelectorAll("#configTabs .nav-link").forEach((item) => {
                item.classList.toggle("active", item === tab);
                item.setAttribute("aria-selected", item === tab ? "true" : "false");
            });
            document.querySelectorAll("#configTabsContent > .tab-pane").forEach((pane) => {
                const selected = "#" + pane.id === tab.getAttribute("data-bs-target");
                pane.classList.toggle("active", selected);
                pane.classList.toggle("show", selected);
            });
            const details = document.getElementById("band-preferences");
            details.open = true;
            details.scrollIntoView({ block: "start" });
            document.scrollingElement.scrollTop = Math.max(
                0,
                window.scrollY + details.getBoundingClientRect().top - 120
            );
            document.querySelectorAll(".toast.show").forEach((toast) => toast.classList.remove("show"));
        })()`,
    });
    await new Promise((resolve) => setTimeout(resolve, 250));
    const screenshot = await client.send("Page.captureScreenshot", {
        format: "png",
        captureBeyondViewport: false,
    });
    fs.writeFileSync(outputPath, screenshot.data, "base64");
}

async function browserTest() {
    const fail = (message) => { throw new Error(message); };
    const equal = (actual, expected, message) => {
        if (actual !== expected) fail(`${message}: expected ${JSON.stringify(expected)}, got ${JSON.stringify(actual)}`);
    };
    const ok = (condition, message) => { if (!condition) fail(message); };

    class FakeClock {
        constructor() { this.now = 0; this.nextId = 1; this.tasks = new Map(); }
        setTimeout(callback, delay = 0) {
            const id = this.nextId++;
            this.tasks.set(id, { callback, due: this.now + Number(delay || 0) });
            return id;
        }
        clearTimeout(id) { this.tasks.delete(id); }
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
        reset() { this.now = 0; this.tasks.clear(); }
    }

    class ResolvedDeferred {
        done(callback) { callback(); return this; }
        fail() { return this; }
        always(callback) { callback(); return this; }
    }

    const clock = new FakeClock();
    window.setTimeout = clock.setTimeout.bind(clock);
    window.clearTimeout = clock.clearTimeout.bind(clock);
    const patches = [];
    ajaxWithEndpointFallback = (endpoint, options) => {
        patches.push({ endpoint, options });
        return new ResolvedDeferred();
    };
    showBackendStatus = () => {};
    clearBackendStatus = () => {};
    persistLocalConfigDraftIfPossible = () => {};
    removePersistedConfigDraft = () => {};

    const field = (id) => document.getElementById(id);
    const roles = ["band", "led", "shutdown", "amp"];
    const bandRow = () => document.querySelector('#bandGpioTable tr[data-band="20m"]');

    const reset = (backend, txPin, transmit) => {
        clock.reset();
        patches.length = 0;
        configAutosaveTimer = null;
        configSaveStatusClearTimer = null;
        configAutosaveSuspended = false;
        configAutosaveInFlight = false;
        configAutosavePendingAfterFlight = false;
        configAutosaveDirty = false;
        lastSavedConfigPayload = "";
        lastFailedConfigPayload = "";
        lastFailedConfigMessage = "";
        field("transmit_backend").value = backend;
        field("tx_pin").value = String(txPin);
        field("tx_pin").disabled = false;
        window.conditionalGpioTestTransmitState = transmit;
        field("use_led").checked = false;
        field("ledDropdownButton").disabled = false;
        setLEDPin(18);
        field("use_shutdown").checked = false;
        field("shutdownDropdownButton").disabled = false;
        setShutdownPin(19);
        setUseAmp(false);
        setAmpPin(-1);
        populateBandGpioForm({});
        refreshGpioConflictOptions();
        validateGpioConflictFields();
    };

    const assignRole = (role, pin, enabled) => {
        if (role === "band") {
            const row = bandRow();
            const option = row.querySelector(`.band-gpio-input option[value="${pin}"]`);
            option.disabled = false;
            option.hidden = false;
            row.querySelector(".band-gpio-input").value = String(pin);
            row.querySelector(".band-gpio-enabled").checked = enabled;
            row.querySelector(".band-gpio-active-high").checked = true;
            setBandGpioRowState($(row), enabled);
        } else if (role === "led") {
            setLEDPin(pin);
            field("use_led").checked = enabled;
        } else if (role === "shutdown") {
            setShutdownPin(pin);
            field("use_shutdown").checked = enabled;
        } else {
            setAmpPin(pin);
            setUseAmp(enabled);
            field("ampDropdownButton").disabled = !enabled;
        }
        refreshGpioConflictOptions();
    };

    const roleField = (role) => {
        if (role === "band") return bandRow().querySelector(".band-gpio-input");
        if (role === "led") return field("ledDropdownButton");
        if (role === "shutdown") return field("shutdownDropdownButton");
        return field("ampDropdownButton");
    };
    const validationMessage = (control) =>
        control.dataset.validationMessage || control.validationMessage;

    const ordinaryOption = (role, pin) => {
        if (role === "band") {
            return bandRow().querySelector(`.band-gpio-input option[value="${pin}"]`);
        }
        const buttonId = role === "led"
            ? "ledDropdownButton"
            : role === "shutdown" ? "shutdownDropdownButton" : "ampDropdownButton";
        return document.querySelector(`[aria-labelledby="${buttonId}"] [data-val="GPIO${pin}"]`);
    };

    validatePage = () => validateGpioConflictFields();

    for (const band of ["1.25m", "70cm"]) {
        ok(document.querySelector(`#bandGpioTable tr[data-band="${band}"]`),
            `${band}: Band GPIO configuration row must render`);
        ok(Object.hasOwn(collectBandGpioConfig(), band),
            `${band}: Band GPIO configuration must serialize`);
    }
    ok(validateWsprFrequencyBaseToken("223.5MHz"),
        "numeric 1.25 m WSPR input must remain valid");
    ok(validateWsprFrequencyBaseToken("435000000"),
        "numeric 70 cm WSPR input must remain valid");
    ok(validateWsprFrequencyBaseToken("1.25m") &&
        validateWsprFrequencyBaseToken("70cm"),
        "authoritative 1.25 m and 70 cm WSPR aliases must validate");
    ok(validateWsprFrequencyBaseToken("60m:legacy") &&
        validateWsprFrequencyBaseToken("60M:WRC15"),
        "qualified 60 m WSPR preset identities must validate case-insensitively");
    equal(field("frequency_profile").value, "existing_common",
        "frequency profile must default to Existing/Common");
    field("frequency_profile").value = "wrc15";
    equal(buildConfigPayload().WSPR["Frequency Profile"], "wrc15",
        "frequency profile selection must serialize through the Setup payload");
    field("frequency_profile").value = "existing_common";
    const preferenceBands = [
        "2200m", "630m", "160m", "80m", "60m", "40m", "30m", "20m", "17m",
        "15m", "12m", "10m", "6m", "4m", "2m", "1.25m", "70cm"
    ];
    const preferenceDials = [
        136000, 474200, 1836600, 3568600, 5287200, 7038600, 10138700,
        14095600, 18104600, 21094600, 24924600, 28124600, 50293000, 70091000,
        144489000, 222100000, 432300000
    ];
    updateBandPreferenceCatalog({
        audioOffsetHz: 1500,
        bands: preferenceBands.map((band, index) => ({
            band,
            dial_frequency_hz: preferenceDials[index],
            tone_frequency_hz: preferenceDials[index] + 1500
        })),
        presets: preferenceBands.map((band, index) => ({
            preset: band, band, dial_frequency_hz: preferenceDials[index], existing_common: true
        })).concat([
            { preset: "60m:legacy", band: "60m", dial_frequency_hz: 5287200, existing_common: true },
            { preset: "60m:wrc15", band: "60m", dial_frequency_hz: 5364700, existing_common: false }
        ])
    });
    const sixtyRow = document.querySelector('#band-preferences-body tr[data-band="60m"]');
    equal(sixtyRow.querySelector(".band-preference-mode").value, "default",
        "60 m preference must default to following the selected profile");
    sixtyRow.querySelector(".band-preference-mode").value = "preset";
    sixtyRow.querySelector(".band-preference-preset").value = "60m:wrc15";
    handleBandPreferenceInput({ target: sixtyRow.querySelector(".band-preference-preset") });
    equal(buildConfigPayload().WSPR["Band Preferences"]["60m"], "60m:wrc15",
        "60 m preference must serialize through the Setup payload");
    sixtyRow.querySelector(".band-preference-clear").click();
    ok(!Object.hasOwn(buildConfigPayload().WSPR["Band Preferences"], "60m"),
        "following the profile must remove the local 60 m override");
    const eightRow = document.querySelector('#band-preferences-body tr[data-band="8m"]');
    eightRow.querySelector(".band-preference-mode").value = "custom";
    eightRow.querySelector(".band-preference-custom").value = "40680000";
    handleBandPreferenceInput({ target: eightRow.querySelector(".band-preference-custom") });
    equal(buildConfigPayload().WSPR["Band Preferences"]["8m"], 40680000,
        "custom numeric band preference must serialize as a number");

    let matrixCases = 0;
    for (const transmit of [false, true]) {
        for (const txPin of [4, 20]) {
            const availablePin = txPin === 4 ? 20 : 4;
            for (const role of roles) {
                reset("gpio", txPin, transmit);
                ok(ordinaryOption(role, txPin).disabled, `${role}: selected RF pin must be unavailable for new selection`);
                ok(!ordinaryOption(role, availablePin).disabled, `${role}: other GPCLK0 pin must remain available`);

                assignRole(role, txPin, true);
                const message = `GPIO${txPin} is reserved by GPIO RF Output.`;
                ok(!validateGpioConflictFields(), `${role}: retained RF conflict must be invalid`);
                equal(
                    validationMessage(roleField(role)),
                    message,
                    `${role}: exact ordinary-field message (led=${getLEDPin()}, useLed=${field("use_led").checked}, rf=${getReservedGpioRfOutputPin()}, disabled=${roleField(role).disabled})`
                );
                equal(roleField(role).getAttribute("aria-invalid"), "true", `${role}: ordinary aria-invalid`);
                equal(field("tx_pin").validationMessage, message, `${role}: exact transmit-pin message`);
                equal(field("tx_pin").getAttribute("aria-invalid"), "true", `${role}: transmit aria-invalid`);
                equal(field("tx-pin-error").textContent, message, `${role}: visible transmit-pin error`);
                ok(!field("tx-pin-error").hidden, `${role}: transmit-pin error must be visible`);
                const roleError = role === "band"
                    ? field("band-gpio-gpio-20m-error")
                    : field(`${role}-pin-error`);
                equal(roleError.textContent, message, `${role}: visible ordinary-field error`);
                ok(!roleError.hidden, `${role}: ordinary-field error must be visible`);
                ok(!ordinaryOption(role, txPin).hidden, `${role}: retained invalid option must remain visible`);

                assignRole(role, txPin, false);
                ok(validateGpioConflictFields(), `${role}: disabled retained ordinary role must recover`);
                matrixCases++;
            }
        }
    }

    for (const retainedPin of [4, 20]) {
        for (const ordinaryPin of [4, 20]) {
            for (const role of roles) {
                reset("si5351", retainedPin, false);
                ok(!ordinaryOption(role, ordinaryPin).disabled, `${role}: Si5351 must leave GPIO${ordinaryPin} available`);
                assignRole(role, ordinaryPin, true);
                ok(validateGpioConflictFields(), `${role}: Si5351 retained TX pin must not conflict`);
                matrixCases++;
            }
        }
    }

    reset("gpio", 20, false);
    assignRole("led", 4, true);
    refreshTransmitGpioOptions();
    ok(field("tx_pin").querySelector('option[value="4"]').disabled,
        "ordinary assignment first must make that RF transmit choice unavailable");
    field("tx_pin").querySelector('option[value="4"]').disabled = false;
    field("tx_pin").value = "4";
    refreshGpioConflictOptions();
    ok(!validateGpioConflictFields(), "programmatically retained reverse-direction conflict must remain visible and invalid");

    scheduleAutosave();
    clock.tick(800);
    equal(patches.length, 0, "invalid GPIO ownership must block autosave");
    equal(field("configSaveStatus").textContent, "Invalid - not saved", "invalid ownership status");
    field("transmit_backend").value = "si5351";
    clickTransmitBackend();
    clock.tick(800);
    equal(patches.length, 1, "changing to Si5351 must recover and resume autosave");
    ok(field("tx_pin").getAttribute("aria-invalid") !== "true", "recovery must clear transmit aria-invalid");
    equal(field("ledDropdownButton").getAttribute("aria-invalid"), "false", "recovery must clear ordinary aria-invalid");

    reset("gpio", 4, false);
    const sharedRows = ["20m", "40m"].map((band) =>
        document.querySelector(`#bandGpioTable tr[data-band="${band}"]`));
    sharedRows.forEach((row) => {
        row.querySelector(".band-gpio-input").value = "20";
        row.querySelector(".band-gpio-enabled").checked = true;
        row.querySelector(".band-gpio-active-high").checked = true;
        setBandGpioRowState($(row), true);
    });
    ok(validateGpioConflictFields(), "same-pin Band GPIO sharing with matching polarity must remain valid");
    sharedRows[1].querySelector(".band-gpio-active-high").checked = false;
    ok(!validateGpioConflictFields(), "same-pin Band GPIO sharing with conflicting polarity must remain invalid");

    // Both backends expose frequency calibration in the same Transmitter-tab
    // position while retaining independent backend-specific values.
    window.WSPRRYPI_PLATFORM = {
        ...(window.WSPRRYPI_PLATFORM || {}),
        raspberryPiGeneration: 5,
        gpioClockTransmissionSupported: true,
        rp1GpioOperatorVisible: true,
        si5351Detected: true,
    };
    field("use_system_clock_frequency_estimate").checked = true;
    field("gpio_frequency_residual_ppm").value = "-0.125";
    field("gpio_manual_ppm").value = "1.75";
    field("ppm").value = "2.409358";
    field("transmit_backend").value = "gpio";
    clickTransmitBackend();
    equal(field("rp1-gpio-drive-group").hidden, false,
        "Pi 5 GPIO must show the RP1 drive selector");
    equal(field("legacy-gpio-power-group").hidden, true,
        "Pi 5 GPIO must hide the legacy 0-7 power control");
    equal(field("rp1_gpio_drive_ma").disabled, false,
        "Pi 5 GPIO must enable the RP1 drive selector");
    for (const drive of [2, 4, 8, 12]) {
        populateRp1GpioDrive(drive);
        equal(field("rp1_gpio_drive_ma").value, String(drive),
            `RP1 ${drive} mA must populate`);
        equal(buildConfigPayload().GPIO["RP1 Drive mA"], drive,
            `RP1 ${drive} mA must serialize unchanged`);
    }
    populateRp1GpioDrive(6);
    ok(!validateRp1GpioDrive(), "unsupported saved RP1 drive must remain invalid");
    equal(field("rp1_gpio_drive_ma").value, "",
        "unsupported saved RP1 drive must not be presented as a valid selection");
    equal(buildConfigPayload().GPIO["RP1 Drive mA"], 6,
        "unsupported saved RP1 drive must be preserved for server rejection rather than silently defaulted");
    ok(field("rp1-gpio-drive-error").textContent.includes("6 mA"),
        "unsupported saved RP1 drive must receive adjacent recovery guidance");
    populateRp1GpioDrive(8);
    equal(field("gpio-backend-panel").hidden, false,
        "GPIO selection must show GPIO calibration in its backend panel");
    equal(field("gpio_frequency_residual_ppm").disabled, false,
        "enabled system-clock estimation must keep the GPIO residual editable");
    equal(field("gpio_manual_ppm").disabled, false,
        "GPIO manual fallback must remain editable while estimation is enabled");

    field("transmit_backend").value = "si5351";
    clickTransmitBackend();
    equal(field("rp1_gpio_drive_ma").disabled, true,
        "Si5351 must disable the inactive RP1 drive selector without clearing it");
    equal(field("si5351-backend-panel").hidden, false,
        "Si5351 selection must show calibration in the matching backend panel");
    equal(field("ppm").disabled, false,
        "Si5351 reference calibration must remain editable independently");
    equal(field("use_system_clock_frequency_estimate").checked, true,
        "switching to Si5351 must preserve the GPIO estimate preference");
    equal(field("ppm").value, "2.409358",
        "switching to Si5351 must preserve its reference calibration");
    ok(field("ppm-hint").textContent.includes("Applied to the Si5351 reference"),
        "Si5351 must explain how manual PPM is applied");

    field("si5351_reference_source").value = "external_tcxo";
    syncSi5351ReferenceControls();
    equal(field("si5351-crystal-load-group").hidden, true,
        "external TCXO must hide the crystal load control");
    equal(field("si5351_crystal_load_capacitance").disabled, true,
        "external TCXO must disable the hidden crystal load control");
    field("si5351_reference_source").value = "crystal";
    field("si5351_crystal_load_capacitance").value = "8";
    syncSi5351ReferenceControls();
    equal(field("si5351-crystal-load-group").hidden, false,
        "crystal selection must reveal the load-capacitance select");
    equal(field("si5351_crystal_load_capacitance").disabled, false,
        "crystal selection must enable the load-capacitance select");

    const si5351Payload = buildConfigPayload();
    equal(si5351Payload.Operation["Transmit Backend"], "si5351",
        "Si5351 payload must retain the selected backend");
    equal(si5351Payload.GPIO["Use System Clock Frequency Estimate"], true,
        "Si5351 payload must preserve the independent GPIO estimate preference");
    equal(si5351Payload.GPIO["Frequency Residual PPM"], -0.125,
        "Si5351 payload must preserve the independent GPIO residual");
    equal(si5351Payload.GPIO["Manual PPM"], 1.75,
        "Si5351 payload must preserve the independent GPIO manual fallback");
    equal(si5351Payload.GPIO["RP1 Drive mA"], 8,
        "Si5351 payload must preserve the inactive RP1 drive selection");
    equal(si5351Payload.Calibration.PPM, 2.409358,
        "Si5351 payload must save manual Calibration.PPM");
    equal(si5351Payload.Si5351["Reference Source"], "crystal",
        "Si5351 payload must save the reference source");
    equal(si5351Payload.Si5351["Crystal Load Capacitance"], 8,
        "Si5351 payload must save the crystal load capacitance");

    populateRp1GpioDrive(6);
    clickTransmitBackend();
    equal(field("gpio-backend-panel").hidden, false,
        "Si5351 must reveal the GPIO recovery panel for an invalid retained RP1 value");
    equal(field("rp1-gpio-drive-group").hidden, false,
        "Si5351 must reveal an invalid retained RP1 value for recovery");
    equal(field("rp1_gpio_drive_ma").disabled, false,
        "Si5351 must enable an invalid retained RP1 selector for recovery");
    ok(!validateRp1GpioDrive(),
        "Si5351 must not treat an invalid serialized RP1 value as client-valid");
    field("rp1_gpio_drive_ma").value = "8";
    field("rp1_gpio_drive_ma").dispatchEvent(new Event("change", { bubbles: true }));
    equal(field("gpio-backend-panel").hidden, true,
        "repairing the retained RP1 value must dismiss the Si5351 recovery panel");
    equal(field("si5351-backend-panel").hidden, false,
        "repairing the retained RP1 value must leave the selected Si5351 panel visible");
    equal(buildConfigPayload().GPIO["RP1 Drive mA"], 8,
        "repairing the retained RP1 value must preserve the selected value");

    field("transmit_backend").value = "gpio";
    clickTransmitBackend();
    equal(field("gpio-backend-panel").hidden, false,
        "switching back to GPIO must restore its calibration panel");
    equal(field("use_system_clock_frequency_estimate").checked, true,
        "round-trip backend switching must preserve the GPIO estimate preference");
    equal(field("ppm").value, "2.409358",
        "round-trip backend switching must preserve Si5351 calibration");
    equal(field("gpio_manual_ppm").value, "1.75",
        "round-trip backend switching must preserve GPIO manual fallback");
    equal(field("rp1_gpio_drive_ma").value, "8",
        "round-trip backend switching must preserve RP1 drive strength");

    window.WSPRRYPI_PLATFORM.raspberryPiGeneration = 4;
    syncGpioDriveControls();
    equal(field("legacy-gpio-power-group").hidden, false,
        "Pi 4 GPIO must restore the legacy power control");
    equal(field("gpio-power-range").disabled, false,
        "Pi 4 GPIO must keep the legacy 0-7 power control authoritative");
    equal(field("rp1-gpio-drive-group").hidden, true,
        "Pi 4 GPIO must hide the RP1-only drive selector");
    equal(buildConfigPayload().GPIO["RP1 Drive mA"], 8,
        "Pi 4 operation must preserve the inactive RP1 selection");
    populateRp1GpioDrive(6);
    syncGpioDriveControls();
    equal(field("rp1-gpio-drive-group").hidden, false,
        "Pi 4 must reveal an invalid retained RP1 value for recovery");
    equal(field("rp1_gpio_drive_ma").disabled, false,
        "Pi 4 must enable an invalid retained RP1 selector for recovery");
    ok(!validateRp1GpioDrive(),
        "Pi 4 must not treat an invalid serialized RP1 value as client-valid");
    populateRp1GpioDrive(8);

    field("use_system_clock_frequency_estimate").checked = false;
    syncCalibrationControls();
    equal(field("gpio_frequency_residual_ppm").disabled, true,
        "disabled system-clock estimation must disable the unused residual");
    equal(field("gpio_manual_ppm").disabled, false,
        "disabled system-clock estimation must leave manual GPIO PPM editable");

    window.WSPRRYPI_PLATFORM.raspberryPiGeneration = 5;
    window.WSPRRYPI_PLATFORM.gpioClockTransmissionSupported = false;
    window.WSPRRYPI_PLATFORM.gpioClockTransmissionError =
        "The canonical RP1 GPCLK provider is unavailable. Review the RP1 clock route status below; transmission remains disabled.";
    window.WSPRRYPI_PLATFORM.rp1GpioOperatorVisible = true;
    equal(transmitBackendForUi("rp1-gpclk"), "gpio",
        "canonical RP1 backend must load into the operator-facing GPIO selector");
    field("transmit_backend").value = "gpio";
    initializeRp1RouteUi();
    await rp1RouteUi.query();
    updateBackendPlatformSupportUi();
    equal(field("transmit_backend").value, "gpio",
        "route-neutral Pi 5 RP1 must preserve GPIO for route administration");
    equal(field("transmit_backend").querySelector('option[value="gpio"]').textContent,
        "GPIO",
        "route-neutral Pi 5 RP1 must keep status out of the backend label");
    equal(field("transmit_backend").querySelector('option[value="gpio"]').disabled, false,
        "route-neutral Pi 5 RP1 must keep GPIO selectable for route administration");
    equal(field("rp1-gpio-drive-group").hidden, false,
        "route-neutral Pi 5 RP1 must show its route-bound drive selector");
    equal(field("backend-selector-hint").textContent,
        "GPIO uses the RP1 GPCLK provider. Route status and administration are beside Transmit Pin.",
        "route-neutral Pi 5 RP1 must use stable backend guidance");
    equal(field("backendPlatformHint").hidden, true,
        "the compact RP1 route controls must own route and provider status");
    equal(field("backendPlatformHint").textContent, "",
        "the backend platform hint must not duplicate RP1 route status");
    equal(field("backendStatus").hidden, true,
        `the backend alert must not duplicate RP1 route status (source=${field("backendStatus").dataset.source || "none"}, text=${field("backendStatus").textContent})`);
    equal(field("backendStatus").textContent, "",
        "the hidden backend alert must not retain provisional RP1 copy");
    equal(field("legacy-gpio-power-group").hidden, true,
        "Pi 5 must not substitute the legacy GPIO power control");
    equal(field("gpio-backend-panel").hidden, false,
        "route-neutral Pi 5 RP1 must expose the GPIO settings panel");
    equal(field("rp1-route-panel").hidden, true,
        "the detailed RP1 route panel must remain hidden for debugging");
    equal(field("rp1-route-apply").hidden, false,
        "route-neutral Pi 5 RP1 must expose the compact route action");
    equal(field("si5351-backend-panel").hidden, true,
        "route administration must not imply Si5351 was selected");
    equal(buildConfigPayload().GPIO["RP1 Drive mA"], 8,
        "route administration must preserve retained RP1 configuration");
    equal(buildConfigPayload().Operation["Transmit Backend"], "rp1-gpclk",
        "Pi 5 route administration must preserve the canonical RP1 backend");
    populateRp1GpioDrive(8);
    field("tx_pin").querySelector('option[value="20"]').disabled = false;
    field("tx_pin").value = "20";
    validateTransmitterHardwareFields();
    ok(validateRp1GpioDrive(),
        "route-neutral RP1 drive must remain valid for explicit route setup");
    equal(field("tx_pin").validationMessage, "",
        "route-neutral GPIO pin must remain a valid route draft");
    equal(buildConfigPayload().GPIO["RP1 Drive mA"], 8,
        "route setup must preserve the selected RP1 drive state");
    equal(buildConfigPayload().GPIO["Transmit Pin"], 20,
        "route setup must preserve the selected GPIO pin draft");
    rp1RouteUi.render({
        profile: "runtime", ok: true, state: "runtime_inhibited",
        requested: null, persisted: null, active: null,
        compatible: true, generation: 0,
    });
    ok(selectedBackendUnavailableMessage().includes("No RP1 clock route is selected"),
        "RP1 empty-route warning must acknowledge the neutral selector state");
    equal(field("transmit_backend").querySelector('option[value="gpio"]').textContent,
        "GPIO",
        "RP1 route state must remain in the compact route controls");
    rp1RouteUi.render({
        profile: "runtime", ok: true, state: "runtime_inhibited",
        requested: "GPIO20", persisted: "GPIO20", active: "GPIO4",
        compatible: true, generation: 1,
    });
    updateBackendPlatformSupportUi();
    ok(selectedBackendUnavailableMessage().includes("GPIO20 is selected, but GPIO4 is active"),
        "RP1 warning must describe a selected-versus-active mismatch");
    equal(field("transmit_backend").querySelector('option[value="gpio"]').textContent,
        "GPIO",
        "RP1 route mismatch must remain in the compact route controls");
    rp1RouteUi.render({
        profile: "runtime", ok: true, state: "runtime_ready",
        requested: "GPIO20", persisted: "GPIO20", active: "GPIO20",
        compatible: true, generation: 2,
    });
    updateBackendPlatformSupportUi();
    ok(selectedBackendUnavailableMessage().includes("GPIO20 is selected and active"),
        "RP1 warning must not ask for a route that is already active");
    equal(field("backend-selector-hint").textContent,
        "GPIO uses the RP1 GPCLK provider. Route status and administration are beside Transmit Pin.",
        "RP1 selector guidance must remain stable after status changes");
    field("transmit_backend").value = "si5351";
    field("transmit_backend").dispatchEvent(new Event("change", { bubbles: true }));
    equal(buildConfigPayload().Operation["Transmit Backend"], "si5351",
        "an explicit Si5351 selection must update the persisted backend");
    equal(field("si5351-backend-panel").hidden, false,
        "an explicit Si5351 selection must reveal its configuration panel");

    return { matrixCases, patches: patches.length, assertions: "passed" };
}

async function main() {
    const phpPort = await freePort();
    const debugPort = await freePort();
    const php = spawn("php", ["-S", `127.0.0.1:${phpPort}`, "-t", "data"], {
        cwd: UI_ROOT,
        stdio: "ignore",
    });
    const profileDir = `/tmp/wsprrypi-conditional-gpio-test-${process.pid}`;
    let chromium;
    let client;
    try {
        await waitFor(async () => await getStatus(
            `http://127.0.0.1:${phpPort}/index.php?page=config`) === 200,
        "local PHP fixture");
        const chromiumExecutable = process.env.CHROME_BIN || (
            process.platform === "darwin"
                ? "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome"
                : "chromium"
        );
        chromium = spawn(chromiumExecutable, [
            "--headless", "--no-sandbox", "--disable-gpu",
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
                expression: `({
                    ready: typeof refreshGpioConflictOptions === "function" &&
                        document.readyState === "complete",
                    state: document.readyState,
                    title: document.title,
                    url: location.href,
                    targetUrl: ${JSON.stringify(page.url)},
                    refreshType: typeof refreshGpioConflictOptions,
                    scheduleType: typeof scheduleAutosave,
                    scripts: [...document.scripts].map((script) => script.src),
                })`,
                returnByValue: true,
            });
            if (!result.result.value.ready) {
                throw new Error(JSON.stringify(result.result.value));
            }
            return true;
        }, "GPIO configuration scripts");

        const result = await client.send("Runtime.evaluate", {
            expression: `(${browserTest.toString()})()`,
            awaitPromise: true,
            returnByValue: true,
        });
        if (result.exceptionDetails) {
            const detail = result.exceptionDetails.exception && result.exceptionDetails.exception.description;
            throw new Error(detail || result.exceptionDetails.text || "Browser test failed");
        }
        assert.deepEqual(result.result.value, {
            matrixCases: 32,
            patches: 0,
            assertions: "passed",
        });
        if (process.env.WSPRRYPI_CONDITIONAL_GPIO_SCREENSHOT_DIR) {
            const screenshotDir = process.env.WSPRRYPI_CONDITIONAL_GPIO_SCREENSHOT_DIR;
            fs.mkdirSync(screenshotDir, { recursive: true });
            await client.send("Emulation.setDeviceMetricsOverride", {
                width: 1440,
                height: 1200,
                deviceScaleFactor: 1,
                mobile: false,
            });
            await captureConflictScreenshot(
                client,
                path.join(screenshotDir, "GPIO_RF_Conflict.png"),
                "transmitter-hardware-tab",
                "#gpio-backend-panel"
            );
            await captureConflictScreenshot(
                client,
                path.join(screenshotDir, "TX_LED_RF_Conflict.png"),
                "pi-hardware-tab",
                "#pi-hardware-pane > fieldset:first-of-type"
            );
            await captureConflictScreenshot(
                client,
                path.join(screenshotDir, "GPIO_PPM_Desktop.png"),
                "transmitter-hardware-tab",
                "#gpio-backend-panel .backend-calibration-section"
            );
            await captureRp1DriveScreenshot(client, path.join(screenshotDir, "RP1_Drive_Desktop_Light.png"), "light");
            await captureRp1DriveScreenshot(client, path.join(screenshotDir, "RP1_Drive_Desktop_Dark.png"), "dark");
            await captureRouteRequiredRp1Screenshot(client, path.join(screenshotDir, "RP1_Route_Required_Desktop_Light.png"), "light");
            await captureRouteRequiredRp1Screenshot(client, path.join(screenshotDir, "RP1_Route_Required_Desktop_Dark.png"), "dark");
            await captureRouteProgressModalScreenshot(client, path.join(screenshotDir, "RP1_Route_Progress_Desktop_Light.png"), "light");
            await captureRouteProgressModalScreenshot(client, path.join(screenshotDir, "RP1_Route_Progress_Desktop_Dark.png"), "dark");
            await captureBandPreferencesScreenshot(client, path.join(screenshotDir, "Band_Preferences_Desktop.png"));
            await client.send("Emulation.setDeviceMetricsOverride", {
                width: 390,
                height: 844,
                deviceScaleFactor: 1,
                mobile: true,
            });
            await captureConflictScreenshot(
                client,
                path.join(screenshotDir, "GPIO_PPM_Mobile.png"),
                "transmitter-hardware-tab",
                "#gpio-backend-panel .backend-calibration-section"
            );
            await captureBandPreferencesScreenshot(client, path.join(screenshotDir, "Band_Preferences_Mobile.png"));
            await captureRp1DriveScreenshot(client, path.join(screenshotDir, "RP1_Drive_Mobile_Light.png"), "light");
            await captureRp1DriveScreenshot(client, path.join(screenshotDir, "RP1_Drive_Mobile_Dark.png"), "dark");
            await captureRouteRequiredRp1Screenshot(client, path.join(screenshotDir, "RP1_Route_Required_Mobile_Light.png"), "light");
            await captureRouteRequiredRp1Screenshot(client, path.join(screenshotDir, "RP1_Route_Required_Mobile_Dark.png"), "dark");
            await captureRouteProgressModalScreenshot(client, path.join(screenshotDir, "RP1_Route_Progress_Mobile_Light.png"), "light");
            await captureRouteProgressModalScreenshot(client, path.join(screenshotDir, "RP1_Route_Progress_Mobile_Dark.png"), "dark");
        }
        console.log("conditional_transmit_gpio_integration_test passed");
    } finally {
        if (client) client.close();
        await terminate(chromium);
        await terminate(php);
        fs.rmSync(profileDir, {
            recursive: true,
            force: true,
            maxRetries: 5,
            retryDelay: 100,
        });
    }
}

main().catch((error) => {
    console.error(error.stack || error.message);
    process.exitCode = 1;
});
