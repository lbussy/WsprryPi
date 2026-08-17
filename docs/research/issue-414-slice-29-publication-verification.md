# Issue 414 Slice 29: Production Public Verification

Status: Exact public pair retrieved and authenticated; runtime inactive

## Outcome

The published production intake pair is end-to-end verified. The qualified
Slice 18 controller retrieved the two fixed public resources, compared both
bodies byte-for-byte with the authenticated local generation, independently
authenticated the retrieved pair, and returned:

```text
status: verified
generation: 1
manifest SHA-256: 80902216b212ca1a8c2a9fd3e9693aac2c0aa17c7838d939bbebaa8887fb71e8
candidate commit: 3e0b4017bfe7612bd39ccc6e2f29d743174664b5
```

Neither response body was displayed, logged, copied into WsprryPi, or retained
outside the controller's owner-only temporary authentication directory.

## Retrieval and authentication evidence

Before retrieval, the controller re-authenticated the owner-controlled staged
source and revalidated the exact local publication candidate. It then used
fixed root-owned `/usr/bin/curl` to retrieve only:

```text
wsprrypi/intake.json
wsprrypi/intake.json.sig
```

The transport disabled curl configuration and proxies, permitted only HTTPS,
retained ordinary certificate and hostname validation, followed no redirects,
applied connect and total deadlines, required HTTP 200, and enforced separate
body limits.

The controller required exact equality between both retrieved bodies and the
local authenticated pair. It then wrote the retrieved bytes only to an
owner-only temporary generation directory and independently ran lifecycle
authentication using the reviewed public signing metadata. Generation and
manifest digest exactly matched the source.

## Transient first retrieval

The first controller invocation failed closed with typed status
`retrieval_failed`; it retained or disclosed no body. Status-only checks then
showed HTTP 200 for both fixed endpoints with bodies discarded. A transient
reproduction of the controller's fixed curl contract recorded only exit status,
bounded byte count, and the four-byte HTTP trailer; both responses had exit 0
and the expected `\n200` trailer. No content was printed.

One deliberate controller retry was then performed. It returned `verified`.
There were no further retries, content normalization, repairs, alternate URLs,
redirects, or TLS-policy changes.

## Post-verification checks

After successful verification:

- a separate controlled ref-only query confirmed remote `refs/heads/main` still
  equals `3e0b4017bfe7612bd39ccc6e2f29d743174664b5`;
- local lifecycle inspection again authenticated active generation 1 with
  signing key `wsprrypi-intake-2026-01`, bundle key
  `wsprrypi-bundle-2026-01`, and the same manifest digest;
- both local candidate blobs remain byte-identical to the staged source; and
- no local publication or staging ref/file changed.

Focused validation passed:

- manifest lifecycle: 10 tests, one real-tool fixture skipped locally;
- publication commit: 8 tests;
- publication verification: 7 tests;
- Python syntax and final diff checks.

Known macOS Make probes for Linux `/proc/meminfo` and `nproc` emitted warnings
without affecting the tests.

## Remaining boundary

This slice does not compile or activate production trust in the application,
expose the request capability to runtime, encrypt or upload a support bundle, or
change UI, installer, service, Raspberry Pi, GPIO, transmitter, or RF behavior.

The published pair is now suitable input for a separately reviewed production
trust compilation and activation decision. That later work must preserve the
existing signature-before-disclosure, rollback, expiry, disabled, and minimum
upload-version contracts.
