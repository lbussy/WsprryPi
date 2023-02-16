<?php

$file = "wspr.ini";

read_ini_file($file);

function read_ini_file($file)
{
    // Parse with sections
    $ini_array = parse_ini_file($file, true, INI_SCANNER_TYPED);
    try {
        echo json_encode($ini_array);
    } catch (HttpException $ex) {
        echo $ex;
    }
}

?>
