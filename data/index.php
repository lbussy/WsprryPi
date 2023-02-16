<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="utf-8">
    <title>Wsprry Pi</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="stylesheet" href="bootstrap.css">
    <link rel="stylesheet" href="custom.min.css">
  </head>
  <body>
    <div class="navbar navbar-expand-lg fixed-top navbar-dark bg-primary">
      <div class="container">
        <a href="../" class="navbar-brand">Wsprry Pi</a>
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

          <fieldset class="form-group">
            <!-- First Row -->
            <legend class="mt-4">Control</legend>
            <div class="container">
              <div class="row ">
                <div class="col-md-6">
                  <div class="row gx-1 ">
                    <div class="col-md-4 text-end">
                      <label class="form-check-label" for="flexSwitchCheckDefault">Enable Transmission:&nbsp;</label>
                    </div>
                    <div class="col-md-8">
                      <div class="form-check form-switch">
                        <input class="form-check-input" type="checkbox" id="flexSwitchCheckDefault" data-form-type="other">
                      </div>
                    </div>
                  </div>
                </div>
                <div class="col-md-6">
                  <div class="row gx-1 ">
                    <div class="col-md-4 text-end">
                    <label class="form-check-label" for="flexSwitchCheckDefault">Enable LED:&nbsp;</label>
                    </div>
                    <div class="col-md-8">
                      <div class="form-check form-switch">
                        <input class="form-check-input" type="checkbox" id="flexSwitchCheckDefault" data-form-type="other">
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
                      <input type="text" class="form-control" id="exampleInputEmail1" aria-describedby="emailHelp" placeholder="Enter callsign">
                      <small id="emailHelp" class="form-text text-muted">Please make sure you are licensed and this is really your call sign.</small>
                    </div>
                  </div>
                </div>
                <div class="col-md-6">
                  <div class="row gx-1 ">
                    <div class="col-md-3 text-end">
                      Grid Square:&nbsp;
                    </div>
                    <div class="col-md-9">
                      <input type="text" class="form-control" id="exampleInputEmail1" aria-describedby="emailHelp" placeholder="Enter gridsquare">
                      <small id="emailHelp" class="form-text text-muted">This should be the first four characters like 'EM18'.</small>
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
                      <input type="text" class="form-control" id="exampleInputEmail1" aria-describedby="emailHelp" placeholder="Enter transmit power">
                      <small id="emailHelp" class="form-text text-muted">This is a number representing power in dBm (without the suffix.)</small>
                    </div>
                  </div>
                </div>
                <div class="col-md-6">
                  <div class="row gx-1 ">
                    <div class="col-md-3 text-end">
                      Frequency:&nbsp;
                    </div>
                    <div class="col-md-9">
                      <input type="text" class="form-control" id="exampleInputEmail1" aria-describedby="emailHelp" placeholder="Enter frequency">
                      <small id="emailHelp" class="form-text text-muted">Add a single frequency or a space-delimted list (see documentation.)</small>
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
                        <label class="form-check-label" for="flexSwitchCheckDefault">Random Offset:&nbsp;</label>
                      </div>
                      <div class="col-md-8">
                        <div class="form-check form-switch">
                          <input class="form-check-input" type="checkbox" id="flexSwitchCheckDefault" data-form-type="other">
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
                      <label class="form-check-label" for="flexSwitchCheckDefault">Self Calibration:&nbsp;</label>
                    </div>
                    <div class="col-md-8">
                      <div class="form-check form-switch">
                        <input class="form-check-input" type="checkbox" id="flexSwitchCheckDefault" data-form-type="other">
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
                      <input type="text" class="form-control" id="exampleInputEmail1" aria-describedby="emailHelp" placeholder="Enter PPM">
                      <small id="emailHelp" class="form-text text-muted">This is a positive or negative decimal number.</small>
                    </div>
                  </div>
                </div>
              </div>
            </div>
            <!-- End Fourth Row -->

          </fieldset>

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
  </body>
</html>
