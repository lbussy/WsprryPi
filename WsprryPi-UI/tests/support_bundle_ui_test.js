"use strict";

const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");

const root = path.resolve(__dirname, "..");
const view = fs.readFileSync(path.join(root, "data/views/maintenance.php"), "utf8");
const script = fs.readFileSync(path.join(root, "data/maintenance.js"), "utf8");
const styles = fs.readFileSync(path.join(root, "data/maintenance.css"), "utf8");
const header = fs.readFileSync(path.join(root, "data/header.php"), "utf8");
const site = fs.readFileSync(path.join(root, "data/site.js"), "utf8");

function includes(source, expected, message) {
    assert.ok(source.includes(expected), message);
}

includes(view, "Support Bundle", "Maintenance must expose the Support Bundle panel");
includes(view, "Create Support Bundle", "Support Bundle must have an explicit create action");
includes(view, "id=\"supportBundleProbeI2c\"", "active I2C probe must be an explicit checkbox");
includes(view, "aria-describedby=\"supportBundleProbeI2cHelp\"", "probe warning must be associated with its checkbox");
includes(view, "i2cdetect -y 1", "active I2C probe warning must name the exact command");
includes(view, "Passive I²C information is included in normal collection.", "passive I2C collection must be explained");
includes(view, "aria-live=\"polite\"", "progress must be announced politely");
includes(view, "role=\"alert\"", "immediate failures must use alert treatment");
includes(view, "Delete from Pi", "cleanup retry must be explicitly destructive");

includes(header, "'supportBundlesPath'", "support bundle path must be centrally configured");
includes(site, "const SUPPORT_BUNDLES_ENDPOINT", "support bundle endpoint must use central fallback definitions");
includes(site, "buildDirectRestFallbackUrl(\"/api/support-bundles\")", "support bundle direct fallback must remain centralized");
includes(script, "JSON.stringify({ probe_i2c: supportBundleProbeI2c.checked })", "default and checked consent must produce boolean probe payloads");
includes(script, "snapshot.state === \"queued\"", "queued jobs must poll");
includes(script, "snapshot.state === \"running\"", "running jobs must poll");
includes(script, "stopSupportBundlePolling();", "terminal and unload paths must stop polling");
includes(script, "await response.blob();", "download must receive the complete archive into a Blob");
includes(script, "if (!response.ok)", "non-OK downloads must be rejected before cleanup");
includes(script, "if (blob.size === 0", "incomplete downloads must not be treated as complete");
includes(script, "invokeBrowserDownload(blob, filename);\n            await deleteSupportBundle(jobId, true, filename);", "DELETE must follow complete Blob receipt and browser download invocation");
includes(script, "URL.revokeObjectURL(objectUrl)", "download object URLs must be revoked");
includes(script, "safeSupportBundleFilename", "attachment filenames must be validated");
includes(script, "/^[A-Za-z0-9][A-Za-z0-9._-]*\\.tar\\.gz$/i", "unsafe attachment filenames must fall back");
includes(script, "SUPPORT_BUNDLE_FILENAME_FALLBACK", "safe filename fallback must exist");
includes(script, "will expire automatically within 24 hours", "cleanup failure must explain retention");
includes(script, "Your browser chose the save location", "UI must not claim a filesystem path");
assert.ok(!script.includes("upload"), "support bundle UI must not auto-upload");
includes(script, "supportBundleDownloadInFlight", "duplicate downloads must be prevented");
includes(script, "supportBundleCreateInFlight", "duplicate creation must be prevented");
includes(script, "textContent", "server-derived values must use DOM-safe text assignment");
includes(script, "let supportBundleModalOpener = null", "Support Bundle modal must retain its opener");
includes(script, "event.currentTarget", "Support Bundle modal must retain the actual invoking element");
includes(script, "supportBundleModalElement?.addEventListener(\"hidden.bs.modal\", restoreSupportBundleModalFocus)", "one hidden lifecycle listener must restore focus for every dismissal path");
includes(script, "function isSupportBundleFocusTarget(element)", "focus restoration must verify that its target remains usable");
includes(script, "element.isConnected", "a detached modal opener must not receive focus");
includes(script, "!element.matches(\":disabled\")", "a disabled modal opener must not receive focus");
includes(script, ": createSupportBundleButton", "a detached or disabled opener must fall back to the stable create button");
includes(script, "supportBundleModalOpener = null", "modal opener state must be cleared after the hidden lifecycle");
includes(script, "opener.focus({ preventScroll: true })", "focus must return only after the hidden lifecycle completes");
assert.equal((script.match(/hidden\.bs\.modal/g) || []).length, 1, "repeated modal opens must not add duplicate focus lifecycle handlers");
includes(styles, ".maintenance-pane--support", "Support Bundle must use the established Maintenance panel treatment");
includes(styles, "@media (max-width: 767.98px)", "existing narrow full-width action behavior must remain available");

console.log("support_bundle_ui_test passed");
