const overlay = document.getElementById("maintenanceOverlay");

function lockUI() {
    overlay.classList.remove("d-none");
}

function unlockUI() {
    overlay.classList.add("d-none");
}

document.addEventListener("DOMContentLoaded", () => {
    bindTestToneControls();

    const repairButton = document.getElementById("repairConfigButton");
    const restoreButton = document.getElementById("restoreConfigButton");
    const globalToastContainer =
        document.getElementById("globalToastContainer");
    const resultPanel = document.getElementById("maintenanceResult");
    const resultTitle = document.getElementById("maintenanceResultTitle");
    const resultBody = document.getElementById("maintenanceResultBody");

    /**
     * Removes any existing toasts from the global container.
     *
     * @returns {void}
     */
    function clearToasts() {
        globalToastContainer.replaceChildren();
    }

    /**
     * Builds and shows a Bootstrap toast in the global container.
     *
     * The toast includes a centered OK button and auto-hides after
     * 10 seconds.
     *
     * @param {string} message Toast message text
     * @param {string} type Bootstrap semantic type
     * @returns {void}
     */
    function showToast(message, type) {
        clearToasts();

        const typeMap = {
            success: "border-success text-success",
            danger: "border-danger text-danger",
            warning: "border-warning text-warning",
            info: "border-info text-info"
        };

        const classes = typeMap[type] || typeMap.info;

        const toastElement = document.createElement("div");
        toastElement.className = `toast ${classes} border shadow-sm bg-body text-body`;
        toastElement.setAttribute("role", "alert");
        toastElement.setAttribute("aria-live", "assertive");
        toastElement.setAttribute("aria-atomic", "true");

        const toastBody = document.createElement("div");
        toastBody.className = "toast-body text-center";

        const messageBlock = document.createElement("div");
        messageBlock.className = "mb-3";
        messageBlock.textContent = message;

        const actionRow = document.createElement("div");
        actionRow.className = "d-flex justify-content-center";

        const okButton = document.createElement("button");
        okButton.type = "button";
        okButton.className = "btn btn-outline-secondary maintenance-toast-ok";
        okButton.textContent = "OK";

        actionRow.appendChild(okButton);
        toastBody.append(messageBlock, actionRow);
        toastElement.appendChild(toastBody);
        globalToastContainer.appendChild(toastElement);

        const toast = new bootstrap.Toast(toastElement, {
            autohide: true,
            delay: 10000
        });

        okButton.addEventListener("click", () => {
            toast.hide();
        });

        toastElement.addEventListener("hidden.bs.toast", () => {
            toastElement.remove();
            unlockUI();
        });

        lockUI();
        toast.show();
    }

    /**
     * Sets both maintenance buttons enabled or disabled.
     *
     * @param {boolean} disabled True to disable buttons
     * @returns {void}
     */
    function setButtonsDisabled(disabled) {
        repairButton.disabled = disabled;
        restoreButton.disabled = disabled;
    }

    function showResult(state, title, body) {
        resultPanel.dataset.state = state;
        resultTitle.textContent = title;
        resultBody.textContent = body;
        resultPanel.classList.remove("d-none");
    }

    /**
     * Sets a temporary working label on the active button.
     *
     * @param {HTMLButtonElement} button Button being acted on
     * @param {string} text Temporary text
     * @returns {void}
     */
    function setButtonWorkingState(button, text) {
        button.dataset.originalText = button.textContent;
        button.textContent = text;
    }

    /**
     * Restores the original label on the provided button.
     *
     * @param {HTMLButtonElement} button Button to restore
     * @returns {void}
     */
    function restoreButtonText(button) {
        if (button.dataset.originalText) {
            button.textContent = button.dataset.originalText;
            delete button.dataset.originalText;
        }
    }

    /**
     * Attempts to reload the current configuration from the backend.
     *
     * @returns {Promise<boolean>} True if reload succeeded
     */
    async function refreshConfig() {
        try {
            const response = await fetchWithEndpointFallback(SETTINGS_ENDPOINT, {
                method: "GET",
                cache: "no-store"
            });

            if (!response.ok) {
                return false;
            }

            await response.json();
            return true;
        } catch {
            return false;
        }
    }

    /**
     * Sends a repair or restore request to the backend.
     *
     * @param {string} verb Requested backend verb
     * @param {HTMLButtonElement} button Button that initiated the request
     * @returns {Promise<void>}
     */
    async function postRepairVerb(verb, button) {
        const isRepair = verb === "repair";

        const workingText = isRepair
            ? "Repairing Configuration..."
            : "Restoring to Stock...";
        const successTitle = isRepair
            ? "Configuration repaired"
            : "Configuration reset to stock defaults";
        const successText = isRepair
            ? "Review the Setup page now. Confirm the repaired settings, then save any adjustments you still need."
            : "Review station, transmit mode, and hardware settings now. The stock baseline is loaded, but you still need to save your intended operating values.";
        const warningText = isRepair
            ? "The repair completed, but the updated configuration could not be reloaded automatically. Reload the page and verify the repaired values before transmitting."
            : "The reset completed, but the stock configuration could not be reloaded automatically. Reload the page before making further changes.";
        const fallbackFailureText = isRepair
            ? "Configuration repair failed."
            : "Configuration reset failed.";

        showToast("Processing request...", "info");
        showResult(
            "warning",
            isRepair ? "Repair in progress" : "Reset in progress",
            isRepair
                ? "The UI is checking the configuration and writing repaired values where possible."
                : "The UI is replacing the current configuration with the stock baseline."
        );
        setButtonWorkingState(button, workingText);
        setButtonsDisabled(true);

        try {
            const response = await fetchWithEndpointFallback(REPAIR_ENDPOINT, {
                method: "POST",
                headers: {
                    "Content-Type": "application/json"
                },
                body: JSON.stringify({ verb })
            });

            let data = null;

            try {
                data = await response.json();
            } catch {
                data = null;
            }

            if (!response.ok) {
                const message =
                    data && data.message
                        ? data.message
                        : fallbackFailureText;

                showToast(message, "danger");
                showResult(
                    "danger",
                    isRepair ? "Repair did not complete" : "Reset did not complete",
                    `${message} No further changes were applied. Review the current configuration before trying again.`
                );
                return;
            }

            const configReloaded = await refreshConfig();

            if (configReloaded) {
                showToast(successTitle, "success");
                showResult("success", successTitle, successText);
            } else {
                showToast(
                    `${successTitle}. The updated configuration could not be reloaded.`,
                    "warning"
                );
                showResult("warning", successTitle, warningText);
            }
        } catch {
            showToast(
                "Unable to contact the server for the configuration operation.",
                "danger"
            );
            showResult(
                "danger",
                isRepair ? "Repair could not reach the server" : "Reset could not reach the server",
                "The request did not complete. Leave the current configuration unchanged, verify controller connectivity, then try again."
            );
        } finally {
            restoreButtonText(button);
            setButtonsDisabled(false);
        }
    }

    repairButton.addEventListener("click", () => {
        postRepairVerb("repair", repairButton);
    });

    restoreButton.addEventListener("click", () => {
        if (typeof showConfirmationDialog === "function") {
            showConfirmationDialog({
                title: "Reset configuration",
                message:
                    "Restore configuration to stock defaults? This will replace the current configuration.",
                confirmLabel: "Reset to defaults",
                confirmClass: "btn-danger",
                onConfirm: () => {
                    postRepairVerb("restore", restoreButton);
                }
            });
        } else {
            postRepairVerb("restore", restoreButton);
        }
    });

    const createSupportBundleButton = document.getElementById("createSupportBundleButton");
    const confirmCreateSupportBundleButton = document.getElementById("confirmCreateSupportBundleButton");
    const downloadSupportBundleButton = document.getElementById("downloadSupportBundleButton");
    const deleteSupportBundleButton = document.getElementById("deleteSupportBundleButton");
    const supportBundleProbeI2c = document.getElementById("supportBundleProbeI2c");
    const supportBundleStatus = document.getElementById("supportBundleStatus");
    const supportBundleAlert = document.getElementById("supportBundleAlert");
    const supportBundleModalElement = document.getElementById("supportBundleModal");
    const supportBundleModal = supportBundleModalElement
        ? bootstrap.Modal.getOrCreateInstance(supportBundleModalElement)
        : null;
    const SUPPORT_BUNDLE_POLL_INTERVAL_MS = 2000;
    const SUPPORT_BUNDLE_FILENAME_FALLBACK = "wsprrypi-support-bundle.tar.gz";
    let supportBundleJobId = "";
    let supportBundlePollTimer = null;
    let supportBundleCreateInFlight = false;
    let supportBundleDownloadInFlight = false;
    let supportBundleDeleteInFlight = false;
    let supportBundlePageUnloading = false;
    let supportBundleModalOpener = null;

    function isSupportBundleFocusTarget(element) {
        return element instanceof HTMLElement &&
            element.isConnected &&
            !element.matches(":disabled") &&
            element.getAttribute("aria-disabled") !== "true" &&
            element.tabIndex >= 0;
    }

    function restoreSupportBundleModalFocus() {
        const opener = isSupportBundleFocusTarget(supportBundleModalOpener)
            ? supportBundleModalOpener
            : createSupportBundleButton;
        supportBundleModalOpener = null;

        if (isSupportBundleFocusTarget(opener)) {
            opener.focus({ preventScroll: true });
        }
    }

    function supportBundleEndpoint(suffix = "") {
        return createEndpointDefinition(
            "support bundles",
            `${SUPPORT_BUNDLES_ENDPOINT.proxyUrl}${suffix}`,
            `${SUPPORT_BUNDLES_ENDPOINT.directUrl}${suffix}`
        );
    }

    function stopSupportBundlePolling() {
        if (supportBundlePollTimer !== null) {
            window.clearTimeout(supportBundlePollTimer);
            supportBundlePollTimer = null;
        }
    }

    function setSupportBundleStatus(state, message) {
        supportBundleStatus.dataset.state = state;
        supportBundleStatus.textContent = message;
        supportBundleStatus.classList.remove("visually-hidden");
    }

    function clearSupportBundleAlert() {
        supportBundleAlert.textContent = "";
        supportBundleAlert.classList.add("d-none");
    }

    function showSupportBundleAlert(message) {
        supportBundleAlert.textContent = message;
        supportBundleAlert.classList.remove("d-none");
    }

    function setSupportBundleActions() {
        const hasReadyDownload = supportBundleJobId !== "" &&
            downloadSupportBundleButton.dataset.available === "true";
        createSupportBundleButton.disabled = supportBundleCreateInFlight ||
            supportBundleDownloadInFlight || supportBundleDeleteInFlight ||
            (supportBundleJobId !== "" && supportBundlePollTimer !== null);
        downloadSupportBundleButton.disabled = !hasReadyDownload || supportBundleDownloadInFlight;
        deleteSupportBundleButton.disabled = !hasReadyDownload || supportBundleDeleteInFlight;
    }

    function setDownloadAvailability(available) {
        downloadSupportBundleButton.dataset.available = available ? "true" : "false";
        downloadSupportBundleButton.classList.toggle("d-none", !available);
        if (!available) {
            deleteSupportBundleButton.classList.add("d-none");
        }
        setSupportBundleActions();
    }

    function safeSupportBundleFilename(contentDisposition) {
        if (typeof contentDisposition !== "string") {
            return SUPPORT_BUNDLE_FILENAME_FALLBACK;
        }
        const match = contentDisposition.match(/(?:^|;)\s*filename="?([^";]+)"?/i);
        const filename = match ? match[1].trim() : "";
        if (!/^[A-Za-z0-9][A-Za-z0-9._-]*\.tar\.gz$/i.test(filename)) {
            return SUPPORT_BUNDLE_FILENAME_FALLBACK;
        }
        return filename;
    }

    function genericSupportBundleFailure(action) {
        const messages = {
            create: "Support bundle creation could not start. Check the controller connection and try again.",
            collect: "Support bundle collection did not complete. Try again when the controller is available.",
            status: "Support bundle status could not be checked. Try again shortly.",
            download: "The support bundle could not be downloaded. Try the download again.",
            delete: "The Pi-side support bundle could not be removed. Your downloaded file is safe; the Pi-side copy will expire automatically within 24 hours."
        };
        return messages[action] || "The support bundle operation did not complete. Try again.";
    }

    function scheduleSupportBundlePoll(jobId) {
        stopSupportBundlePolling();
        if (supportBundlePageUnloading || jobId !== supportBundleJobId) {
            return;
        }
        supportBundlePollTimer = window.setTimeout(() => {
            supportBundlePollTimer = null;
            pollSupportBundle(jobId);
        }, SUPPORT_BUNDLE_POLL_INTERVAL_MS);
        setSupportBundleActions();
    }

    async function pollSupportBundle(jobId) {
        if (supportBundlePageUnloading || jobId !== supportBundleJobId) {
            return;
        }
        try {
            const response = await fetchWithEndpointFallback(
                supportBundleEndpoint(`/${encodeURIComponent(jobId)}`),
                { method: "GET", cache: "no-store" }
            );
            if (!response.ok) {
                throw new Error("status request failed");
            }
            const snapshot = await response.json();
            if (!snapshot || jobId !== supportBundleJobId) {
                return;
            }
            if (snapshot.state === "queued") {
                setSupportBundleStatus("queued", "Support bundle queued.");
                scheduleSupportBundlePoll(jobId);
                return;
            }
            if (snapshot.state === "running") {
                setSupportBundleStatus("running", "Collecting diagnostic information.");
                scheduleSupportBundlePoll(jobId);
                return;
            }
            stopSupportBundlePolling();
            if (snapshot.state === "succeeded" && snapshot.download_available === true) {
                setSupportBundleStatus("ready", "Support bundle ready to download.");
                setDownloadAvailability(true);
                return;
            }
            setDownloadAvailability(false);
            setSupportBundleStatus("failed", "Support bundle collection did not complete.");
            showSupportBundleAlert(genericSupportBundleFailure("collect"));
            supportBundleJobId = "";
            setSupportBundleActions();
        } catch {
            stopSupportBundlePolling();
            setDownloadAvailability(false);
            setSupportBundleStatus("failed", "Support bundle status unavailable.");
            showSupportBundleAlert(genericSupportBundleFailure("status"));
            supportBundleJobId = "";
            setSupportBundleActions();
        }
    }

    async function createSupportBundle() {
        if (supportBundleCreateInFlight || supportBundleJobId !== "") {
            return;
        }
        supportBundleCreateInFlight = true;
        clearSupportBundleAlert();
        setSupportBundleActions();
        try {
            const response = await fetchWithEndpointFallback(SUPPORT_BUNDLES_ENDPOINT, {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify({ probe_i2c: supportBundleProbeI2c.checked })
            });
            if (!response.ok) {
                throw new Error("create request failed");
            }
            const snapshot = await response.json();
            if (!snapshot || typeof snapshot.id !== "string" || snapshot.id === "") {
                throw new Error("missing job id");
            }
            supportBundleJobId = snapshot.id;
            setDownloadAvailability(false);
            supportBundleModal.hide();
            setSupportBundleStatus("queued", "Support bundle queued.");
            scheduleSupportBundlePoll(supportBundleJobId);
        } catch {
            setSupportBundleStatus("failed", "Support bundle was not created.");
            showSupportBundleAlert(genericSupportBundleFailure("create"));
        } finally {
            supportBundleCreateInFlight = false;
            setSupportBundleActions();
        }
    }

    function invokeBrowserDownload(blob, filename) {
        const objectUrl = URL.createObjectURL(blob);
        const link = document.createElement("a");
        link.href = objectUrl;
        link.download = filename;
        link.style.display = "none";
        document.body.appendChild(link);
        try {
            link.click();
        } finally {
            link.remove();
            window.setTimeout(() => URL.revokeObjectURL(objectUrl), 0);
        }
    }

    async function deleteSupportBundle(jobId, afterDownload = false, filename = "") {
        if (supportBundleDeleteInFlight || jobId !== supportBundleJobId) {
            return false;
        }
        supportBundleDeleteInFlight = true;
        setSupportBundleActions();
        try {
            const response = await fetchWithEndpointFallback(
                supportBundleEndpoint(`/${encodeURIComponent(jobId)}`),
                { method: "DELETE" }
            );
            if (!response.ok) {
                throw new Error("delete request failed");
            }
            setDownloadAvailability(false);
            deleteSupportBundleButton.classList.add("d-none");
            supportBundleJobId = "";
            setSupportBundleStatus(
                "deleted",
                afterDownload
                    ? `Downloaded: ${filename}. Your browser chose the save location. Review the archive for sensitive information, then attach it to the relevant GitHub issue.`
                    : "The Pi-side support bundle was deleted."
            );
            return true;
        } catch {
            setDownloadAvailability(true);
            deleteSupportBundleButton.classList.remove("d-none");
            setSupportBundleStatus(
                "cleanup-pending",
                afterDownload
                    ? `Downloaded: ${filename}. Your browser chose the save location. The downloaded file is safe; the Pi-side copy will expire automatically within 24 hours.`
                    : "The Pi-side support bundle is still retained."
            );
            showSupportBundleAlert(genericSupportBundleFailure("delete"));
            return false;
        } finally {
            supportBundleDeleteInFlight = false;
            setSupportBundleActions();
        }
    }

    async function downloadSupportBundle() {
        if (supportBundleDownloadInFlight || supportBundleJobId === "" ||
            downloadSupportBundleButton.dataset.available !== "true") {
            return;
        }
        const jobId = supportBundleJobId;
        supportBundleDownloadInFlight = true;
        clearSupportBundleAlert();
        setSupportBundleStatus("downloading", "Downloading support bundle.");
        setSupportBundleActions();
        try {
            const response = await fetchWithEndpointFallback(
                supportBundleEndpoint(`/${encodeURIComponent(jobId)}/download`),
                { method: "GET", cache: "no-store" }
            );
            if (!response.ok) {
                throw new Error("download request failed");
            }
            const blob = await response.blob();
            if (blob.size === 0 || jobId !== supportBundleJobId) {
                throw new Error("incomplete download");
            }
            const filename = safeSupportBundleFilename(response.headers.get("Content-Disposition"));
            invokeBrowserDownload(blob, filename);
            await deleteSupportBundle(jobId, true, filename);
        } catch {
            setSupportBundleStatus("failed", "Support bundle download did not complete.");
            showSupportBundleAlert(genericSupportBundleFailure("download"));
        } finally {
            supportBundleDownloadInFlight = false;
            setSupportBundleActions();
        }
    }

    supportBundleModalElement?.addEventListener("hidden.bs.modal", restoreSupportBundleModalFocus);

    createSupportBundleButton.addEventListener("click", (event) => {
        if (!supportBundleCreateInFlight && supportBundleJobId === "") {
            clearSupportBundleAlert();
            supportBundleModalOpener = event.currentTarget;
            supportBundleModal.show();
        }
    });
    confirmCreateSupportBundleButton.addEventListener("click", createSupportBundle);
    downloadSupportBundleButton.addEventListener("click", downloadSupportBundle);
    deleteSupportBundleButton.addEventListener("click", () => {
        deleteSupportBundle(supportBundleJobId);
    });
    window.addEventListener("pagehide", () => {
        supportBundlePageUnloading = true;
        stopSupportBundlePolling();
    }, { once: true });
    setDownloadAvailability(false);
});
