# Wsprry Pi UI

This is the Bootstrap UI for [Wsprry Pi](https://github.com/WsprryPi/WsprryPi/).
It is installed with the current application installation.

## Packaged UI manifest

`scripts/generate_ui_manifest.py` generates and validates the schema-v1,
content-addressed manifest used to identify a packaged UI artifact. Generation
is build-time infrastructure only; the installer and runtime do not consume the
manifest yet.

The identity covers every regular file below the selected UI root except the
top-level runtime `cache/` and `backups/` directories, `ui-manifest.json`
itself, and `.DS_Store` metadata. Symbolic links and unsafe or non-normalized
manifest paths are rejected. Each record contains a normalized relative path
and a SHA-256 content digest; timestamps, permissions, ownership, and traversal
order are not identity inputs.

Generate and then validate a manifest with:

```sh
python3 scripts/generate_ui_manifest.py \
  --ui-root data \
  --output /tmp/ui-manifest.json \
  --source-commit "$(git rev-parse HEAD)" \
  --application-version 0.0.0
python3 scripts/generate_ui_manifest.py --validate /tmp/ui-manifest.json
```

Run the focused regression suite with `npm run test:manifest`.

The same module provides the read-only `classify_installed_ui()` comparison
layer. It validates the immutable packaged manifest, calculates the identity
of the covered files currently present, and returns `packaged`,
`locally_modified`, or fail-closed `unknown` state with deterministic modified,
added, and missing path lists. Classification does not alter the installed
files and is not connected to the browser or installer yet.
