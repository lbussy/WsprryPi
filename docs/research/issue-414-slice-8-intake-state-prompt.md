# Issue 414 Slice 8: Signed-Intake Rollback State Prompt

## Objective

Implement and qualify the private filesystem state that remembers the highest
accepted signed-intake manifest generation and its exact-byte SHA-256. Preserve
the Slice 7 validator as an offline pure trust boundary and stop before HTTPS
retrieval, production keys/endpoints, runtime wiring, or UI.

## Verified context

- Slice 7 accepts caller-supplied previous generation/hash state and rejects
  rollback and same-generation mutation, but does not persist state.
- The installed support-bundle root is an owner-only `0700` directory beneath
  `/var/lib/wsprrypi`; this slice receives an injected root and does not change
  installation behavior.
- Persisted state is private operational metadata, not a secret, and must never
  contain request/release URLs, user messages, signatures, or diagnostic data.

## Scope

1. Add a typed C++ primitive to load and atomically commit a fixed version-1
   state document beneath an injected support-bundle root.
2. Require an absolute, real, current-user-owned `0700` root opened without
   following a symlink. Perform file operations relative to the verified
   directory descriptor.
3. Strictly validate a bounded `intake-state.json`: regular file, one link,
   current owner, exact `0600`, schema/project, positive generation, and exactly
   64 lowercase hexadecimal SHA-256 characters. Reject duplicate, unknown,
   missing, wrong-type, trailing, empty, oversized, symlinked, or unsafe state.
4. Commit only schema, project, generation, and manifest SHA-256. Refuse a lower
   generation or a different hash at the same generation; treat identical state
   as idempotent; allow only a higher generation.
5. For a higher generation, create a same-directory exclusive `0600` temporary
   file, write all bytes, sync it, atomically rename it over the fixed state,
   and sync the directory. Clean removable temporary output on pre-publication
   failure and never truncate the prior state in place.
6. Add focused tests for first commit, load, higher commit, idempotency, rollback,
   same-generation mutation, malformed documents, permissions/ownership-shape,
   symlinks, hard links, temp collisions, bounds, and prior-state preservation.
7. Add the focused target to Debian non-hardware CI and document the boundary.

## Constraints and non-goals

- Do not persist request URLs, release URLs, version text, messages, signatures,
  keys, recipients, contact data, or support-bundle contents.
- Do not add HTTPS, DNS, TLS, redirects, timeout policy, production endpoints,
  production keys, runtime construction, installer changes, services, or UI.
- Do not weaken Slice 7 validation or make persisted state authorize a manifest;
  the signature and complete policy must still validate every fetched response.
- Do not perform hardware, GPIO, I2C, transmitter, or RF activity.

## Validation

- Focused C++ filesystem and transition tests on macOS and Debian.
- Confirm exact `0600` state, no unexpected fields, atomic higher-generation
  replacement, prior-state survival across rejected updates, and no symlink or
  hard-link acceptance.
- `git diff --check`, complete staged-diff review, and independent adversarial
  review; correct every actionable finding before commit.

## Exit criteria

Stop with an offline persistence primitive and documented outcome. Commit and
push only attributable Slice 8 files. Secure HTTPS retrieval and application
integration remain later slices.
