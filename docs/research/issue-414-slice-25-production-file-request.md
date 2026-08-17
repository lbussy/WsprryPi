# Issue 414 Slice 25: Production Dropbox File Request

Status: Established and signed-out transfer qualified

## Corrected prerequisite

At the start of this slice, Dropbox's authoritative File Requests list was
empty. The only prior request URL available in approved browser history returned
`File request not found`. It was therefore rejected rather than signed into a
production manifest.

This discovery corrected the sequence: production File Request establishment
and signed-out verification had to precede generation-1 manifest preparation.

## Outcome

One open Dropbox File Request named `WsprryPi Support Bundles` now targets the
existing private `Support Bundle Intake/WsprryPi/Incoming` directory. No
description, password, invitation list, optional deadline, or naming convention
was configured. The authenticated File Requests view lists it once under
`Opened`, and only the maintainer can access the destination folder.

The live opaque request URL is not committed or printed in repository content.
It is stored in the macOS login Keychain as:

- service: `org.wsprrypi.support-intake`;
- account: `wsprrypi-file-request`; and
- label: `WsprryPi Dropbox File Request`.

The Keychain metadata and URL policy shape were validated without displaying the
secret. No plaintext URL file remains.

## Signed-out qualification

A fresh Chrome Incognito window displayed the request with a visible `Sign in`
option, proving the uploader was not authenticated to Dropbox. The page named
the WsprryPi request, offered file selection, and required name and email as
provider metadata. It did not expose the private destination path or maintainer
email address.

After explicit action-time approval, the test submitted one 72-byte synthetic
`.age`-named text fixture containing no diagnostic, credential, private key, or
personal data. Dropbox reported `Finished uploading`, and the authenticated
destination received one object. Dropbox appended the synthetic submitter name
to the stored filename, confirming filename parsing cannot be authoritative.

The locally synced received object matched the source fixture byte for byte:

```text
SHA-256: b4b42fad03761f4ba324d629ea5a7fdaf31e33f1822ce45fd89ed29029c7e0f8
Size: 72 bytes
```

After explicit deletion approval, the Dropbox object, source fixture, and two
incidental browser-download copies were removed. The authenticated `Incoming`
directory is empty and the File Request remains open. Dropbox may retain the
cloud deletion under its ordinary deleted-file recovery policy; the temporary
source is not ordinarily recoverable.

## Boundary and remaining work

Neither production private identity was opened or used. No intake manifest or
signature was prepared, committed, pushed, retrieved, or activated. The public
`WsprryPi/support-intake` repository remains at its README-only initial commit.

The next separately reviewed slice is local generation-1 manifest preparation
and lifecycle authentication. It must retrieve the request URL from Keychain
without printing it or placing it in process arguments. Candidate commit, live
push, exact public-byte verification, runtime activation, and encrypted upload
orchestration remain later independent boundaries.

No application, UI, installer, service, Raspberry Pi, GPIO, transmitter, or RF
state changed.
