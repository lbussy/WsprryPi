<?php
error_reporting(0);
ini_set('display_errors', 0);
// error_reporting(E_ALL);
// ini_set('display_errors', 1);

// Set custom exception handler
set_exception_handler('myException');

$file = "wspr.ini";

if ($_SERVER['REQUEST_METHOD'] === 'POST' || $_SERVER['REQUEST_METHOD'] === 'PUT') {
    // Convert input to a PHP object
    $post = file_get_contents('php://input');
    $data = json_decode($post, true);

    if ($data === null) {
        throw new Exception('Invalid JSON input.');
    }

    // Write INI and send response only on success
    if (write_ini_file($file, $data)) {
        header("Cache-Control: no-cache, must-revalidate");
        header("HTTP/1.0 200 OK");
    }
} else {
    // Read and send INI
    read_ini_file($file);
}

/**
 * Reads an INI file and outputs it as JSON.
 *
 * @param string $file The path to the INI file.
 * @throws Exception If the INI file cannot be read or parsed.
 */
function read_ini_file($file)
{
    if (!file_exists($file)) {
        throw new Exception("INI file '$file' not found.");
    }

    $ini_array = parse_ini_file($file, true, INI_SCANNER_TYPED);
    if (!$ini_array) {
        throw new Exception('Unable to read configuration.');
    }

    $json_output = json_encode($ini_array);
    if ($json_output === false) {
        throw new Exception('JSON encoding failed: ' . json_last_error_msg());
    }

    try {
        header("Cache-Control: no-cache, must-revalidate");
        header("HTTP/1.0 200 OK");
        echo $json_output;
    } catch (Exception $e) {
        throw new Exception('Unable to process configuration: ' . $e->getMessage());
    }
}

/**
 * Writes an array to an INI file.
 *
 * @param string $file The file path.
 * @param array $array The data array.
 * @return bool Returns true on success.
 * @throws Exception If file access or writing fails.
 */
function write_ini_file($file, $array = [])
{
    if (!is_string($file)) {
        throw new Exception('Function argument 1 must be a string.');
    }

    if (!is_array($array)) {
        throw new Exception('Function argument 2 must be an array.');
    }

    $data = [];
    foreach ($array as $key => $val) {
        if (is_array($val)) {
            $data[] = "[$key]";
            foreach ($val as $skey => $sval) {
                if (is_array($sval)) {
                    foreach ($sval as $_skey => $_sval) {
                        $data[] = is_numeric($_skey)
                            ? sprintf("%s[] = %s", $skey, (is_bool($_sval) ? ($_sval ? 'true' : 'false') : $_sval))
                            : sprintf("%s[%s] = %s", $skey, $_skey, (is_bool($_sval) ? ($_sval ? 'true' : 'false') : $_sval));
                    }
                } else {
                    $data[] = sprintf("%s = %s", $skey, (is_bool($sval) ? ($sval ? 'true' : 'false') : $sval));
                }
            }
        } else {
            $data[] = sprintf("%s = %s", $key, (is_bool($val) ? ($val ? 'true' : 'false') : $val));
        }
        $data[] = null; // Add empty line
    }

    // Open file pointer
    $fp = fopen($file, 'w');
    if (!$fp) {
        throw new Exception('Unable to open file for writing.');
    }

    // Try to lock the file
    $retries = 0;
    $max_retries = 100;
    while (!flock($fp, LOCK_EX) && $retries <= $max_retries) {
        usleep(rand(1, 5000));
        $retries++;
    }

    if ($retries >= $max_retries) {
        fclose($fp);
        throw new Exception('Unable to obtain lock on file.');
    }

    // Write and close
    fwrite($fp, implode(PHP_EOL, $data) . PHP_EOL);
    flock($fp, LOCK_UN);
    fclose($fp);

    return true;
}

/**
 * Custom exception handler for returning JSON error responses.
 *
 * @param Exception $exception The exception object.
 */
function myException($exception)
{
    header("Cache-Control: no-cache, must-revalidate");
    header("HTTP/1.0 500 System Error");
    echo json_encode(["error" => $exception->getMessage()]);
}