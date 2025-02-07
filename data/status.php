<?php
// Execute the command and capture the output
$output = shell_exec('/usr/local/bin/wsprrypi --status');

// Remove any leading or trailing whitespace
$output = trim($output);

// Display the output
echo "<pre>$output</pre>";
?>