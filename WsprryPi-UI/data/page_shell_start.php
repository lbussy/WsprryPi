<?php
if (!isset($bodyClass)) {
    $bodyClass = '';
}

if (!isset($cardClass)) {
    $cardClass = 'template-card';
}
?>
<body<?= $bodyClass !== '' ? ' class="' . htmlspecialchars($bodyClass) . '"' : '' ?>>
    <a class="skip-link" href="#main-content">Skip to main content</a>
    <?php require_once 'connection_alert.php'; ?>

    <!-- Fixed Navbar -->
    <?php require_once 'navbar.php'; ?>

    <!-- Main Content -->
    <main id="main-content" class="page-shell" tabindex="-1">
        <div class="container page-shell__inner">
            <div class="card shadow-sm page-card <?= htmlspecialchars($cardClass) ?>">
