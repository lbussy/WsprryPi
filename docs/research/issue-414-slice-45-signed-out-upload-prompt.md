# Issue #414 Slice 45 — signed-out encrypted upload qualification

## Objective

Exercise one complete end-user support-bundle workflow on `wspr4`: collect a
readable candidate for Issue #414, inspect it locally, finalize the exact
downloaded bytes, encrypt it with the signed production intake key, download
the ciphertext and receipt, and upload only the ciphertext through a fresh
signed-out Dropbox File Request session.

## Scope

Use a release-like qualification build whose canonical version satisfies the
signed minimum upload version. Keep transmission disabled and omit the optional
I2C probe. Verify candidate structure, manifest identity and version, exact
archive and ciphertext digests, signed intake status, receipt correlation,
anonymous Dropbox completion, and truthful local upload-reporting state. Retain
the ciphertext and receipt privately for the maintainer-processing slice.

If qualification exposes an attributable blocker, correct the smallest
maintainable contract violation, add focused regression coverage, reassess it
on macOS and Linux, and repeat until no actionable finding remains.

## Constraints and non-goals

Do not upload the readable archive or receipt. Do not disclose the private
request URL in committed evidence. Do not decrypt or process the production
upload, delete Dropbox data, alter production keys or manifests, reboot, probe
I2C, operate GPIO or transmitter hardware, enable transmission, or emit RF.
Preserve ordinary worktrees and unrelated target data.

## Acceptance

Require a canonical project version in the readable manifest, safe archive
paths, exact digest and receipt agreement, successful local encryption, a
signed-out Dropbox page displaying `Finished uploading`, retained private
maintainer inputs, an active/enabled service with `Transmit = false`, working
same-origin and direct-fallback intake routes, focused automated checks,
desktop and narrow responsive visual review, a clean adversarial assessment,
and truthful committed evidence.
