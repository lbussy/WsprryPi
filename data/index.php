<!DOCTYPE html>
<html lang="en">

<!-- TODO:
    - Get -24 hours from:
        https://www.wsprnet.org/olddb?mode=html&band=all&limit=50&findcall=AA0NT&findreporter=&sort=date
-->

<head>
    <meta charset="utf-8">
    <title>Wsprry Pi</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="apple-touch-icon" sizes="180x180" href="apple-touch-icon.png">
    <link rel="icon" type="image/png" sizes="32x32" href="favicon-32x32.png">
    <link rel="icon" type="image/png" sizes="16x16" href="favicon-16x16.png">
    <link rel="manifest" href="site.webmanifest">
    <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/css/bootstrap.min.css" rel="stylesheet" integrity="sha384-QWTKZyjpPEjISv5WaRU9OFeRpok6YctnYmDr5pNlyT2bRjXh0JMhjY6hW+ALEwIH" crossorigin="anonymous">
    <script
        src="https://code.jquery.com/jquery-3.7.1.min.js"
        integrity="sha256-/JqT3SQfawRcv/BIHPThkBvs0OEvtFFmqPF/lYI/Cxo="
        crossorigin="anonymous">
    </script>
    <style>
        body {
            font-family: 'Open Sans', sans-serif;
        }
        /* Custom Navbar Styling */
        .custom-navbar {
            position: fixed;  /* Fix to top */
            top: 0;
            width: 100%;
            margin: 0;
            padding: 0;
            border-bottom: 2px solid #343a40; /* Dark border */
            z-index: 1030; /* Ensures it's above other content */
        }

        /* Adjust card body padding */
        .custom-navbar .card-body {
            padding: 0.5rem 1rem;
        }

        /* Navbar Logo Styling */
        .navbar-logo {
            width: 30px;
            height: 30px;
        }

        /* Navbar Link Styling */
        .navbar-nav .nav-link {
            transition: color 0.3s ease-in-out;
        }

        /* Keep some space between card and header/footer */
        body {
            padding-top: 120px;
            padding-bottom: 80px;
        }

        /* Style the Font Awesome buttons in the control box */
        .btn-fafa {
            border: none;
            outline: none;
            background: none;
            cursor: pointer;
            padding: 0;
            text-decoration: none;
            font-family: inherit;
            font-size: inherit;
            
            /* Use the same color as .custom-muted */
            color: inherit !important;
        }

        .btn-fafa:hover {
            font-weight: bold;
            transform: scale(1.1);
            transition: transform 0.2s ease-in-out;
        }

        /* Muted text for card header */
        .custom-muted {
            color: #5a6268 !important;
            font-weight: normal;
            opacity: 0.85;
        }

        /* Customize the tooltip */
        .custom-tooltip .tooltip-inner {
            background-color: #5a189a; /* Custom background color */
            color: #ffffff; /* White text */
            font-size: 16px;
            font-weight: bold;
            padding: 10px 15px;
            border-radius: 8px;
            box-shadow: 0px 4px 10px rgba(0, 0, 0, 0.2);
        }

        /* Custom arrow color */
        .custom-tooltip.bs-tooltip-top .tooltip-arrow::before,
        .custom-tooltip.bs-tooltip-bottom .tooltip-arrow::before,
        .custom-tooltip.bs-tooltip-start .tooltip-arrow::before,
        .custom-tooltip.bs-tooltip-end .tooltip-arrow::before {
            background-color: #5a189a;
        }

        /* Ensure validation messages always take up space */
        #ppm ~ .valid-feedback,
        #ppm ~ .invalid-feedback {
            min-height: 1.5em; /* Adjust as needed */
            display: block; /* Ensures space is always there */
            visibility: hidden; /* Hides it but keeps the space */
        }

        /* Show feedback messages when validation is active */
        #ppm.is-valid ~ .valid-feedback {
            visibility: visible;
        }

        #ppm.is-invalid ~ .invalid-feedback {
            visibility: visible;
        }

        /* Ensure validation messages always take up space */
        #use_ntp ~ .valid-feedback,
        #use_ntp ~ .invalid-feedback {
            min-height: 1.5em; /* Adjust as needed */
            display: block; /* Ensures space is always there */
            visibility: hidden; /* Hides it but keeps the space */
        }

        /* Match Bootstrap's "is-valid" green color */
        #power_level {
            accent-color: #198754; /* Works for modern browsers */
        }

        /* Fallback for older browsers */
        #power_level::-webkit-slider-thumb {
            background-color: #198754; /* Green */
            border: 2px solid #145c32; /* Darker green border */
        }

        #power_level::-moz-range-thumb {
            background-color: #198754;
            border: 2px solid #145c32;
        }

        #power_level::-ms-thumb {
            background-color: #198754;
            border: 2px solid #145c32;
        }

        /* Reduce spacing in the footer */
        #footer {
            font-size: 0.85rem; /* Smaller text */
            line-height: 1.2; /* Reduce line spacing */
        }

        /* Reduce padding for compact height */
        #footer .container {
            padding-top: 4px;
            padding-bottom: 4px;
        }
    </style>

