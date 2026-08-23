# Issue 412: RP1-GPCLK-DKMS 1.1.1 executor-bearing consumer

WsprryPi accepts only `rp1-gpclk-dkms_1.1.1-1_all.deb` SHA-256
`247bd7da35e4ad812a13828668fe03673da127bad7ed2b3e970876f3f21c002d`,
from source commit `1bcdb0d6dcc2c56dfa8ee1c089386c9ca4f9cddd`. The ABI-v2 UAPI
SHA-256 is `998ab96d7dbcc0d935c05758c46acba56bbcf92aa1b674b899bdab6932dc8384`.
The only endpoint is `/dev/rp1-gpclk`.

The installed executor SHA-256 is
`a1e247df88650cad0866cc37f946a2859e0594e457a03f8674f6a691901be2da`;
the schema SHA-256 is
`097762cf365e864162b1199bceb05d2937b719ddf426d3a90b6a7f680803251b`;
and the installed contract-document SHA-256 is
`b5936885ee3cddaeaf0b21590a0657a73f8d9b1dc1b26fd1b2fbcd2afa043f25`.
The socket and per-connection service unit SHA-256 values are respectively
`336f2ca703ab95b4d124d643f9b08b939ec055b11c7f4bb573207f4cb99b4068`
and `0d14b1ba451af698d831cb4fa342ec046d513391569eacbe3c98b8fd9104e3ce`.
The package contains 62 installed data members; the upstream canonical member
inventory SHA-256 is
`888807e4b14dffda75c20e264671d2cfe41437612ec76093618224940a698d70`
and its rendered inventory-document SHA-256 is
`e38d5ddebf516a313033fbdf41e01dc753fab78580a9119f08c388a69c17ac32`.

The package-installed `rp1-gpclk-route-manager-v1` executor is consumed only
over `/run/rp1-gpclk-dkms/route-manager.sock`. WsprryPi never reads, edits, or
claims ownership of the DKMS-owned boot block or transaction journals. Closed
schema-version-1 JSON carries no path, overlay, command, shell, or sudo input.
WsprryPi owns application idleness and confirmation; the executor independently
owns fixed-service quiescence, atomic boot mutation, recovery, and reboot.

This exact executor-bearing package completed attributable output-inhibited
validation for GPIO4, GPIO20, and restored GPIO4 on a Raspberry Pi 5. The
evidence archive SHA-256 is
`af4bb75d7d747a6e9bab067c563fba4031db08c1ed1800c3cb4c8c4d2587561e`
and its final per-file manifest SHA-256 is
`0078e69f6886282ce4822bacf03b32056cd47dedf5f0cd3fc6357484c0379a29`.
Both routes are therefore accepted for output-inhibited route management under
this exact package identity. Live output, transmitter behavior, timing,
frequency, spectral, SDR, and RF qualification remain unavailable and do not
transfer from route-management evidence.

The three independently retained route bindings are:

- initial GPIO4 transaction `48ef743c-e127-45e1-9994-901006283a2d`, final
  journal SHA-256
  `b5dc50842151f6719980ec5d7d06a0d12f514074215684929d5eb55dc71b361e`;
- GPIO20 transaction `14470a51-dede-4ebc-badb-8e63d8789a65`, final journal
  SHA-256
  `212177a69d4f8d702fd5d0e6f9c25033adc1178b37814ac3996a7ea2310aa168`;
- restored GPIO4 transaction `7197a0b1-3f69-4bbd-9220-47ac9abc5e2c`, final
  journal SHA-256
  `244b8604293b30912ec79a4b9fd4a4ad8b9caa899657c912542ef01b2dd49d9d`.

Package installation remains route-neutral. It verifies the module, ABI,
overlays, executor, schema, documentation, and disabled systemd units without
selecting a route, loading the module, opening the endpoint, changing GPIO,
rebooting, enabling the socket, or enabling output. Socket enrollment and
activation are a separate explicit consumer policy after a supported backend
is selected; they are not package-install side effects.
