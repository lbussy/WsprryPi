# Created for WsprryPi version "0.0.1"
/var/log/wsprrypi/*.log {
    rotate 14
    daily
    compress
    missingok
    notifempty
    sharedscripts
    postrotate
        systemctl reload your-app
    endscript
}