</head>

<body>
    <div class="card bg-dark text-white border-dark fixed-top">
        <div class="card-body p-2"> 
            <nav class="navbar navbar-expand-lg navbar-dark">
                <div class="container">
                    <!-- Logo & Brand -->
                    <a href="<?php echo htmlspecialchars($_SERVER['PHP_SELF']); ?>" class="navbar-brand">
                        <img src="antenna.svg" style="width:30px;height:30px;" alt="Logo" class="me-2" />
                        Wsprry Pi
                    </a>

                    <!-- Navbar Toggler (for mobile) -->
                    <button class="navbar-toggler" type="button" data-bs-toggle="collapse" data-bs-target="#navbarResponsive"
                        aria-controls="navbarResponsive" aria-expanded="false" aria-label="Toggle navigation">
                        <span class="navbar-toggler-icon"></span>
                    </button>

                    <!-- Navbar Links -->
                    <div class="collapse navbar-collapse custom-muted" id="navbarResponsive">
                        <ul class="navbar-nav"></ul> <!-- Left-side nav (if needed) -->
                        <ul class="navbar-nav ms-md-auto">
                            <li class="nav-item">
                                <a target="_blank" rel="noopener" class="nav-link text-white"
                                    href="https://github.com/lbussy/WsprryPi/">
                                    <i class="fa-brands fa-github"></i>&nbsp;&nbsp;GitHub
                                </a>
                            </li>
                            <li class="nav-item">
                                <a target="_blank" rel="noopener" class="nav-link text-white"
                                    href="http://wsprdocs.aa0nt.net">
                                    <i class="fa-solid fa-book"></i>&nbsp;&nbsp;Documentation
                                </a>
                            </li>
                        </ul>
                    </div>
                </div>
            </nav>
        </div>
    </div>

    <div class="container">
        <div class="card border-dark mb-3">
            <div class="card-header d-flex custom-muted">
                Connected to: <?php echo gethostname(); ?>
                <span class="ms-auto d-flex">
                    <form action="semaphore.php" method="post">
                        <input type="hidden" name="action" value="reboot">
                        <button type="submit" class="btn-fafa me-2 custom-tooltip" data-bs-toggle="tooltip" title="Reboot">
                            <i class="fa-solid fa-arrows-rotate"></i>
                        </button>
                    </form>
                    <form action="semaphore.php" method="post">
                        <input type="hidden" name="action" value="shutdown">
                        <button type="submit" class="btn-fafa custom-tooltip" data-bs-toggle="tooltip" title="Shutdown">
                            <i class="fa-solid fa-power-off"></i>
                        </button>
                    </form>
                </span>
            </div>
            <div class="card-body">

                <h3 class="card-title">Wsprry Pi Configuration</h3>

                <form id="wsprform">
                    <fieldset id="wsprconfig" class="form-group" disabled="disabled">
                        <!-- First Row -->
                        <legend class="mt-4">Control</legend>
                        <div class="container">
                            <div class="row">
                                <!-- Left Column: Enable Transmission -->
                                <div class="col-md-6 d-flex align-items-center">
                                    <div class="row w-100">
                                        <div class="col-md-4 text-end">
                                            <label class="form-check-label" for="transmit">Enable Transmission:</label>
                                        </div>
                                        <div class="col-md-8">
                                            <div class="form-check form-switch was-validated">
                                                <input class="form-check-input" type="checkbox" id="transmit" data-form-type="other">
                                            </div>
                                        </div>
                                    </div>
                                </div>

                                <!-- Right Column: Enable LED + GPIO Select -->
                                <div class="col-md-6 d-flex align-items-center">
                                    <div class="row w-100">
                                        <div class="col-md-4 text-end">
                                            <label class="form-check-label" for="use_led">Enable LED:</label>
                                        </div>
                                        <div class="col-md-2">
                                            <div class="form-check form-switch was-validated">
                                                <input class="form-check-input" type="checkbox" id="use_led" data-form-type="other">
                                            </div>
                                        </div>
                                        <div class="col-md-2 text-end">
                                            <label class="form-check-label" for="gpio_select">LED Pin:</label>
                                        </div>
                                        <div class="col-md-4 was-validated">
                                            <select id="gpio_select" class="form-select">
                                                <option value="GPIO17">GPIO17 (Pin 11)</option>
                                                <option value="GPIO18">GPIO18 (Pin 12)</option>
                                                <option value="GPIO21">GPIO21 (Pin 13)</option>
                                                <option value="GPIO22">GPIO22 (Pin 15)</option>
                                                <option value="GPIO23">GPIO23 (Pin 16)</option>
                                                <option value="GPIO24">GPIO24 (Pin 18)</option>
                                                <option value="GPIO25">GPIO25 (Pin 22)</option>
                                            </select>
                                        </div>
                                    </div>
                                </div>
                            </div>
                        </div>
                        <!-- First Row -->

                        <!-- Second Row -->
                        <legend class="mt-4">Operator Information</legend>
                        <div class="container">
                            <div class="row">
                                <!-- Call Sign Input -->
                                <div class="col-md-6 d-flex align-items-center">
                                    <div class="row w-100">
                                        <div class="col-md-4 text-end">
                                            <label class="form-check-label" for="callsign">Call Sign:</label>
                                        </div>
                                        <div class="col-md-8 was-validated">
                                            <input type="text" 
                                                pattern="^([A-Za-z]{1,2}[0-9][A-Za-z0-9]{1,3}|[A-Za-z][0-9][A-Za-z]|[0-9][A-Za-z][0-9][A-Za-z0-9]{2,3})$" 
                                                minlength="3" maxlength="6" 
                                                class="form-control" id="callsign" 
                                                placeholder="Enter callsign" required>    
                                            <div class="valid-feedback">Valid.</div>
                                            <div class="invalid-feedback">Please enter your callsign.</div>
                                        </div>
                                    </div>
                                </div>

                                <!-- Grid Square Input -->
                                <div class="col-md-6 d-flex align-items-center">
                                    <div class="row w-100">
                                        <div class="col-md-4 text-end">
                                            <label class="form-check-label" for="gridsquare">Grid Square:</label>
                                        </div>
                                        <div class="col-md-8 was-validated">
                                            <input minlength="4" maxlength="4" 
                                                pattern="[a-z,A-Z]{2}[0-9]{2}" 
                                                type="text" class="form-control" 
                                                id="gridsquare" 
                                                placeholder="Enter grid square" required>
                                            <div class="valid-feedback">Valid.</div>
                                            <div class="invalid-feedback">Please enter your four-character grid square.</div>
                                        </div>
                                    </div>
                                </div>
                            </div>
                        </div>
                        <!-- End Second Row -->

                        <!-- Third Row -->
                        <legend class="mt-4">Station Information</legend>
                        <div class="container">
                            <div class="row">
                                <!-- Transmit Power Input -->
                                <div class="col-md-6 d-flex align-items-center">
                                    <div class="row w-100">
                                        <div class="col-md-4 text-end">
                                            <label class="form-label" for="dbm">Transmit Power<br>(in dBm):</label>
                                        </div>
                                        <div class="col-md-8 was-validated">
                                            <input type="number" min="-10" max="62" step="1" 
                                                class="form-control" id="dbm" 
                                                placeholder="Enter transmit power" required>
                                            <div class="valid-feedback">Valid.</div>
                                            <div class="invalid-feedback">
                                                Please enter your transmit power in dBm (without the 'dBm' suffix.)
                                            </div>
                                        </div>
                                    </div>
                                </div>

                                <!-- Frequency Input -->
                                <div class="col-md-6 d-flex align-items-center">
                                    <div class="row w-100">
                                        <div class="col-md-4 text-end">
                                            <label class="form-label" for="frequencies">Frequency:</label>
                                        </div>
                                        <div class="col-md-8 was-validated">
                                            <input type="text" class="form-control" id="frequencies" 
                                                placeholder="Enter frequency" oninput="checkFreq();">
                                            <div class="valid-feedback">Valid.</div>
                                            <div class="invalid-feedback">
                                                Add a single frequency or a space-delimited list (see 
                                                <a href="https://wsprry-pi.readthedocs.io/en/latest/Operations/index.html" 
                                                target="_blank" rel="noopener noreferrer">documentation</a>).
                                            </div>
                                        </div>
                                    </div>
                                </div>
                            </div>

                            <!-- Additional Row for Random Offset Switch -->
                            <div class="row mt-3">
                                <div class="col-md-6">
                                    <!-- Empty Space for Proper Alignment -->
                                </div>
                                <div class="col-md-6 d-flex align-items-center">
                                    <div class="row w-100">
                                        <div class="col-md-4 text-end">
                                            <label class="form-check-label" for="useoffset">Add Random Offset:</label>
                                        </div>
                                        <div class="col-md-8 was-validated">
                                            <div class="form-check form-switch">
                                                <input class="form-check-input" type="checkbox" id="useoffset" data-form-type="other">
                                            </div>
                                        </div>
                                    </div>
                                </div>
                            </div>
                        </div>
                        <!-- End Third Row -->

                        <!-- Fourth Row -->
                        <legend class="mt-4">Calibration</legend>
                        <div class="container">
                            <div class="row">
                                <!-- Use NTP for Calibration -->
                                <div class="col-md-6 d-flex align-items-top">
                                    <div class="row w-100">
                                        <div class="col-md-5 text-end">
                                            <label class="form-label" for="use_ntp">Use NTP for Calibration:</label>
                                        </div>
                                        <div class="col-md-7 d-flex align-items-top">
                                            <div class="form-check form-switch was-validated">
                                                <input class="form-check-input" type="checkbox" id="use_ntp" data-form-type="other">
                                                <div class="valid-feedback"></div>
                                                <div class="invalid-feedback"></div>
                                            </div>
                                        </div>
                                    </div>
                                </div>

                                <!-- PPM Offset Input -->
                                <div class="col-md-6 d-flex align-items-center">
                                    <div class="row w-100">
                                        <div class="col-md-4 text-end">
                                            <label class="form-label" for="ppm">PPM Offset:</label>
                                        </div>
                                        <div class="col-md-8">
                                            <input type="number" min="-200" max="200" step=".000001" 
                                                class="form-control w-100" id="ppm" 
                                                placeholder="Enter PPM">
                                            <div class="valid-feedback">Valid.</div>
                                            <div class="invalid-feedback">
                                                Enter a positive or negative decimal number for frequency correction.
                                            </div>
                                        </div>
                                    </div>
                                </div>
                            </div>
                        </div>
                        <!-- End Fourth Row -->

                        <!-- Fifth Row -->
                        <legend class="mt-4">Transmit Power</legend>
                        <p>This sets power on the Raspberry Pi GPIO only. Any amplification (such as that which is provided by the TAPR board) must be taken into account.</p>
                        <div class="container">
                            <div class="row justify-content-center">
                                <div class="col-md-8">
                                    <div class="range-wrap d-flex align-items-center">
                                        <input id="power_level" type="range" class="form-range flex-grow-1" min="0" max="7">
                                        <output id="rangeText" class="ms-3"></output>
                                    </div>
                                </div>
                            </div>
                        </div>
                        <p></p>

                        <!-- End Fifth Row -->

                        <!-- Hidden Form Items -->
                        <div id="hidden-row" class="d-none">
                            <input type="text" class="form-control" id="server_port" placeholder="">
                        </div>
                        <!-- End Hidden Form Items -->

                        <!-- Button Row -->
                        <div id="buttons" class="modal-footer justify-content-center">
                            <button id="submit" type="button" class="btn btn-dark me-2">Save</button>
                            <button id="reset" type="button" class="btn btn-light">Reset</button>
                        </div>
                        <!-- End Button Row -->

                    </fieldset>
                </form>
            </div>
        </div>
    </div>
    <footer id="footer" class="bg-dark text-white fixed-bottom w-100">
        `    <div class="container py-2"> <!-- Reduce padding -->
                <div class="row text-center">
                    <div class="col-lg-12">
                        <ul class="list-inline mb-1 small"> <!-- Smaller font size -->
                            <li class="list-inline-item"><a href="http://wsprdocs.aa0nt.net" class="text-white">Docs</a></li>
                            <li class="list-inline-item">|</li>
                            <li class="list-inline-item"><a href="https://github.com/lbussy/WsprryPi" class="text-white">GitHub</a></li>
                            <li class="list-inline-item">|</li>
                            <li class="list-inline-item"><a href="https://tapr.org/" class="text-white">TAPR</a></li>
                            <li class="list-inline-item">|</li>
                            <li class="list-inline-item"><a href="https://www.wsprnet.org/" class="text-white">WSPRNet</a></li>
                        </ul>
                        <p class="mb-1 small">Created by Lee Bussy, AA0NT.</p>
                        <p class="mb-0 small">
                            Original WsprryPi: <a href="https://github.com/lbussy/WsprryPi/blob/main/LICENSE.md" class="text-white">GPL</a> | 
                            New Code & Web UI: <a href="https://github.com/lbussy/WsprryPi/blob/main/LICENSE.md" class="text-white">MIT License</a>
                        </p>
                    </div>
                </div>
            </div>
        </footer>`

    <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/js/bootstrap.bundle.min.js" integrity="sha384-YvpcrYf0tY3lHB60NNkmXc5s9fDVZLESaAA55NDzOxhy9GkcIdslK1eN7N6jIeHz" crossorigin="anonymous"></script>
    <script src="https://kit.fontawesome.com/e51821420e.js" crossorigin="anonymous"></script>

    <script>
        var url = "wspr_ini.php";
        var populateConfigRunning = false;

        $(function() {
            $('[data-bs-toggle="tooltip"]').tooltip()
        })

        var rangeValues = {
            // Define range labels for slider
            "0": "2mA<br />-3.4dBm",
            "1": "4mA<br />2.1dBm",
            "2": "6mA<br />4.9dBm",
            "3": "8mA<br />6.6dBm",
            "4": "10mA<br />8.2dBm",
            "5": "12mA<br />9.2dBm",
            "6": "14mA<br />10.0dBm",
            "7": "16mA<br />10.6dBm"
        };

        $(document).ready(function() {
            bindActions();
            loadPage();
        });

        function bindActions() {
            // Grab Use NTP Switch
            $("#use_ntp").on("click", function() {
                clickUseNTP();
            });

            // Grab Use LED switch (key) {
            $("#use_led").on("click", function() {
                clickUseLED();
            });

            // Grab Submit and Reset Buttons
            $("#submit").click(function() {
                savePage();
            });
            $("#reset").click(function() {
                resetPage();
            });

            // Handle slider move and bubble value
            $('#power_level').on('input change', function() {
                $('#rangeText').html(rangeValues[$(this).val()]);
                const val = parseFloat($('#power_level').val());
                let percentage = (val / 7); // Normalize between 0 and 1
                const minOffset = 0.0095;
                const maxOffset = 0.984;
                const offsetRange = maxOffset - minOffset;
                percentage = ((percentage * offsetRange) + minOffset) * 100;
                $('#rangeText').css("left", percentage + "%");
            });
        }

        function checkFreq() {
            let isValid = true;
            freqString = $("#frequencies").val().toLowerCase();

            // Min length would be one of 2m, 4m, 6m, = 2
            if (freqString.length < 2) isValid = false;

            // Check for alphanumerics, spaces or hypens only
            compareRegEx = "^[a-zA-Z0-9 \-]*$";
            if (!freqString.match(compareRegEx)) isValid = false;

            // Split on whitespace (duplicates are merged)
            freqArray = freqString.split(/\s+/);
            freqArrayLength = freqArray.length;
            for (var i = 0; i < freqArrayLength; i++) {
                freqWord = freqArray[i];
                // Make sure this is not an empty string
                if (!freqArray[i] == " ") {
                    // Check if all numbers (also catches exponents)
                    if (isNumeric(freqWord)) {
                        // Ok
                    }
                    // Check for LF or MF
                    else if (freqWord == "lf" || freqWord == "mf") {
                        // Ok
                    }
                    // Check for "-15" on the end
                    else if (freqWord.endsWith("-15") && !freqWord.endsWith("--15")) {
                        // If removing the "-15" does not yield a number
                        if (isNaN(trimLast(freqWord, 3)) || trimLast(freqWord, 1) < 3) {
                            // See if it indicates a band plan
                            if (!trimLast(freqWord, 3).endsWith("m")) {
                                // Not a number but does not end in 'm'
                                if (trimLast(freqWord, 3) == "lf" || trimLast(freqWord, 3) == "mf") {
                                    // Ol, LF or MF
                                } else {
                                    isValid = false;
                                }
                            } else if (isNumeric(trimLast(freqWord, 4))) {
                                // It's numeric, is it a band plan?
                                if (!isBand(trimLast(freqWord, 4))) {
                                    // Not a band plan
                                    isValid = false;
                                } else {
                                    band = parseInt(trimLast(freqWord, 4));
                                    if (band == 160) {
                                        // Ok - 160m is good with -15
                                    } else {
                                        isValid = false;
                                    }
                                }
                            } else {
                                // All letters
                                isValid = false;
                            }
                        } else {
                            // Straight frequency, no -15 available
                            isValid = false;
                        }
                    }
                    // Check for "m" on the end
                    else if (freqWord.endsWith("m")) {
                        // Check to see if it's a number
                        if (isNaN(trimLast(freqWord, 1)) || trimLast(freqWord, 1) < 1) {
                            // Not a valid number
                            isValid = false;
                        } else if (!isBand(trimLast(freqWord, 1))) {
                            // Not a valid band plan
                            isValid = false;
                        }
                        // Ok
                    }
                    // Anything else is invalid
                    else {
                        // Not Ok
                        isValid = false;
                    }
                }
            }

            // Handle showing validity
            if (isValid) {
                $("#frequencies").removeClass("is-invalid");
                $("#frequencies").addClass("is-valid");
            } else {
                $("#frequencies").removeClass("is-valid");
                $("#frequencies").addClass("is-invalid");
            }
        };

        document.querySelector("#ppm").addEventListener("input", function () {
            let form = this.closest("form"); // Find the closest form

            if (this.checkValidity()) {
                this.classList.add("is-valid");
                this.classList.remove("is-invalid");
            } else {
                this.classList.add("is-invalid");
                this.classList.remove("is-valid");
            }

            form.classList.add("was-validated"); // Keep Bootstrap validation active
        });

        function isNumeric(num) {   
            return !isNaN(num)
        };

        function trimLast(string, num) {
            return string.substring(0, string.length - num);
        };

        function isBand(num) {
            const bandArray = [160, 80, 60, 40, 30, 20, 17, 15, 12, 10, 6, 4, 2];
            return bandArray.includes(parseInt(num));
        };

        function loadPage() {
            populateConfig();
        };

        function validatePage() {
            // $('element').hasClass('className')
            var thisSection = $('#wsprconfig');
            var thisForm = document.querySelector('#wsprform');
            var failcount = 0;
            if (!thisForm.reportValidity()) failcount++;
            if (!$("#frequencies").hasClass('is-valid')) failcount++;
            return (failcount > 0 ? false : true);
        };

        function populateConfig(callback = null) {
            if (populateConfigRunning) return;
            populateConfigRunning = true;

            $.getJSON(url)
                .done(function(configJson) {
                    try {
                        if (!configJson || typeof configJson !== "object") {
                            throw new Error("Invalid JSON data received.");
                        }

                        // Assign values from JSON to form elements
                        //
                        // [Control]
                        $('#transmit').prop('checked', configJson["Control"]["Transmit"]);
                        // [Common]
                        $('#callsign').val(configJson["Common"]["Call Sign"]);
                        $('#gridsquare').val(configJson["Common"]["Grid Square"]);
                        $('#dbm').val(configJson["Common"]["TX Power"]);
                        $('#frequencies').val(configJson["Common"]["Frequency"]);
                        // [Extended]
                        // Safely retrieve and parse the PPM value
                        let ppmValue = parseFloat(configJson?.["Extended"]?.["PPM"]);

                        // Check if ppmValue is a valid number, otherwise default to 0.0
                        if (isNaN(ppmValue)) {
                            ppmValue = 0.0;
                        }

                        // Assign the valid double value to the input field
                        $('#ppm').val(ppmValue.toFixed(2));  // Formats as a decimal (e.g., 3.14)
                        $('#use_ntp').prop('checked', configJson["Extended"]["Use NTP"]);
                        $('#useoffset').prop('checked', configJson["Extended"]["Offset"]);
                        $('#use_led').prop('checked', configJson["Extended"]["Use LED"]);
                        setGpioSelect(configJson["Extended"]["LED Pin"]);
                        $('#power_level').val(configJson["Extended"]["Power Level"]).change();
                        // [Server]
                        $('#server_port').val(configJson["Server"]["Port"]);
                        
                        // Enable or disable PPM based on NTP setting
                        // TODO: Do I need this for #gpio_select?
                        $('#ppm').prop("disabled", $('#use_ntp').is(":checked"));

                        // Enable the form
                        $('#submit').prop("disabled", false);
                        $('#reset').prop("disabled", false);
                        $('#wsprconfig').prop("disabled", false);

                        // Validate frequency field
                        checkFreq();

                        // Run callback if provided
                        if (typeof callback === "function") {
                            callback();
                        }
                    } catch (error) {
                        alert("Unable to parse data.");
                        console.error("Error parsing config JSON:", error);
                        setTimeout(populateConfig, 10000);
                    }
                })
                .fail(function(jqXHR, textStatus, errorThrown) {
                    alert("Unable to retrieve data.");
                    console.error("Error fetching config JSON:", textStatus, errorThrown);
                    setTimeout(populateConfig, 10000);
                })
                .always(function() {
                    populateConfigRunning = false;
                });
        };

        function savePage() {
            if (!validatePage()) {
                alert("Please correct the errors on the page.");
                return false;
            }
            $('#submit').prop("disabled", true);
            $('#reset').prop("disabled", true);
            var Control = {
                "Transmit": $('#transmit').is(":checked"),
            };

            var Common = {
                "Call Sign": $('#callsign').val(),
                "Grid Square": $('#gridsquare').val(),
                "TX Power": parseInt($('#dbm').val()),
                "Frequency": $('#frequencies').val(),
            };

            var Extended = {
                "PPM": parseFloat($('#ppm').val()),
                "Power Level": parseInt($('#power_level').val()),
                "Use NTP": $('#use_ntp').is(":checked"),
                "Use NTP": $('#use_ntp').is(":checked"),
                "Offset": $('#useoffset').is(":checked"),
                "Offset": $('#useoffset').is(":checked"),
                "Use LED": $('#use_led').is(":checked"),
                "LED Pin": parseInt(getGPIONumber()),
            };

            var Server = {
                "Port": $('#server_port').val(),
            };

            var Config = {
                Control,
                Common,
                Extended,
                Server,
            };
            var json = JSON.stringify(Config);

            $.ajax({
                    url: url,
                    data: json,
                    type: 'PUT'
                })
                .done(function(data) {
                    // Done
                })
                .fail(function(data) {
                    // Fail
                    alert("Unable to save data.")
                    console.log("Unable to POST data.\n" + data);
                })
                .always(function(data) {
                    setTimeout(() => {
                        $('#submit').prop("disabled", false);
                        $('#reset').prop("disabled", false);
                    }, 500)

                });
        };

        function resetPage() {
            // Disable Form
            $('#submit').prop("disabled", false);
            $('#reset').prop("disabled", false);
            $('#wsprconfig').prop("disabled", true);
            populateConfig();
        };

