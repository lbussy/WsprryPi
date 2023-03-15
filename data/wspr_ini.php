<?php
set_exception_handler('myException');

$file = "wspr.ini";

if ($_SERVER['REQUEST_METHOD'] === 'POST' || $_SERVER['REQUEST_METHOD'] === 'PUT') {
    // Convert to a PHP object
    $post = file_get_contents('php://input');
    $decoded = urldecode($post);
    $data = json_decode($post, true);
    // Write INI
    if (write_ini_file($file, $data)) {
        header("Cache-Control: no-cache, must-revalidate");
        header("HTTP/1.0 200 Ok");
    } else {
        ; // Errors written beloe
    }
} else {
    // Read and send INI
    read_ini_file($file);
}

function read_ini_file($file)
{
    // Parse with sections
    $ini_array = parse_ini_file($file, true, INI_SCANNER_TYPED);
    if (! $ini_array) {
        throw new Exception('Unable to read configuration.');
    }
    try {
        header("Cache-Control: no-cache, must-revalidate");
        header("HTTP/1.0 200 Ok");
        echo json_encode($ini_array);
    }
    // Catch other exceptions
    catch(Exception $e) {
        throw new Exception('Unable to process configuration: ' . $e->getMessage());
    }
}

function write_ini_file($file, $array = []) {
    // check first argument is string
    if (!is_string($file)) {
        throw new Exception('Function argument 1 must be a string.');
    }

    // check second argument is array
    if (!is_array($array)) {
        throw new Exception('Function argument 2 must be an array.');
    }

    // process array
    $data = array();
    foreach ($array as $key => $val) {
        if (is_array($val)) {
            $data[] = "[$key]";
            foreach ($val as $skey => $sval) {
                if (is_array($sval)) {
                    foreach ($sval as $_skey => $_sval) {
                        if (is_numeric($_skey)) {
                            $data[] = $skey.'[] = '.(is_bool($_sval) ? (($_sval) ? 'true' : 'false') : $_sval);
                        } else {
                            $data[] = $skey.'['.$_skey.'] = '.(is_bool($_sval) ? (($_sval) ? 'true' : 'false') : $_sval);
                        }
                    }
                } else {
                    $data[] = $skey.' = '.(is_bool($sval) ? (($sval) ? 'true' : 'false') : $sval);
                }
            }
        } else {
            $data[] = $key.' = '.(is_bool($val) ? (($val) ? 'true' : 'false') : $val);
        }
        // empty line
        $data[] = null;
    }

    // open file pointer, init flock options
    $fp = fopen($file, 'w');
    $retries = 0;
    $max_retries = 100;

    if (!$fp) {
        throw new Exception('Unable to find file.');
    }

    // loop until get lock, or reach max retries
    do {
        if ($retries > 0) {
            usleep(rand(1, 5000));
        }
        $retries += 1;
    } while (!flock($fp, LOCK_EX) && $retries <= $max_retries);

    // couldn't get the lock
    if ($retries == $max_retries) {
        throw new Exception('Unable to obtain lock on file.');
    }

    // got lock, write data
    fwrite($fp, implode(PHP_EOL, $data).PHP_EOL);

    // release lock
    flock($fp, LOCK_UN);
    fclose($fp);

    return true;
}

function myException($exception) {
    header("Cache-Control: no-cache, must-revalidate");
    header("HTTP/1.0 500 System Error");
    echo "<b>Error:</b> " . $exception->getMessage();
  }
