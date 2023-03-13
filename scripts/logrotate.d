# Created for WsprryPi version "0.0.1"
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
