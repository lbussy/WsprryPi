// SPDX-License-Identifier: MIT
"use strict";
const assert = require("node:assert/strict"), fs = require("node:fs"), vm = require("node:vm"), path = require("node:path");
const source = fs.readFileSync(path.join(__dirname, "../data/wtp.js"), "utf8");
const { defaults, errors, summarize } = require("../data/wtp.js");
const settings = { ...defaults, Endpoint: "/dev/serial/by-id/pico-if02", "USB Serial": "000012345678", "Device ID": "4".repeat(32), "USB Vendor ID": 51966, "USB Product ID": 16402 };
assert.deepEqual(errors(settings, true), {});
assert.match(summarize(null).text, /unavailable/);
assert.match(summarize({selected:false}).text, /not selected/);
assert.deepEqual(errors(defaults, false), {});
assert.equal(Object.keys(errors(defaults, true)).length, 5);
for (const bad of [-1, 1.2, NaN, 65536, true]) assert(errors({ ...settings, "USB Vendor ID": bad }, true)["USB Vendor ID"]);
const status = { selected: true, ready: true, phase: "idle", session_phase: "ready", now_ms: "9007199254749999", status_observed_ms: "9007199254740999", remote: { output_active: false }, identity: { device_id: "d", boot_id: "b" }, last_report: { outcome: "complete", job_id: "old" } };
assert.match(summarize(status).output, /Inactive \(9 s ago/);
assert.equal(summarize({ ...status, remote: null }).output, "Unknown");
assert.match(summarize({ ...status, recovery_required: true }).text, /unresolved/);
assert.equal(summarize({ ...status, worker_active: true }).recover, false);
assert.equal(summarize({ ...status, session_phase: "identity_changed" }).recover, false);
assert.match(summarize({ ...status, remote: { output_active: true } }).output, /^Active/); // historical completion cannot override current output
const fields = new Map();
function field(id, key, type = "text") {
    const f = { id, dataset: key ? { wtpKey: key } : {}, type, value: "", checked: false, disabled: false, hidden: false, listeners: {}, textContent: "", setCustomValidity(v) { this.validityMessage = v; }, setAttribute() {}, addEventListener(name, fn) { this.listeners[name] = fn; } };
    fields.set(id, f); return f;
}
for (const id of ["wtp_visible", "wtp_use", "wtp-controls", "wtp-hidden-selection", "wtp-status-text", "wtp-output", "wtp-clock", "wtp-identity", "wtp-history", "wtp-recover", "wtp-feedback"]) field(id);
for (const [key, value] of Object.entries(defaults)) field(key, key, typeof value === "number" ? "number" : typeof value === "boolean" ? "checkbox" : "text");
const writes = [], calls = [], events = {}, storage = new Map();
let response = status;
const root = { document: { readyState: "loading", getElementById: id => fields.get(id), querySelectorAll: () => [...fields.values()].filter(f => f.dataset.wtpKey), addEventListener: (name, fn) => { events[name] = fn; } }, localStorage: { getItem: k => storage.get(k), setItem: (k, v) => { storage.set(k, v); writes.push(v); } }, addEventListener() {}, fetch: async (url, options) => { calls.push({ url, options }); return { ok: true, json: async () => response }; } };
const context = { window: root, AbortController, setTimeout: () => 1, clearTimeout() {}, console };
vm.runInNewContext(source, context);
(async () => {
    events.DOMContentLoaded();
    assert.equal(fields.get("wtp_visible").checked, false);
    assert.equal(fields.get("wtp-controls").hidden, true);
    root.WtpUi.populate(settings); root.WtpUi.select(true);
    await new Promise(setImmediate);
    assert.equal(root.WtpUi.selected(), true);
    assert.equal(fields.get("wtp-hidden-selection").hidden, false);
    for (const visible of [true, false, true]) {
        fields.get("wtp_visible").checked = visible;
        fields.get("wtp_visible").listeners.change();
        await new Promise(setImmediate);
        assert.equal(root.WtpUi.selected(), true);
        assert.equal(JSON.stringify(root.WtpUi.read()), JSON.stringify(settings));
    }
    assert(calls.every(c => c.options.method === "GET"));
    assert.deepEqual(writes, ["true", "false", "true"]);
    fields.get("USB Serial").value = "";
    assert.equal(root.WtpUi.validate(), false);
    fields.get("wtp_visible").checked = false; fields.get("wtp_visible").listeners.change();
    assert.equal(root.WtpUi.read()["USB Serial"], ""); // hiding does not restore or normalize a failed draft
    await new Promise(setImmediate);
    console.log("WTP UI validation, observation, hidden-selection and draft-preservation tests passed");
})().catch(error => { console.error(error); process.exitCode = 1; });
