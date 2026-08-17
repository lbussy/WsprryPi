# Issue 414 Slice 24: Publication Repository Establishment

Status: Established and validated; no intake publication

## Outcome

The dedicated public GitHub repository now exists at
`https://github.com/WsprryPi/support-intake`. Its description is
`Signed WsprryPi support-bundle intake configuration`, its default and only
branch is `main`, and Issues, Wiki, Projects, and Discussions are disabled.

The repository has one benign root commit:

```text
770d63521cf23d1ccb5eb7c9911e040ab18032d7
```

That commit has no parent and its exact tree contains only mode `100644`
`README.md`. The README identifies the repository and repeats its description.
There is no manifest, signature, workflow, publication credential, or routing
value in the remote tree. This slice created no tag, release, hook, deploy key,
secret, environment, collaborator, or team grant.

No branch-protection rule was added. This is intentional: the Slice 17
maintainer controller is the reviewed direct-publication boundary and uses an
exact candidate, exact remote-parent check, and exact `--force-with-lease`.
Generic pull-request enforcement would block that narrow protocol without
adding stronger identity checks to it.

## Local publication repository

An owner-controlled bare clone exists outside the WsprryPi checkout under the
maintainer's established `WsprryPi Support Intake/publication/` root. Both the
publication root and bare repository are owned by the maintainer and have exact
mode `0700`.

The production Slice 16 validator confirms:

- bare repository form;
- symbolic `HEAD` at `refs/heads/main`;
- exactly one ref, `refs/heads/main`, at the remote initial commit;
- exactly one remote, `origin`, with the exact fetch and push URL
  `https://github.com/WsprryPi/support-intake.git`;
- no alternates, shallow state, grafts, or replacement refs; and
- no unsafe local transport configuration under the Slice 17 policy.

The external absolute maintainer path is deliberately not embedded in source,
configuration, logs, or public metadata. It is operational state, not
application configuration.

## Validation

GitHub repository metadata, branch inventory, root-commit parents, and exact
tree were queried after creation. An independent controlled `git ls-remote`
returned only the same `main` commit. The remote README was inspected, and the
local bare clone passed the production Slice 16 and Slice 17 repository-policy
validators.

The existing focused tests passed:

- Slice 16 publication commit: 8 tests;
- Slice 17 authenticated push: 8 tests; and
- Slice 18 public verification: 7 tests.

Python syntax checks and the final WsprryPi diff check also passed. The known
macOS Make probes for Linux `/proc/meminfo` and `nproc` emitted warnings without
affecting the tests.

## Publication boundary and remaining work

No production manifest or signature was prepared, committed, pushed, or
retrieved. Neither private identity was opened or used. The remote contains no
`wsprrypi/intake.json` or `wsprrypi/intake.json.sig`.

Slice 25 subsequently established and signed-out-qualified the production
Dropbox File Request after discovering the prior test capability was gone. The
next separately reviewed slice is generation-1 manifest preparation and local
lifecycle authentication using the Keychain-held request URL and approved
production identities. Candidate commit, live push, public exact-byte
verification, and runtime activation remain later independent boundaries.

No Dropbox administration, application runtime, installer, HTTP, UI, service,
Raspberry Pi, GPIO, transmitter, or RF state changed.
