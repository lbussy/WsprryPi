# Issue 414 Slice 35: Truthful Upload Reporting

Status: Implemented and non-hardware qualified; maintainer confirmation remains external

## Outcome

The retained private lifecycle now distinguishes `encrypted_downloaded`,
`upload_page_opened`, and `upload_reported_complete`. The ciphertext completion
callback reaches `encrypted_downloaded` only after exact-byte validation and
receipt publication. The guarded handoff reaches `upload_page_opened` only when
it has revalidated active signed intake and is about to issue the Dropbox
redirect.

A separate guarded empty-body POST records the user's completion assertion. It
is rejected before `upload_page_opened`, is idempotent after success, accepts no
provider metadata or user-supplied fields, and returns only the typed lifecycle
state. No application transition represents maintainer confirmation.

After requesting the Dropbox page, the Maintenance workflow presents a distinct
inline return step. Its unchecked assertion names Dropbox's `Finished
uploading` message and the submitted `.age` file. A transient failure preserves
the assertion for retry. Success says that the user reported Dropbox success,
explicitly denies maintainer confirmation, retains the case ID, and explains
that the readable archive remains until delete or expiry.

The version-1 receipt remains unchanged at
`upload_state=encrypted_artifact_downloaded`. The application does not claim to
mutate the copy already downloaded by the browser and does not issue a
replacement receipt in this slice.

## Validation

Local macOS validation passed:

- JavaScript syntax and whitespace checks;
- manager transition and idempotency tests;
- guarded HTTP body, ordering, lifecycle, retry, non-disclosure, and unchanged
  receipt tests;
- web-server wiring and complete UI unit/browser suites;
- DOM failure recovery, separate assertion, truthful copy, intake revocation,
  and workflow reset tests;
- Impeccable detector with no findings; and
- desktop and mobile visual inspection of handoff and reported-complete states.

An isolated owner-only wspr4 source snapshot passed the manager, HTTP, and
web-server wiring targets with the simulated/GPIO-free profile. The temporary
tree was removed and the normal wspr4 checkout was not modified.

No Dropbox page was opened, no upload was attempted, and no maintainer
confirmation was simulated. No application installation, service operation,
reboot, GPIO/I2C, transmission, or RF activity occurred.

## Documentation impact

- Added the Slice 35 prompt and this implementation record.
- Updated the implementation plan to identify truthful upload reporting as the
  current boundary.
- The separate operator guide still needs the page-opened, user-reported, and
  maintainer-confirmed distinctions, plus retry, retention, and case-ID advice.

## Remaining boundary

Maintainer receipt remains external to the application. The next slice should
add the optional GitHub continuation workflow: authenticated users may open or
update an issue with case correlation, while non-GitHub users retain the
required description/contact path already embedded in the encrypted bundle.
No diagnostic content or transfer capability may enter a public issue.
