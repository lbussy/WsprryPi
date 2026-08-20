# Bounded Qualification Tone Product Slice - Execution Prompt

Implement the product-owned half of bounded qualification Tone control in
WsprryPi. Add a distinct `bounded_tone` transaction accepted only through an
explicitly loopback-bound WebSocket server. Require a caller-supplied request
ID, an exact positive duration in milliseconds with a conservative product cap,
and the existing validated frequency selector. Start through the normal Test
Tone path; arm a monotonic, product-owned watchdog before acknowledging success;
stop internally at the deadline; emit request-correlated terminal status; and
fail closed on malformed input, duplicate or overlapping requests, startup
failure, shutdown, cancellation, or watchdog setup failure.

Preserve existing `tone_start`, `tone_end`, and the normal remote UI default.
Add hardware-free parser, bind, lifecycle, and cleanup coverage plus concise
developer documentation. Do not change the UI, operator-docs repository,
Qualification Harness, service configuration, installed software, GPIO, SDR,
or RF. Run only non-hardware validation, adversarially review races and cleanup,
correct findings, then commit and push the attributable WsprryPi changes if the
tree and validation are clean.

Exit when the product primitive is source-reviewed, reproducible in non-hardware
tests, opt-in and loopback-contained, internally time-bounded, and ready for a
separately reviewed harness-helper integration slice. Do not claim physical
timing or RF qualification.
