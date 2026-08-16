<?php

function getWsprryPiUiVersion(): string
{
    static $version = null;

    if ($version !== null) {
        return $version;
    }

    $output = shell_exec('/usr/local/bin/wsprrypi --version');
    if (!is_string($output)) {
        $version = '';
        return $version;
    }

    $version = trim($output);
    $prefix = 'WsprryPi version ';
    if (strncmp($version, $prefix, strlen($prefix)) === 0) {
        $version = substr($version, strlen($prefix));
    }
    $version = rtrim($version, ".");

    return $version;
}

function wsprrypiUiBuildFileRecords(): array
{
    $root = __DIR__;
    $trackedExtensions = [
        'css' => true,
        'js' => true,
        'php' => true,
    ];
    $excludedFiles = [
        'view_diag_logs.php' => true,
    ];
    $excludedDirectories = [
        'cache' => true,
    ];
    $records = [];

    $directory = new RecursiveDirectoryIterator(
        $root,
        FilesystemIterator::SKIP_DOTS
    );
    $filter = new RecursiveCallbackFilterIterator(
        $directory,
        static function (SplFileInfo $current) use ($root, $excludedDirectories): bool {
            if (!$current->isDir()) {
                return true;
            }

            $relative = str_replace('\\', '/', substr($current->getPathname(), strlen($root) + 1));
            $segments = explode('/', $relative);
            return !isset($excludedDirectories[$segments[0] ?? '']);
        }
    );
    $iterator = new RecursiveIteratorIterator($filter);

    foreach ($iterator as $file) {
        if (!$file instanceof SplFileInfo || !$file->isFile()) {
            continue;
        }

        $relative = str_replace('\\', '/', substr($file->getPathname(), strlen($root) + 1));
        if (isset($excludedFiles[$relative])) {
            continue;
        }

        $extension = strtolower($file->getExtension());
        if (!isset($trackedExtensions[$extension])) {
            continue;
        }

        $records[] = sprintf(
            '%s|%d|%d',
            $relative,
            $file->getMTime(),
            $file->getSize()
        );
    }

    sort($records, SORT_STRING);
    return $records;
}

function getWsprryPiUiBuildId(): string
{
    static $buildId = null;

    if ($buildId !== null) {
        return $buildId;
    }

    $records = wsprrypiUiBuildFileRecords();
    $buildId = 'mtime-' . substr(hash('sha256', implode("\n", $records)), 0, 16);
    return $buildId;
}

function wsprrypiAssetUrl(string $path): string
{
    $buildId = getWsprryPiUiBuildId();
    if ($buildId === '') {
        return $path;
    }

    $separator = strpos($path, '?') !== false ? '&' : '?';
    return $path . $separator . 'v=' . rawurlencode($buildId);
}
