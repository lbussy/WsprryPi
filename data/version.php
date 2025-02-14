<?php
// Execute the command and capture the output
$output = shell_exec('/usr/local/bin/wsprrypi --version');

// Strip "WsprryPi version " from the beginning and the period from the end
$output = substr($output, strlen("WsprryPi version "));
$output = rtrim($output, ".");

// Remove any leading or trailing whitespace
$output = trim($output);

// Display the output
echo "<pre>$output</pre>";
?>