<?php
set_exception_handler('myException');

$file = "shutdown";

if ($_SERVER['REQUEST_METHOD'] === 'POST' || $_SERVER['REQUEST_METHOD'] === 'PUT') {
    // Write semaphore
    if (write_shutdown($file)) {
        header("Cache-Control: no-cache, must-revalidate");
        header("HTTP/1.0 200 Ok");
        echo("Server shutdown has started.");
    } else {
        header("Cache-Control: no-cache, must-revalidate");
        header("HTTP/1.0 500 System Error");
        echo "<b>Error:</b> " . $exception->getMessage();
        ;
    }
} else {
    header("Cache-Control: no-cache, must-revalidate");
    header("HTTP/1.0 400 System Error");
    echo("HTTP/1.0 400 Bad Request");
}

function write_shutdown($file) {
    // Touch file
    $myfile = fopen($file, "a");
    fclose($myfile);
    return true;
}

function myException($exception) {
    header("Cache-Control: no-cache, must-revalidate");
    header("HTTP/1.0 500 System Error");
    echo "<b>Error:</b> " . $exception->getMessage();
}
