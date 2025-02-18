<?php
header("Content-Type: application/json"); // Set response type to JSON

// Execute the command and capture the output
$output = shell_exec('/usr/local/bin/wsprrypi --version');

// Process the output
$output = substr($output, strlen("WsprryPi version ")); // Remove prefix
$output = trim($output);    // Remove leading/trailing spaces & newlines
$output = rtrim($output, ".");  // Remove trailing period (if present)

// Send JSON response
echo json_encode(["wspr_version" => $output]);
?>