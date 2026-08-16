<?php
$cardTitleId = 'cardTitle';
$cardTitleText = 'Wsprry Pi Maintenance';
require __DIR__ . '/../card_header.php';
?>

            <div
                id="globalToastContainer"
                class="toast-container position-fixed start-50 translate-middle-x p-3">
            </div>

            <div class="card-body tab-content bg-body">
                <section
                    id="maintenanceResult"
                    class="maintenance-result d-none mb-4"
                    role="status"
                    aria-live="polite"
                    aria-atomic="true">
                    <div class="maintenance-result__label" id="maintenanceResultLabel">Maintenance update</div>
                    <div class="maintenance-result__title" id="maintenanceResultTitle"></div>
                    <p class="maintenance-result__body mb-0" id="maintenanceResultBody"></p>
                </section>

                <section class="maintenance-recovery" aria-label="Recovery actions">
                    <div class="maintenance-recovery__grid">
                        <section class="maintenance-pane maintenance-pane--primary">
                            <h3 class="maintenance-pane__title h5 mb-0">Repair configuration</h3>
                            <p class="maintenance-pane__body mb-0">
                                Check the current configuration for missing or invalid values and repair what can be repaired while preserving usable settings.
                            </p>
                            <dl class="maintenance-fact-list">
                                <div class="maintenance-fact">
                                    <dt>Keeps</dt>
                                    <dd>Existing values whenever they are still valid.</dd>
                                </div>
                                <div class="maintenance-fact">
                                    <dt>Use when</dt>
                                    <dd>The transmitter worked before and only needs cleanup.</dd>
                                </div>
                                <div class="maintenance-fact">
                                    <dt>Next</dt>
                                    <dd>Setup reloads so you can confirm repaired values before transmitting.</dd>
                                </div>
                            </dl>
                            <div class="maintenance-action maintenance-action--start">
                                <button
                                    id="repairConfigButton"
                                    type="button"
                                    class="btn btn-warning">
                                    Repair current configuration
                                </button>
                            </div>
                        </section>

                        <section class="maintenance-pane maintenance-pane--danger">
                            <h3 class="maintenance-pane__title h5 mb-0">Reset to stock defaults</h3>
                            <p class="maintenance-pane__body mb-0">
                                Replace the current configuration with the stock baseline when the existing settings are no longer trustworthy.
                            </p>
                            <dl class="maintenance-fact-list maintenance-fact-list--danger">
                                <div class="maintenance-fact">
                                    <dt>Replaces</dt>
                                    <dd>The current configuration with stock defaults.</dd>
                                </div>
                                <div class="maintenance-fact">
                                    <dt>Use when</dt>
                                    <dd>You need a clean baseline instead of trying to salvage the current values.</dd>
                                </div>
                                <div class="maintenance-fact">
                                    <dt>Next</dt>
                                    <dd>Review and re-save station, mode, and hardware settings in Setup.</dd>
                                </div>
                            </dl>
                            <div class="maintenance-action maintenance-action--start">
                                <button
                                    id="restoreConfigButton"
                                    type="button"
                                    class="btn btn-danger">
                                    Reset to defaults
                                </button>
                            </div>
                        </section>
                    </div>
                </section>

                <section class="maintenance-utility" aria-label="Maintenance utilities">
                    <div class="maintenance-utility__grid">
                        <section class="maintenance-pane maintenance-pane--utility" aria-labelledby="maintenanceUtilityTitle">
                            <h2 id="maintenanceUtilityTitle" class="maintenance-section-title h5 mb-0">Transmit test tone</h2>
                            <p class="maintenance-pane__body mb-0">
                                Test tone opens the manual tone dialog so you can start or stop a quick output-path check and then return to normal scheduling.
                            </p>
                            <div class="maintenance-action maintenance-action--start">
                                <button
                                    id="test_tone"
                                    type="button"
                                    class="btn btn-outline-warning"
                                    data-bs-toggle="tooltip"
                                    title="Click to generate a test tone">
                                    Test tone
                                </button>
                            </div>
                        </section>

                        <section
                            id="updateCheckPanel"
                            class="maintenance-pane maintenance-pane--update"
                            aria-labelledby="updateCheckPanelTitle">
                            <h2 id="updateCheckPanelTitle" class="maintenance-section-title h5 mb-0">Checking update status</h2>
                            <div
                                id="updateCheckStatus"
                                class="maintenance-update-status visually-hidden"
                                role="status"
                                aria-live="polite"
                                aria-atomic="true">
                                Waiting for version data.
                            </div>
                            <details id="updateCheckTechnical" class="maintenance-update-technical d-none">
                                <summary id="updateCheckTechnicalSummary">Technical details ▼</summary>
                                <dl id="updateCheckTechnicalList" class="maintenance-update-technical-list"></dl>
                            </details>
                            <div id="updateCheckAction" class="maintenance-update-action d-none"></div>
                            <div class="maintenance-action maintenance-action--start maintenance-action--wrap">
                                <button
                                    id="updateCheckNowBtn"
                                    type="button"
                                    class="btn btn-outline-primary">
                                    Check now
                                </button>
                                <button
                                    id="updateCheckToggleBtn"
                                    type="button"
                                    class="btn btn-outline-secondary">
                                    Disable update checks
                                </button>
                            </div>
                        </section>

                        <section
                            id="supportBundlePanel"
                            class="maintenance-pane maintenance-pane--support"
                            aria-labelledby="supportBundlePanelTitle">
                            <h2 id="supportBundlePanelTitle" class="maintenance-section-title h5 mb-0">Support Bundle</h2>
                            <p class="maintenance-pane__body mb-0">
                                Collect a diagnostic archive to review and attach to the relevant GitHub issue. Bundles can contain sensitive information.
                            </p>
                            <div
                                id="supportBundleStatus"
                                class="maintenance-support-status visually-hidden"
                                role="status"
                                aria-live="polite"
                                aria-atomic="true">
                            </div>
                            <div
                                id="supportBundleAlert"
                                class="alert alert-danger d-none mb-0"
                                role="alert"
                                aria-live="assertive"
                                aria-atomic="true">
                            </div>
                            <div class="maintenance-action maintenance-action--start maintenance-action--wrap">
                                <button id="createSupportBundleButton" type="button" class="btn btn-primary">
                                    Create Support Bundle
                                </button>
                                <button id="downloadSupportBundleButton" type="button" class="btn btn-outline-primary d-none" disabled>
                                    Download support bundle
                                </button>
                                <button id="deleteSupportBundleButton" type="button" class="btn btn-outline-danger d-none" disabled>
                                    Delete from Pi
                                </button>
                            </div>
                        </section>
                    </div>
                </section>
            </div>

            <div id="maintenanceOverlay" class="maintenance-overlay d-none"></div>

            <div
                class="modal fade"
                id="supportBundleModal"
                tabindex="-1"
                aria-labelledby="supportBundleModalLabel"
                aria-describedby="supportBundleModalDescription"
                aria-hidden="true">
                <div class="modal-dialog modal-dialog-centered">
                    <div class="modal-content">
                        <div class="modal-header">
                            <h3 class="modal-title h5" id="supportBundleModalLabel">Create Support Bundle</h3>
                            <button type="button" class="btn-close" data-bs-dismiss="modal" aria-label="Close"></button>
                        </div>
                        <div class="modal-body">
                            <p id="supportBundleModalDescription">
                                Wsprry Pi will collect diagnostic information into an archive. Support bundles can contain sensitive information, so review the archive before sharing it.
                            </p>
                            <p class="mb-3">
                                Passive I²C information is included in normal collection.
                            </p>
                            <div class="form-check">
                                <input
                                    id="supportBundleProbeI2c"
                                    class="form-check-input"
                                    type="checkbox"
                                    aria-describedby="supportBundleProbeI2cHelp">
                                <label class="form-check-label" for="supportBundleProbeI2c">
                                    Actively probe I²C bus 1
                                </label>
                                <div id="supportBundleProbeI2cHelp" class="form-text">
                                    Enabling this runs <code>i2cdetect -y 1</code> and actively probes I²C bus 1.
                                </div>
                            </div>
                        </div>
                        <div class="modal-footer">
                            <button type="button" class="btn btn-secondary" data-bs-dismiss="modal">Cancel</button>
                            <button id="confirmCreateSupportBundleButton" type="button" class="btn btn-primary">Create Support Bundle</button>
                        </div>
                    </div>
                </div>
            </div>

            <div
                class="modal fade"
                id="testToneModal"
                tabindex="-1"
                aria-labelledby="testToneModalLabel"
                aria-hidden="true">
                <div class="modal-dialog modal-dialog-centered">
                    <div class="modal-content">
                        <div class="modal-header">
                            <h3 class="modal-title h5" id="testToneModalLabel">Test Tone</h3>
                            <button
                                type="button"
                                class="btn-close"
                                data-bs-dismiss="modal"
                                aria-label="Close"></button>
                        </div>
                        <div class="modal-body">
                            <p class="mb-2">Use the controls below to start or stop the test tone.</p>
                            <p id="testToneFrequencyContext" class="mb-0">
                                Configured frequency: unavailable.
                            </p>
                            <fieldset class="mt-3 mb-0">
                                <legend class="form-label mb-2">Frequency source</legend>
                                <div class="d-flex flex-wrap gap-3">
                                    <div class="form-check">
                                        <input
                                            class="form-check-input"
                                            type="radio"
                                            name="testToneFrequencySource"
                                            id="testToneSourceBand"
                                            value="wspr_band">
                                        <label class="form-check-label" for="testToneSourceBand">
                                            WSPR band
                                        </label>
                                    </div>
                                    <div class="form-check">
                                        <input
                                            class="form-check-input"
                                            type="radio"
                                            name="testToneFrequencySource"
                                            id="testToneSourceCustom"
                                            value="custom_rf">
                                        <label class="form-check-label" for="testToneSourceCustom">
                                            Custom RF frequency
                                        </label>
                                    </div>
                                </div>
                            </fieldset>
                            <div class="mt-3">
                                <label for="testToneBand" class="form-label">WSPR band</label>
                                <select id="testToneBand" class="form-select" disabled>
                                    <option value="">Select a WSPR band</option>
                                </select>
                            </div>
                            <div class="mt-3">
                                <label for="testToneFrequencyHz" class="form-label">
                                    Custom RF frequency, Hz
                                    <span class="visually-hidden">Test tone transmit frequency, Hz.</span>
                                </label>
                                <input
                                    type="number"
                                    id="testToneFrequencyHz"
                                    class="form-control"
                                    inputmode="numeric"
                                    min="1"
                                    step="1">
                            </div>
                            <p id="testToneSelectionPreview" class="small mb-0 mt-3" aria-live="polite">
                                Select a frequency source.
                            </p>
                            <p id="testToneSelectionError" class="small text-danger mb-0 mt-2" role="alert"></p>
                            <p
                                id="testToneExecutionResult"
                                class="small mb-0 mt-2"
                                role="status"
                                aria-live="polite"
                                aria-atomic="true"></p>
                        </div>
                        <div class="modal-footer">
                            <button
                                type="button"
                                id="testToneStart"
                                class="btn btn-primary">
                                Start
                            </button>
                            <button
                                type="button"
                                id="testToneEnd"
                                class="btn btn-danger">
                                End
                            </button>
                            <button
                                type="button"
                                id="testToneClose"
                                class="btn btn-secondary"
                                data-bs-dismiss="modal">
                                Close
                            </button>
                        </div>
                    </div>
                </div>
            </div>
