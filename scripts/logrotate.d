# Created for WsprryPi version "1.1.0"
/var/log/wsprrypi/wspr.*.log {
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

/var/log/wsprrypi/shutdown-button.*.log {
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
