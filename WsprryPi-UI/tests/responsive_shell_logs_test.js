"use strict";

const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");

const root = path.resolve(__dirname, "..");
const siteCss = fs.readFileSync(path.join(root, "data/site.css"), "utf8");
const logsCss = fs.readFileSync(path.join(root, "data/view_logs.css"), "utf8");
const logsJs = fs.readFileSync(path.join(root, "data/view_logs.js"), "utf8");

assert.match(
    siteCss,
    /@media \(min-width: 992px\) and \(max-width: 1399\.98px\)[\s\S]*?#mainNav[\s\S]*?flex: 1 1 100%/,
    "compact desktop navigation must use an intentional second row"
);
assert.match(
    siteCss,
    /@media \(max-width: 767\.98px\)[\s\S]*?grid-template-areas:[\s\S]*?"copy"[\s\S]*?"status"[\s\S]*?\.navbar-title[\s\S]*?white-space: normal/,
    "mobile masthead must retain readable title and controller status rows"
);
assert.match(
    siteCss,
    /@media \(max-width: 575\.98px\)[\s\S]*?footer\.fixed-bottom\s*{\s*position: static;/,
    "phone footer must participate in document flow"
);
assert.doesNotMatch(
    siteCss.match(/@media \(max-width: 575\.98px\)[\s\S]*?@media \(prefers-reduced-motion/)?.[0] || "",
    /text-overflow:\s*ellipsis/,
    "phone footer must not ellipsize the build identifier"
);
assert.match(
    logsCss,
    /\.logs-line\s*{[\s\S]*?display:\s*grid;/,
    "log entries must expose structured columns"
);
assert.match(
    logsCss,
    /@media \(max-width: 767\.98px\)[\s\S]*?\.logs-ts\s*{[\s\S]*?grid-column:\s*1 \/ -1/,
    "mobile timestamps must remain on their own complete row"
);
assert.match(
    logsJs,
    /function formatUnitFixed16\(unit\)[\s\S]*?return s;\s*}/,
    "log service names must remain complete"
);
assert.doesNotMatch(
    logsJs.match(/function formatUnitFixed16\(unit\)[\s\S]*?\n\s*}/)?.[0] || "",
    /slice\(|padEnd\(/,
    "log service names must not be truncated or padded into fragments"
);

console.log("responsive_shell_logs_test passed");
