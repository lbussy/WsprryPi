<?php
if (!isset($bodyClass)) {
    $bodyClass = '';
}

if (!isset($cardClass)) {
    $cardClass = 'template-card';
}
?>
<body<?= $bodyClass !== '' ? ' class="' . htmlspecialchars($bodyClass) . '"' : '' ?>>
    <?php require_once 'connection_alert.php'; ?>

    <!-- Fixed Navbar -->
    <?php require_once 'navbar.php'; ?>

    <!-- Main Content -->
    <main class="page-shell">
        <div class="container page-shell__inner">
            <div class="card shadow-sm page-card <?= htmlspecialchars($cardClass) ?>">
