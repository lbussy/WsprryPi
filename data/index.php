<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="utf-8">
    <title>Wsprry Pi</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="apple-touch-icon" sizes="180x180" href="apple-touch-icon.png">
    <link rel="icon" type="image/png" sizes="32x32" href="favicon-32x32.png">
    <link rel="icon" type="image/png" sizes="16x16" href="favicon-16x16.png">
    <link rel="manifest" href="site.webmanifest">
    <link rel="stylesheet" href="bootstrap.css">
    <link rel="stylesheet" href="custom.min.css">
    <script src="jquery-3.6.3.min.js"></script>
  </head>
  <body>
    <div class="navbar navbar-expand-lg fixed-top navbar-dark bg-primary">
      <div class="container">
        <a href="../" class="navbar-brand">
          <img src="ham_white.svg" style="width:30px;height:30px;"/>
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
              <a target="_blank" rel="noopener" class="nav-link" href="#"><i class="fa-solid fa-book"></i>&nbsp;&nbsp;Documentation</a>
            </li>
          </ul>
        </div>
      </div>
    </div>

    <div class="container">

      <div class="card border-primary mb-3">
        <div class="card-header">For server: <?php echo gethostname();?> </div>
        <div class="card-body">

          <h3 class="card-title">Wsprry Pi Configuration</h3>

          <form class="was-validated">
            <fieldset id="wsprconfig" class="form-group" disabled="disabled">
              <!-- First Row -->
              <legend class="mt-4">Control</legend>
              <div class="container">
                <div class="row ">
                  <div class="col-md-6">
                    <div class="row gx-1 ">
                      <div class="col-md-4 text-end">
                        <label class="form-check-label" for="transmit">Enable Transmission:&nbsp;</label>
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
                      <label class="form-check-label" for="useled">Enable LED:&nbsp;</label>
                      </div>
                      <div class="col-md-8">
                        <div class="form-check form-switch">
                          <input class="form-check-input" type="checkbox" id="useled" data-form-type="other">
                        </div>
                      </div>
                    </div>
                  </div>
                </div>
              </div>
              <!-- First Row -->

              <!-- Second Row -->
              <legend class="mt-4">Operator Information</legend>
              <div class="container">
                <div class="row ">
                  <div class="col-md-6">
                    <div class="row gx-1 ">
                      <div class="col-md-3 text-end">
                        Call Sign:&nbsp;
                      </div>
                      <div class="col-md-9">
                        <input minlength="4" type="text" pattern="[-a-zA-Z0-9\/]+" class="form-control" id="callsign" placeholder="Enter callsign" required>
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
                <div class="row ">

                  <div class="col-md-6">
                    <div class="row gx-1 ">
                      <div class="col-md-3 text-end">
                        Transmit Power:&nbsp;
                      </div>
                      <div class="col-md-9">
                        <input type="number" min="-10" max="62" step="1" class="form-control" id="dbm" placeholder="Enter transmit power" required>
                        <div class="valid-feedback">Valid.</div>
                        <div class="invalid-feedback">Please enter your transmit power in dBm (without the 'dBm' suffix.)</div>
                      </div>
                    </div>
                  </div>
                  <div class="col-md-6">
                    <div class="row gx-1 ">
                      <div class="col-md-3 text-end">
                        Frequency:&nbsp;
                      </div>
                      <div class="col-md-9">
                        <input type="text" class="form-control" id="frequencies" placeholder="Enter frequency" required>
                        <div class="valid-feedback">Ok.</div>
                        <div class="invalid-feedback">Add a single frequency or a space-delimted list (see documentation.)</div>
                      </div>
                    </div>
                  </div>

                  <!-- Row Split -->

                  <div class="row ">
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
                          <label class="form-check-label" for="useoffset">Random Offset:&nbsp;</label>
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
              <div class="row ">
                  <div class="col-md-6">
                    <div class="row gx-1 ">
                      <div class="col-md-4 text-end">
                        <label class="form-check-label" for="selfcal">Self Calibration:&nbsp;</label>
                      </div>
                      <div class="col-md-8">
                        <div class="form-check form-switch">
                          <input class="form-check-input" type="checkbox" id="selfcal" data-form-type="other">
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
                        <input type="number" min="-60" max="60" step=".0001" class="form-control" id="ppm" placeholder="Enter PPM" required>
                        <div class="valid-feedback">Valid.</div>
                        <div class="invalid-feedback">Enter a positive or negative decimal number for frequency correction.</div>
                      </div>
                    </div>
                  </div>
                </div>
              </div>
              <!-- End Fourth Row -->

              <hr class="border-2 border-top">

              <div class="modal-footer justify-content-center">
                <button id="submit" type="button" class="btn btn-primary">&nbsp;Save&nbsp;</button>
                &nbsp;
                <button id="reset" type="button" class="btn btn-secondary">Reset</button>
              </div>

            </fieldset>
          </form>

        </div>
      </div>

      <footer id="footer">
        <div class="row">
          <div class="col-lg-12">
            <ul class="list-unstyled">
              <li><a href="https://tapr.org/">TAPR</a></li>
              <li><a href="https://www.wsprnet.org/">WSPRNet</a></li>
              <li><a href="#">Documentation</a></li>
              <li><a href="https://github.com/lbussy/WsprryPi">GitHub</a></li>
            </ul>
            <p>Made by Lee Bussy, AA0NT.</p>
            <p>Code released under the <a href="https://github.com/lbussy/WsprryPi/blob/main/LICENSE.md">GNU General Public License</a>.</p>
            <p>Based on <a href="https://getbootstrap.com/" rel="nofollow">Bootstrap</a>. Icons from <a href="https://fontawesome.com/" rel="nofollow">Font Awesome</a>. Web fonts from <a href="https://fonts.google.com/" rel="nofollow">Google</a>.</p>

          </div>
        </div>
      </footer>
    </div>

    <script src="bootstrap.bundle.min.js"></script>
    <script src="https://kit.fontawesome.com/e51821420e.js" crossorigin="anonymous"></script>

    <script>
      $(document).ready(function() { 
        bindActions();
        loadPage();
      });

      function bindActions(){
        // Grab Self Cal Switch
        $( "#selfcal" ).on( "click", function() {
          clickSelfCal();
        });

        // Grab Submit and Reset Buttons
        $("#submit").click(function (){
          savePage();
        });
        $("#reset").click(function (){
          resetPage();
        });
      };

      function loadPage()
      {
        populateConfig();
      };

      var populateConfigRunning = false;
      function populateConfig(callback = null) { // Get wspr data
      if (populateConfigRunning) return;
        populateConfigRunning = true;

        var url = "ini_read.php";

        var configJson = $.getJSON(url, function () {
            // Clear any warnings here
        })
            .done(function (configJson) {
                try {
                  $('#transmit').prop('checked', configJson["Control"]["Transmit"]);
                  $('#useled').prop('checked', configJson["Extended"]["Use LED"]);
                  $('#callsign').val(configJson["Common"]["Call Sign"]);
                  $('#gridsquare').val(configJson["Common"]["Grid Square"]);
                  $('#dbm').val(configJson["Common"]["TX Power"]);
                  $('#frequencies').val(configJson["Common"]["Frequency"]);
                  $('#useoffset').prop('checked', configJson["Extended"]["Offset"]);
                  $('#selfcal').prop('checked', configJson["Extended"]["Self Cal"]);
                  $('#ppm').val(configJson["Extended"]["PPM"]);
                  if ($('#selfcal').is(":checked"))
                  {
                    // Disable PPM when using self-cal
                    $('#ppm').prop( "disabled", true );
                  }
                  else
                  {
                    // Enable PPM when not using self-cal
                    $('#ppm').prop( "disabled", false );
                  }
                  // Enable Form
                  $('#wsprconfig').prop( "disabled", false );
                  
                  if (typeof callback == "function") {
                      callback();
                  }
                } catch {
                    if (!unloadingState) {
                        // Unable to parse data.
                    }
                    setTimeout(populateConfig, 10000);
                }
            })
            .fail(function () {
                if (!unloadingState) {
                    // Unable to retrieve data.
                }
                setTimeout(populateConfig, 10000);
            })
            .always(function () {
                populateConfigRunning = false;
                // Can post-process here
            });
      }

      function savePage()
      {
        var Control  = {
          "Transmit" : $('#transmit').is(":checked"),
        };

        var Common = {
          "Call Sign" : $('#callsign').val(),
          "Grid Square" : $('#gridsquare').val(),
          "TX Power" : parseInt($('#dbm').val()),
          "Frequency" : $('#frequencies').val(),
        };

        var Extended = {
          "PPM" : parseFloat($('#ppm').val()),
          "Self Cal" : $('#selfcal').is(":checked"),
          "Offset" : $('#useoffset').is(":checked"),
          "Offset" : $('#useoffset').is(":checked"),
          "Use LED" : $('#useled').is(":checked"),
        };

        var Config = {Control, Common, Extended};
        console.log("DEBUG: \n" + JSON.stringify(Config, null, 4));
        
        // TODO: Handle POST
        // $.ajax({
        //     type: "POST",
        //     url: "ini_write.php",
        //     data: Config,
        //     success: function(data) {
        //          //
        //     }
        // });
      };

      function resetPage()
      {
        // Disable Form
        $('#wsprconfig').prop( "disabled", true );
        populateConfig();
      };

      function clickSelfCal()
      {
        console.log("Executed clickSelfCal().");
        if ($('#selfcal').is(":checked"))
        {
          // Disable PPM when using self-cal
          $('#ppm').prop( "disabled", true );
        }
        else
        {
          // Enable PPM when not using self-cal
          $('#ppm').prop( "disabled", false );
        }
      };
    </script>
  </body>
</html>
