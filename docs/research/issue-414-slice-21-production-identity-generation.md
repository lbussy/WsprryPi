# Issue 414 Slice 21: Production Identity Generation

Status: Production identities generated and correspondence verified; backup and
recovery qualification completed by Slice 22

## Outcome

After Slice 20 returned exact `ready`, the qualified Slice 13 provisioner
generated the production Ed25519 intake-signing identity and the qualified Slice
6 provisioner generated the production age X25519 bundle-encryption identity.
Both used the shared ceremony timestamp `2026-08-17T12:47:58Z` and the approved
external owner-only signing, encryption, and public-staging directories.

The production public identities are:

| Purpose | Key ID | Algorithm | SHA-256 fingerprint |
| --- | --- | --- | --- |
| Intake manifest signing | `wsprrypi-intake-2026-01` | Ed25519 | `688b5769d2b763481bad938fe8a9963693950c5e80bcf6d47d71db75711843ac` |
| Bundle encryption | `wsprrypi-bundle-2026-01` | age X25519 | `61289289afbd0f7813eb59b54e60d514f3cd8dbdf05e9c6b2d405b101b5b0fc4` |

No private bytes, age recipient, public-key encoding, local absolute path, or
generated metadata file is committed in this repository.

## Verification evidence

- Both private outputs are owner-owned, regular, single-link mode `0400` files.
- Both public metadata outputs are owner-owned, regular, single-link mode `0600`
  files.
- No provisioner partial or symlink remains.
- The age recipient independently derived from the private identity matched the
  public metadata byte for byte.
- The raw Ed25519 public key independently derived with OpenSSL 3.6.3 matched the
  canonical public metadata value byte for byte.
- Independent SHA-256 calculations matched both recorded fingerprints.
- Slice 19 strict trust compilation accepted the pair.
- The generated identifiers, algorithms, and shared timestamp matched the
  approved ceremony inputs.
- The disposable derived-public verification directory was removed after the
  comparisons passed.

## Mandatory stop state

Slice 22 subsequently qualified independent Dashlane attachment backups and
restored-key signing/decryption. The identities remain unpublished and
unactivated. No public metadata was committed or published, no manifest was
prepared or signed, and no runtime trust was compiled into the application.

See the Slice 22 evidence record for the completed vault and recovery ceremony.

No support-intake repository, GitHub/Dropbox route, credential, application,
installer, HTTP, UI, service, Pi, hardware, or RF state changed.
