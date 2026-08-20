# Issue #414 Slice 46 — Maintainer Processing Qualification

Date: 20 August 2026

## Outcome

The Dropbox-synchronized copy of Slice 45 case `KPXV-ZKYQ-8P7J` independently
matched the retained receipt: 108647 ciphertext bytes with SHA-256
`400c97f7384312792aab1526353d970e6524f60372684a52541f09e2fcd09b3a`.
The inspector selected production key ID `wsprrypi-bundle-2026-01`, decrypted
the exact authenticated bytes, validated the complete archive and its 120-file
manifest without extraction, correlated it to Issue #414, and removed its
temporary plaintext workspace.

Separately staged private copies were promoted with retention class
`active_case`. The canonical case directory is owner-only `0700`; its encrypted
bundle, receipt, and processing record are `0600`. The promoted ciphertext and
receipt match the retained originals. Incoming and the temporary work directory
are empty. The processing record contains the expected case, artifact, public
key ID, Issue #414 correlation, `processed` lifecycle, and no retention-review
timestamp. The Dropbox source and both retained Downloads handoff files remain
present and unchanged.

## Findings closed

Real qualification exposed three interoperability defects in the Slice 37
inspector. Provisioned production identities intentionally use mode `0400`, but
the inspector required `0600`; both exact owner-only modes are now accepted.
macOS cannot reliably give Homebrew `age` access to inherited `/dev/fd` paths,
so the non-Linux path now stages only a bounded ciphertext copy inside the
private temporary workspace and supplies the already-open identity descriptor
through standard input. Linux retains descriptor paths under `/proc/self/fd`.
Maintainers may explicitly select an absolute safe `age` executable on other
platforms while production continues to default to `/usr/bin/age`.

The generated 120-file manifest was 17271 bytes, revealing that the inspector
incorrectly reused the 16 KiB receipt limit for bundle manifests. Receipt JSON
remains capped at 16 KiB; manifests now have an independent 256 KiB bound that
can represent the permitted inventory. A regression proves a valid manifest
above the receipt limit succeeds and still fails closed above its own bound.

## Validation

The focused inspection suite passed all ten tests on macOS, including genuine
Homebrew `age`/`age-keygen`; the processing suite passed all twelve tests.
Python syntax and resource-warning checks passed. Real production-key inspection
reported `inspected`; the real promotion reported `processed`. Independent
post-transaction checks confirmed exact hashes, modes, safe record fields,
empty Incoming/work directories, and unchanged Dropbox/Downloads sources.

No diagnostic file was extracted or opened. No Dropbox source was moved or
deleted, no GitHub state changed, and no installation, service lifecycle,
reboot, hardware, GPIO, transmitter, or RF operation occurred.

## Documentation Impact

This execution prompt and implementation record document the maintainer-only
qualification, and the Slice 37 record now states the corrected identity,
executable, and manifest-bound contracts. The separate operator documentation
repository was considered but remains unchanged because cross-repository work
was not authorized. A maintainer runbook for acquiring, inspecting, promoting,
and later reviewing cases is still required there.

## Remaining boundary

The promoted case remains active and is not eligible for retention deletion.
Maintainer confirmation or any Issue #414 comment, later resolved-case
classification, Dropbox cleanup, backup cleanup, or diagnostic examination is a
separate explicit action.
