# Created for WsprryPi version 1.2.1 [main).

/var/log/wspr/wspr.*.log {
    rotate 14
    daily
    compress
    missingok
    notifempty
    sharedscripts
    postrotate
        systemctl restart wspr
    endscript
}

/var/log/wspr/shutdown-button.*.log {
    rotate 12
    monthly
    compress
    missingok
    notifempty
    sharedscripts
    postrotate
        systemctl restart shutdown-button
    endscript
}
