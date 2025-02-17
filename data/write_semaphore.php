<!doctype html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title><?php echo htmlspecialchars(pathinfo(__FILE__, PATHINFO_FILENAME), ENT_QUOTES, 'UTF-8'); ?></title>
</head>
<body>

<?php
set_exception_handler('myException');

$this_page = basename(pathinfo(__FILE__, PATHINFO_FILENAME)); // Safe filename extraction

try {
    if ($_SERVER['REQUEST_METHOD'] === 'POST' || $_SERVER['REQUEST_METHOD'] === 'PUT') {
        // Check if a semaphore name was posted
        if (empty($_POST['semaphore'])) {
            header("HTTP/1.0 400 Bad Request");
            echo "No semaphore name passed.";
            exit;
        }
    
        // Ensure an action is provided
        if (empty($_POST['action'])) {
            header("HTTP/1.0 400 Bad Request");
            echo "No action specified.";
            exit;
        }
    
        // Get and sanitize the action
        $action = trim($_POST['action']);
    
        // Sanitize semaphore name
        $semaphore_name = preg_replace('/[^a-zA-Z0-9_]/', '', $_POST['semaphore']);
        if (empty($semaphore_name)) {
            header("HTTP/1.0 400 Bad Request");
            echo "Invalid semaphore name.";
            exit;
        }
    
        // Define the semaphore file name
        $file = "$semaphore_name.semaphore";
    
        // Handle different actions
        switch ($action) {
            case 'reboot':
                $message = "Rebooting system.";
                break;
            case 'shutdown':
                $message = "Shutting down system.";
                break;
            default:
                header("HTTP/1.0 400 Bad Request");  // FIXED: Changed from 405 to 400
                echo "Invalid action specified.";
                exit;
        }
    
        // Write the semaphore file
        if (write_file($file)) {
            header("HTTP/1.0 200 OK");
            echo "$message.";
        } else {
            throw new Exception("Failed to write to file: $file.");
        }
    } else {
        header("HTTP/1.0 400 Bad Request");
        echo "Invalid request method.";
    }
} catch (Exception $e) {
    myException($e, $this_page);
}

/**
 * Writes a semaphore file.
 *
 * @param string $file The file path.
 * @return bool True on success, false on failure.
 */
function write_file($file) {
    try {
        // Write empty content to the file
        if (file_put_contents($file, '') === false) {
            throw new Exception("Could not write to file: $file");
        }

        // Set file permissions and check if it succeeds
        if (!chmod($file, 0666)) {
            throw new Exception("Failed to set file permissions for: $file");
        }

        return true;
    } catch (Exception $e) {
        error_log("Error writing file: $file - " . $e->getMessage());
        return false;
    }
}

/**
 * Global exception handler.
 *
 * @param Exception $exception The thrown exception.
 * @param string $this_page The name of the page for logging.
 */
function myException($exception, $this_page) {
    header("Cache-Control: no-cache, must-revalidate");
    header("HTTP/1.0 500 Internal Server Error");
    echo "<b>Error ($this_page):</b> " . htmlspecialchars($exception->getMessage(), ENT_QUOTES, 'UTF-8');
    error_log("Unhandled Exception in $this_page: " . $exception->getMessage());
}
?>

</body>
</html>