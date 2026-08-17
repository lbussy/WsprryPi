# Issue 414 Slice 25 Execution Prompt: Production Dropbox File Request

## Objective

Establish and independently verify the production Dropbox File Request for
WsprryPi support-bundle ciphertext. Recover the missing live upload capability
required before generation-1 manifest preparation, without signing or publishing
any manifest.

## Verified context

- The Dropbox account is signed in and the existing hierarchy is
  `Support Bundle Intake/WsprryPi/Incoming` plus `Processed`.
- Dropbox's authoritative File Requests page currently contains no requests.
- The only prior request URL found in the approved browser history now returns
  `File request not found`; it must not be signed or reused.
- The contract requires a project-specific request directed to WsprryPi's
  `Incoming` directory and a signed-out acceptance check.
- Slices 21-24 established the production identities, public trust, and
  publication repository, but none should be used in this slice.

## Scope and requirements

1. Create one Dropbox File Request named `WsprryPi Support Bundles` with the
   destination `Support Bundle Intake/WsprryPi/Incoming`.
2. Do not add a deadline, password, requester-specific invitation, or recipient
   email; the link itself is the upload capability.
3. Confirm the request appears once as open in the authenticated account and
   that its destination is exact.
4. Open the resulting URL in a signed-out/private browser session and confirm:
   - no Dropbox account is required;
   - name and syntactically valid email are required provider metadata;
   - file selection is available; and
   - the page identifies the WsprryPi request without exposing the destination
     directory or maintainer email.
5. Upload one small synthetic `.age`-named fixture containing no diagnostic,
   credential, private-key, or personal data. Use an explicitly approved,
   syntactically valid test address and an explicitly synthetic name without
   recording either value in Git.
6. Confirm Dropbox reports success and the authenticated account receives the
   upload notification. Download or inspect the stored object only as needed to
   compare exact fixture bytes, then remove the synthetic upload while leaving
   the request open.
7. Record the request's opaque URL only in the macOS login Keychain as service
   `org.wsprrypi.support-intake`, account `wsprrypi-file-request`, for Slice 26.
   Do not commit, print, or place the capability in process arguments.
8. Record non-sensitive outcome evidence in WsprryPi documentation.

## Constraints and non-goals

- Do not prepare, sign, commit, push, retrieve, or activate an intake manifest.
- Do not open or use either production private identity.
- Do not upload a real support bundle or any identifying diagnostic contents.
- Do not expose the request URL, Dropbox destination, or email address in Git,
  logs, command output, screenshots, or public documentation.
- Do not modify the application, UI, installer, service, Pi, GPIO, transmitter,
  or RF state.
- Preserve all unrelated Dropbox content, repositories, worktrees, and branches.

## Validation and adversarial review

- Verify exact authenticated request name, open state, and destination.
- Verify the signed-out workflow through a browser context with no Dropbox
  session before uploading the synthetic fixture.
- Compare uploaded/downloaded fixture bytes and SHA-256 without retaining the
  disposable object.
- Confirm cleanup removed only the synthetic upload and the request remains
  open and usable.
- Validate the Keychain item's service/account metadata and stored URL shape
  without printing its contents.
- Run `git diff --check` and scan the WsprryPi diff for request IDs, URLs,
  submitter data, credentials, private-key markers, or runtime activation.

## Exit criteria

Commit and push only the prompt, non-sensitive implementation record, plan
index, and prior-slice handoff after the request is open, signed-out acceptance
and exact-byte transfer are proven, all synthetic copies are removed, the URL is
retained only in the approved Keychain item, and the WsprryPi diff is clean and
non-disclosing.
