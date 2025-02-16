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
    <link rel="stylesheet" href="bootstrap.css">
    <link rel="stylesheet" href="custom.css">
    <script src="jquery-3.6.3.min.js"></script>
</head>

<body>
    <div class="navbar navbar-expand-lg fixed-top navbar-dark bg-primary">
        <div class="container">
            <a href="../" class="navbar-brand">
                <img src="antenna.svg" style="width:30px;height:30px;" />
                &nbsp;
                Wsprry Pi
            </a>
            <button class="navbar-toggler" type="button" data-bs-toggle="collapse" data-bs-target="#navbarResponsive" aria-controls="navbarResponsive" aria-expanded="false" aria-label="Toggle navigation">
                <span class="navbar-toggler-icon"></span>
            </button>
            <div class="collapse navbar-collapse" id="navbarResponsive">
                <ul class="navbar-nav">

                </ul>
                <ul class="navbar-nav ms-md-auto">
                    <li class="nav-item">
                        <a target="_blank" rel="noopener" class="nav-link" href="https://github.com/lbussy/WsprryPi/"><i class="fa-brands fa-github"></i>&nbsp;&nbsp;GitHub</a>
                    </li>
                    <li class="nav-item">
                        <a target="_blank" rel="noopener" class="nav-link" href="http://wsprdocs.aa0nt.net"><i class="fa-solid fa-book"></i>&nbsp;&nbsp;Documentation</a>
                    </li>
                </ul>
            </div>
        </div>
    </div>

    <div class="container">

        <div class="card border-primary mb-3">
            <div class="card-header d-flex">
                For server: <?php echo gethostname(); ?>
                <span class="ms-auto text-muted">
                    <form action="shutdown.php" method="post">
                        <button type="submit" class="btn-fafa" data-toggle="tooltip" title="Shutdown">
                            <i class="fa-solid fa-power-off"></i>
                        </button>
                    </form>
                </span>
            </div>
            <div class="card-body">

                <h3 class="card-title">Wsprry Pi Configuration</h3>

                <form id="wsprform">
                    <id="wsprconfig" class="form-group" disabled="disabled">
                        <!-- First Row -->
                        <legend class="mt-4">Control</legend>
                        <div class="was-validated container">
                            <div class="row">
                                <div class="col-md-6">
                                    <div class="row gx-1 ">
                                        <div class="col-md-4 text-end">
                                            <label class="form-check-label" for="transmit">Enable
                                                Transmission:&nbsp;</label>
                                        </div>
                                        <div class="col-md-8">
                                            <div class="form-check form-switch">
                                                <input class="form-check-input" type="checkbox" id="transmit" data-form-type="other">
                                            </div>
                                        </div>
                                    </div>
                                </div>
                                <div class="col-md-6">
                                    <div class="row gx-1 ">
                                        <div class="col-md-4 text-end">
                                            <label class="form-check-label" for="use_led">Enable LED:&nbsp;</label>
                                        </div>
                                        <div class="col-md-8">
                                            <div class="form-check form-switch">
                                                <input class="form-check-input" type="checkbox" id="use_led" data-form-type="other">
                                            </div>
                                        </div>
                                    </div>
                                </div>
                            </div>
                        </div>
                        <!-- First Row -->

                        <!-- Second Row -->
                        <legend class="mt-4">Operator Information</legend>
                        <div class="was-validated container">
                            <div class="row">
                                <div class="col-md-6">
                                    <div class="row gx-1 ">
                                        <div class="col-md-3 text-end">
                                            Call Sign:&nbsp;
                                        </div>
                                        <div class="col-md-9">
                                            <input type="text" pattern="^([A-Za-z]{1,2}[0-9][A-Za-z0-9]{1,3}|[A-Za-z][0-9][A-Za-z]|[0-9][A-Za-z][0-9][A-Za-z0-9]{2,3})$" minlength="3" maxlength="6" class="form-control" id="callsign" placeholder="Enter callsign" required>    
                                            <div class="valid-feedback">Valid.</div>
                                            <div class="invalid-feedback">Please enter your callsign.</div>
                                        </div>
                                    </div>
                                </div>
                                <div class="col-md-6">
                                    <div class="row gx-1 ">
                                        <div class="col-md-3 text-end">
                                            Grid Square:&nbsp;
                                        </div>
                                        <div class="col-md-9">
                                            <input minlength="4" maxlength="4" pattern="[a-z,A-Z]{2}[0-9]{2}" type="text" class="form-control" id="gridsquare" placeholder="Enter gridsquare" required>
                                            <div class="valid-feedback">Valid.</div>
                                            <div class="invalid-feedback">Please enter your four-character grid square.
                                            </div>
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

                                <div class="col-md-6">
                                    <div class="row gx-1 ">
                                        <div class="col-md-3 text-end">
                                            Transmit Power:&nbsp;
                                        </div>
                                        <div class="was-validated col-md-9">
                                            <input type="number" min="-10" max="62" step="1" class="form-control" id="dbm" placeholder="Enter transmit power" required>
                                            <div class="valid-feedback">Valid.</div>
                                            <div class="invalid-feedback">Please enter your transmit power in dBm
                                                (without the 'dBm' suffix.)</div>
                                        </div>
                                    </div>
                                </div>
                                <div class="col-md-6">
                                    <div class="row gx-1 ">
                                        <div class="col-md-3 text-end">
                                            Frequency:&nbsp;
                                        </div>
                                        <div class="col-md-9">
                                            <input type="text" class="form-control" id="frequencies" placeholder="Enter frequency" oninput="checkFreq();">
                                            <div class="valid-feedback">Ok.</div>
                                            <div class="invalid-feedback">Add a single frequency or a space-delimited
                                                list (see <a href="https://wsprry-pi.readthedocs.io/en/latest/Operations/index.html" target="_blank" rel="noopener noreferrer">documentation</a>.)</div>
                                        </div>
                                    </div>
                                </div>

                                <!-- Row Split -->

                                <div class="was-validated row">
                                    <div class="col-md-6">
                                        <div class="row gx-1 ">
                                            <div class="col-md-4 text-end">
                                                <!-- Empty -->
                                            </div>
                                            <div class="col-md-8">
                                                <!-- Empty -->
                                            </div>
                                        </div>
                                    </div>
                                    <div class="col-md-6">
                                        <div class="row gx-1 ">
                                            <div class="col-md-4 text-end">
                                                <label class="form-check-label" for="useoffset">Random
                                                    Offset:&nbsp;</label>
                                            </div>
                                            <div class="col-md-8">
                                                <div class="form-check form-switch">
                                                    <input class="form-check-input" type="checkbox" id="useoffset" data-form-type="other">
                                                </div>
                                            </div>
                                        </div>
                                    </div>
                                </div>
                            </div>
                        </div>
                        <!-- End Third Row -->

                        <!-- Fourth Row -->
                        <legend class="mt-4">Advanced Information</legend>
                        <div class="container">
                            <div class="was-validated row">
                                <div class="col-md-6">
                                    <div class="row gx-1 ">
                                        <div class="col-md-4 text-end">
                                            <label class="form-check-label" for="use_ntp">
                                                Use NTP for Calibration:&nbsp;</label>
                                        </div>
                                        <div class="col-md-8">
                                            <div class="form-check form-switch">
                                                <input class="form-check-input" type="checkbox" id="use_ntp" data-form-type="other">
                                            </div>
                                        </div>
                                    </div>
                                </div>
                                <div class="col-md-6">
                                    <div class="row gx-1 ">
                                        <div class="col-md-3 text-end">
                                            PPM Offset:&nbsp;
                                        </div>
                                        <div class="col-md-9">
                                            <input type="number" min="-200" max="200" step=".000001" class="form-control" id="ppm" placeholder="Enter PPM" required>
                                            <div class="valid-feedback">Valid.</div>
                                            <div class="invalid-feedback">Enter a positive or negative decimal number
                                                for frequency correction.</div>
                                        </div>
                                    </div>
                                </div>
                            </div>
                        </div>
                        <!-- End Fourth Row -->

                        <!-- Fifth Row -->
                        <legend class="mt-4">Transmit Power</legend>
                        <p>This sets power on the Pi GPIO only; any amplification must be taken into account.</p>
                        <div class="container">
                            <div class="was-validated row">
                                <div class="col-md-2"></div>
                                <div class="col-md-8">
                                    <div class="range-wrap">
                                        <input id="power_level" type="range" class="range" min="0" max="7">
                                        <output id="rangeText" class="bubble"></output>
                                    </div>
                                </div>
                                <div class="col-md-2"></div>
                            </div>
                        </div>
                        <p></p>

                        <!-- End Fifth Row -->

                        <div id="buttons" class="modal-footer justify-content-center">
                            <button id="submit" type="button" class="btn btn-primary">&nbsp;Save&nbsp;</button>
                            &nbsp;
                            <button id="reset" type="button" class="btn btn-secondary">Reset</button>
                        </div>

                        </fieldset>
                    <!-- :Hidden Form Items: -->
                    <input type="text" class="form-control" id="led_pin" placeholder="">
                    <input type="text" class="form-control" id="server_port" placeholder="">
                    <!-- ^Hidden Form Items^ -->
                </form>
            </div>
        </div>

        <footer id="footer">
            <div class="row">
                <div class="col-lg-12">
                    <ul class="list-unstyled">
                        <li><a href="http://wsprdocs.aa0nt.net">Documentation</a></li>
                        <li><a href="https://github.com/lbussy/WsprryPi">GitHub</a></li>
                        <li><a href="https://tapr.org/">TAPR</a></li>
                        <li><a href="https://www.wsprnet.org/">WSPRNet</a></li>
                    </ul>
                    <p>Created by Lee Bussy, AA0NT.</p>
                    <p>Code released under the <a href="https://github.com/lbussy/WsprryPi/blob/main/LICENSE.md">GNU
                            General Public License</a>.</p>
                    <p>Based on <a href="https://getbootstrap.com/" rel="nofollow">Bootstrap</a>. Icons from <a href="https://fontawesome.com/" rel="nofollow">Font Awesome</a>. Web fonts from <a href="https://fonts.google.com/" rel="nofollow">Google</a>.</p>
                </div>
            </div>
        </footer>
    </div>

    <script src="bootstrap.bundle.min.js"></script>
    <script src="https://kit.fontawesome.com/e51821420e.js" crossorigin="anonymous"></script>

    <script>
        var url = "wspr_ini.php";
        var populateConfigRunning = false;

        $(function () {
            $('[data-toggle="tooltip"]').tooltip()
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
                const val = $('#power_level').val();
                let valPct = (val / 7);
                const min = 0.0095;
                const max = .984;
                const range = max - min;
                valPct = ((valPct * range) + min) * 100;
                $('#rangeText').attr("style", "left:" + valPct + "%;");
            });
        };

        function checkFreq() {
            isValid = true;
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

            var configJson = $.getJSON(url, function(data) {
                    // Clear any warnings here
                })
                .done(function(configJson) {
                    try {
                        $('#transmit').prop('checked', configJson["Control"]["Transmit"]);
                        $('#use_led').prop('checked', configJson["Extended"]["Use LED"]);
                        $('#callsign').val(configJson["Common"]["Call Sign"]);
                        $('#gridsquare').val(configJson["Common"]["Grid Square"]);
                        $('#dbm').val(configJson["Common"]["TX Power"]);
                        $('#frequencies').val(configJson["Common"]["Frequency"]);
                        $('#useoffset').prop('checked', configJson["Extended"]["Offset"]);
                        $('#use_ntp').prop('checked', configJson["Extended"]["Use NTP"]);
                        $('#ppm').val(configJson["Extended"]["PPM"]);
                        if ($('#use_ntp').is(":checked")) {
                            // Disable PPM when using self-cal
                            $('#ppm').prop("disabled", true);
                        } else {
                            // Enable PPM when not using self-cal
                            $('#ppm').prop("disabled", false);
                        }
                        $('#power_level').val(configJson["Extended"]["Power Level"]);
                        $('#power_level').change();

                        // New Items - Hidden form Ffelds
                        $('#led_pin').val(configJson["Extended"]["LED Pin"]);
                        $('#server_port').val(configJson["Server"]["Port"]);

                        // Enable Form
                        $('#submit').prop("disabled", false);
                        $('#reset').prop("disabled", false);
                        $('#wsprconfig').prop("disabled", false);

                        checkFreq(); // frequency is not validated by HTML5

                        if (typeof callback == "function") {
                            callback();
                        }
                    } catch {
                        alert("Unable to parse data.")
                        console.log("Unable to parse data.");
                        setTimeout(populateConfig, 10000);
                    }
                })
                .fail(function(data) {
                    alert("Unable to retrieve data.")
                    console.log("Unable to retrieve data.");
                    setTimeout(populateConfig, 10000);
                })
                .always(function(data) {
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
                "Offset": $('#useoffset').is(":checked"),
                "Offset": $('#useoffset').is(":checked"),
                "Use LED": $('#use_led').is(":checked"),
                "LED Pin": $('#led_pin').val(),
            };

            var Server = {
                "Port": $('#server_port').val(),
            };

            var Config = {
                Control,
                Common,
                Extended,
                Server
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
                // Disable PPM when using self-cal
                $('#ppm').prop("disabled", true);
            } else {
                // Enable PPM when not using self-cal
                $('#ppm').prop("disabled", false);
            }
        };
    </script>
</body>

</html>