function clickUseNTP() {
    if ($('#use_ntp').is(":checked")) {
        // Disable PPM when using self-cal and remove validation
        $('#ppm').prop("disabled", true);
        $('#ppm').removeClass("is-valid is-invalid"); // ✅ Clear validation classes
    } else {
        // Enable PPM when not using self-cal and trigger validation
        $('#ppm').prop("disabled", false).trigger("input");
    }
}

// ✅ Custom function to validate PPM when enabled
function validatePPM() {
    let ppmInput = $('#ppm');
    let value = parseFloat(ppmInput.val());

    if ($('#use_ntp').is(":checked")) {
        // ✅ If NTP is checked, ignore validation
        ppmInput.removeClass("is-valid is-invalid");
        return;
    }

    if (isNaN(value) || value < -200 || value > 200) {
        ppmInput.addClass("is-invalid").removeClass("is-valid");
    } else {
        ppmInput.addClass("is-valid").removeClass("is-invalid");
    }
}

// ✅ Attach event listener for validation when typing
$('#ppm').on("input", validatePPM);
$('#use_ntp').on("change", clickUseNTP); // Ensure function runs when checkbox changes

        function clickUseLED() {
            if ($('#use_led').is(":checked")) {
                // Enable LED pin when using LED
                $('#gpio_select').prop("disabled", false);
            } else {
                // Disable LED pin when not using LED
                $('#gpio_select').prop("disabled", true);
            }
        };

        function getGPIONumber() {
            let gpioValue = $("#gpio_select").val(); // Get the selected value (e.g., "GPIO17")
            let gpioNumber = gpioValue.match(/\d+/); // Extract numeric portion using regex

            return gpioNumber ? parseInt(gpioNumber[0]) : null; // Convert to integer and return
        };

        function setGpioSelect(gpioNumber) {
            let gpioValue = "GPIO" + gpioNumber; // Construct the expected value, e.g., "GPIO17"
            
            // Check if the option exists before setting it
            if ($("#gpio_select option[value='" + gpioValue + "']").length > 0) {
                $("#gpio_select").val(gpioValue).trigger("change"); // Set and trigger change event
            } else {
                console.warn("GPIO value not found:", gpioValue);
            }
        };
    </script>
</body>

</html>
