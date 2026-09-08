<?php // SPDX-License-Identifier: MIT ?>
<div class="wtp-development mt-3">
    <div class="form-check form-switch">
        <input class="form-check-input" type="checkbox" role="switch" id="wtp_visible" aria-controls="wtp-controls" aria-describedby="wtp-visibility-hint">
        <label class="form-check-label" for="wtp_visible">Show Pico development controls</label>
    </div>
    <p class="form-text mb-0" id="wtp-visibility-hint">Browser-only preference. Hiding controls keeps the saved output selection.</p>
    <p id="wtp-hidden-selection" class="form-text mt-2 mb-0" hidden>Pico is selected. Enable development controls to edit its settings or reconcile its state.</p>
    <section id="wtp-controls" class="transmitter-backend-fields" aria-labelledby="wtp-heading" hidden>
        <h3 id="wtp-heading" class="transmitter-backend-fields__title">Pico output</h3>
        <div class="form-check form-switch mb-3">
            <input class="form-check-input" type="checkbox" role="switch" id="wtp_use" aria-controls="wtp-settings" aria-describedby="wtp-selection-hint">
            <label class="form-check-label" for="wtp_use">Use Pico over USB</label>
        </div>
        <p class="form-text" id="wtp-selection-hint">Uses the dedicated WTP interface. Disable TX LED, amplifier, shutdown-button and band GPIO controls before selecting Pico. Host GPIO calibration and CW fades do not apply.</p>
        <div id="wtp-settings" class="row gx-3 gy-3">
            <div class="col-12 col-lg-6 config-stacked-field">
                <label class="form-label" for="wtp_endpoint">Endpoint path</label>
                <input class="form-control" id="wtp_endpoint" type="text" maxlength="512" placeholder="/dev/serial/by-id/…-if02" data-wtp-key="Endpoint" aria-describedby="wtp-endpoint-hint" disabled>
                <div class="form-text" id="wtp-endpoint-hint"></div>
            </div>
            <div class="col-12 col-lg-6 config-stacked-field">
                <label class="form-label" for="wtp_serial">USB serial</label>
                <input class="form-control" id="wtp_serial" type="text" maxlength="128" data-wtp-key="USB Serial" aria-describedby="wtp-serial-hint" disabled>
                <div class="form-text" id="wtp-serial-hint"></div>
            </div>
            <div class="col-12 col-lg-6 config-stacked-field">
                <label class="form-label" for="wtp_device">Device identity</label>
                <input class="form-control" id="wtp_device" type="text" maxlength="32" pattern="[0-9a-f]{32}" data-wtp-key="Device ID" aria-describedby="wtp-device-hint" disabled>
                <div class="form-text" id="wtp-device-hint">32 lowercase hexadecimal characters from Pico HELLO.</div>
            </div>
            <div class="col-12 col-lg-6 config-stacked-field">
                <label class="form-label" for="wtp_vendor">USB vendor ID</label>
                <input class="form-control" id="wtp_vendor" type="number" min="1" max="65535" step="1" data-wtp-key="USB Vendor ID" aria-describedby="wtp-vendor-hint" disabled>
                <div class="form-text" id="wtp-vendor-hint">Decimal USB identifier.</div>
            </div>
            <div class="col-12 col-lg-6 config-stacked-field">
                <label class="form-label" for="wtp_product">USB product ID</label>
                <input class="form-control" id="wtp_product" type="number" min="1" max="65535" step="1" data-wtp-key="USB Product ID" aria-describedby="wtp-product-hint" disabled>
                <div class="form-text" id="wtp-product-hint">Decimal USB identifier.</div>
            </div>
            <div class="col-12 col-lg-6 config-stacked-field">
                <label class="form-label" for="wtp_uncertainty">Maximum start uncertainty (ns)</label>
                <input class="form-control" id="wtp_uncertainty" type="number" min="1" max="1000000000" step="1" value="1000000" data-wtp-key="Start Uncertainty ns" aria-describedby="wtp-uncertainty-hint" disabled>
                <div class="form-text" id="wtp-uncertainty-hint">1,000,000 ns = 1 ms. Device clock evidence must meet this limit.</div>
            </div>
            <div class="col-12">
                <div class="form-check">
                    <input class="form-check-input" type="checkbox" id="wtp_adjust" data-wtp-key="Allow Frequency Adjustment" disabled>
                    <label class="form-check-label" for="wtp_adjust">Allow device frequency rounding</label>
                </div>
                <p class="form-text mb-0">Accepts the device's reported realizable frequencies. This does not establish RF accuracy.</p>
            </div>
        </div>
        <p class="form-text mt-3">The host needs synchronized UTC. The Pico needs its own valid UTC clock with bounded uncertainty; USB does not set that clock. GPSDO frequency discipline alone is not UTC.</p>
        <div class="wtp-status" aria-labelledby="wtp-status-heading">
            <h4 id="wtp-status-heading" class="cw-control-section__title">Pico status</h4>
            <p id="wtp-status-text" class="mb-2" role="status" aria-live="polite">Status has not been checked.</p>
            <dl class="wtp-observations">
                <div><dt>Output observed</dt><dd id="wtp-output">Unknown</dd></div>
                <div><dt>Host UTC</dt><dd id="wtp-clock">Unknown</dd></div>
                <div><dt>Device / boot</dt><dd id="wtp-identity">Unknown</dd></div>
                <div><dt>Last job</dt><dd id="wtp-history">None</dd></div>
            </dl>
            <button type="button" class="btn btn-primary" id="wtp-recover" aria-describedby="wtp-recover-hint" disabled>Reconcile Pico</button>
            <p class="form-text mt-2 mb-0" id="wtp-recover-hint">Checks an idle device, or reconnects the current session to resolve its owned job. Recovery may stop that job. It never resumes a transmission or takes another owner's job.</p>
            <p id="wtp-feedback" class="form-text mt-2 mb-0" role="status" aria-live="polite"></p>
        </div>
    </section>
</div>
