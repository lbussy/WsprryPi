// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Lee Bussy
(function (root) {
    "use strict";
    const defaults = Object.freeze({
        "Endpoint": "", "USB Serial": "", "Device ID": "",
        "USB Vendor ID": 0, "USB Product ID": 0,
        "Start Uncertainty ns": 1000000, "Allow Frequency Adjustment": false
    });
    const storageKey = "wsprrypi.pico-development-controls";
    function errors(settings, selected) {
        const result = {};
        for (const [key, max] of [["Endpoint", 512], ["USB Serial", 128], ["Device ID", 32]]) {
            const value = settings[key];
            if (typeof value !== "string" || value.length > max || /[\x00-\x1f\x7f]/.test(value))
                result[key] = `Enter a valid ${key.toLowerCase()}.`;
        }
        if (selected) {
            if (!String(settings.Endpoint).startsWith("/dev/") || settings.Endpoint.includes("/../")) result.Endpoint = "Select the dedicated WTP device path under /dev/.";
            if (!settings["USB Serial"]) result["USB Serial"] = "Enter the selected device's USB serial.";
            if (!/^[0-9a-f]{32}$/.test(settings["Device ID"])) result["Device ID"] = "Enter 32 lowercase hexadecimal characters.";
        }
        for (const key of ["USB Vendor ID", "USB Product ID", "Start Uncertainty ns"]) {
            const value = settings[key];
            const minimum = key === "Start Uncertainty ns" || selected ? 1 : 0;
            const maximum = key === "Start Uncertainty ns" ? 1000000000 : 65535;
            if (!Number.isInteger(value) || value < minimum || value > maximum) result[key] = `Enter a whole number from ${minimum} to ${maximum}.`;
        }
        return result;
    }
    function summarize(s) {
        if (!s || s.selected !== true) return { text: s?.selected === false ? "Pico is not selected in the running application." : "Pico status is unavailable. Selection and output state are unconfirmed.", output: "Unknown", clock: "Unknown", identity: "Unknown", history: "None", recover: false };
        let age = null;
        if (/^\d+$/.test(s.now_ms) && /^\d+$/.test(s.status_observed_ms)) {
            const delta = BigInt(s.now_ms) - BigInt(s.status_observed_ms);
            if (delta >= 0n) age = `${delta / 1000n} s ago`;
        }
        const remote = s.remote;
        const output = typeof remote?.output_active === "boolean" && age !== null
            ? `${remote.output_active ? "Active" : "Inactive"} (${age}; last observation)` : "Unknown";
        const blocked = s.recovery_required || s.uncertain || s.safety_fault;
        const text = blocked ? "Recovery required. Output safety remains unresolved."
            : s.host_skip_waiting ? "Waiting for a skipped WSPR window; no Pico job was sent."
            : s.phase === "waiting" ? "Waiting to prepare the scheduled job."
            : s.phase === "preparing" ? "Preparing the complete job."
            : s.phase === "executing" ? `Device job: ${s.job?.state || "unconfirmed"}.`
            : s.ready ? "Idle. Device state was checked; each job requires fresh admission."
            : "Device readiness is unconfirmed. Check the endpoint and reconcile.";
        return { text, output, clock: s.host_utc_valid === true ? "Synchronized" : "Not synchronized",
            identity: s.identity ? `${s.identity.device_id} / ${s.identity.boot_id}` : "Unknown",
            history: s.last_report ? `${s.last_report.outcome} · ${s.last_report.job_id || "no remote job"}${s.last_report.error ? ` · ${s.last_report.error}` : ""}` : "None",
            recover: !s.worker_active && ["idle", "blocked"].includes(s.phase) && !["identity_changed", "fault"].includes(s.session_phase) };
    }
    if (typeof module !== "undefined" && module.exports) module.exports = { defaults, errors, summarize };
    if (!root.document) return;
    const byId = id => root.document.getElementById(id);
    let saved = { ...defaults }, snapshot = null, busy = false, timer = null, initialized = false, visible = false, closed = false, statusReadFailed = false;
    const selected = () => byId("wtp_use")?.checked === true;
    function read() {
        const result = { ...saved };
        root.document.querySelectorAll("[data-wtp-key]").forEach(field => {
            result[field.dataset.wtpKey] = field.type === "checkbox" ? field.checked
                : field.type === "number" ? (field.value === "" ? NaN : Number(field.value)) : field.value;
        });
        return result;
    }
    function validate() {
        const invalid = errors(read(), selected());
        root.document.querySelectorAll("[data-wtp-key]").forEach(field => field.setCustomValidity(invalid[field.dataset.wtpKey] || ""));
        return Object.keys(invalid).length === 0;
    }
    function render() {
        if (!byId("wtp-controls")) return;
        byId("wtp-controls").hidden = !visible;
        byId("wtp-hidden-selection").hidden = visible || !selected();
        byId("wtp_visible").setAttribute("aria-expanded", String(visible));
        byId("wtp_use").disabled = !visible;
        root.document.querySelectorAll("[data-wtp-key]").forEach(field => { field.disabled = !visible || !selected(); });
        const state = summarize(snapshot);
        for (const [id, value] of [["wtp-status-text", state.text], ["wtp-output", state.output], ["wtp-clock", state.clock], ["wtp-identity", state.identity], ["wtp-history", state.history]]) byId(id).textContent = value;
        byId("wtp-recover").disabled = busy || !selected() || !state.recover;
        if (selected() && typeof root.updateBackendPlatformSupportUi === "function") root.updateBackendPlatformSupportUi();
        else if (typeof root.syncTransmitAvailabilityUi === "function") root.syncTransmitAvailabilityUi();
    }
    async function request(recover = false) {
        if (busy || closed || (!visible && !selected())) return;
        busy = true;
        clearTimeout(timer);
        render();
        const controller = new AbortController();
        const timeout = setTimeout(() => controller.abort(), recover ? 30000 : 5000);
        if (recover) byId("wtp-feedback").textContent = "Reconciling the current session…";
        let receivedStatus = false;
        try {
            const url = root.WSPRRYPI_PATHS?.wtpPath || "/api/wtp";
            const response = await root.fetch(url + (recover ? "/recover" : ""), {
                method: recover ? "POST" : "GET", cache: "no-store", signal: controller.signal,
                ...(recover ? { headers: { "Content-Type": "application/json" }, body: JSON.stringify({ operation: "reconcile" }) } : {})
            });
            const data = await response.json();
            if (recover && data.status) { snapshot = data.status; receivedStatus = true; }
            if (!response.ok) throw new Error(data.error || `Request failed (${response.status}).`);
            if (!recover) {
                snapshot = data;
                if (statusReadFailed) byId("wtp-feedback").textContent = "Status connection restored.";
                statusReadFailed = false;
            }
            else byId("wtp-feedback").textContent = "Reconciliation finished. Review the current observation above.";
        } catch (error) {
            // Historical results may remain, but a failed read cannot establish current safety.
            if (!recover) statusReadFailed = true;
            if (!receivedStatus) snapshot = null;
            byId("wtp-feedback").textContent = error.name === "AbortError"
                ? "The request timed out. Device state is unconfirmed; refresh status before another recovery."
                : `Pico status unavailable: ${error.message}`;
        } finally {
            clearTimeout(timeout);
            busy = false;
            render();
            if (!closed && (visible || selected())) timer = setTimeout(() => request(), 3000);
        }
    }
    function populate(value) {
        saved = { ...defaults, ...(value || {}) };
        root.document.querySelectorAll("[data-wtp-key]").forEach(field => {
            const value = saved[field.dataset.wtpKey];
            if (field.type === "checkbox") field.checked = value === true;
            else field.value = String(value);
        });
        render();
    }
    root.WtpUi = { selected, read, validate, populate,
        select(value) { if (byId("wtp_use")) byId("wtp_use").checked = value; render(); if (initialized && !busy) request(); },
        unavailable() {
            if (!selected()) return "";
            if (!validate()) return "Complete the Pico endpoint settings before enabling transmission.";
            return snapshot?.selected === true && snapshot.ready === true && snapshot.host_utc_valid === true
                ? "" : "Pico and host UTC readiness are unconfirmed. Review Pico status before enabling transmission.";
        }
    };
    function init() {
        if (!byId("wtp_visible")) return;
        initialized = true;
        try { visible = root.localStorage.getItem(storageKey) === "true"; } catch (_) { /* Preference is optional. */ }
        byId("wtp_visible").checked = visible;
        populate(saved);
        byId("wtp_visible").addEventListener("change", () => {
            visible = byId("wtp_visible").checked;
            try { root.localStorage.setItem(storageKey, String(visible)); } catch (_) { /* Keep session preference. */ }
            render();
            if (visible || selected()) request(); else clearTimeout(timer);
        });
        byId("wtp_use").addEventListener("change", () => { render(); root.clickTransmitBackend?.(); request(); });
        byId("wtp-recover").addEventListener("click", () => request(true));
        root.addEventListener("pagehide", () => { closed = true; clearTimeout(timer); });
        root.addEventListener("pageshow", () => { closed = false; request(); });
        render();
        if (visible || selected()) request();
    }
    if (root.document.readyState === "loading") root.document.addEventListener("DOMContentLoaded", init);
    else init();
})(typeof window === "undefined" ? globalThis : window);
