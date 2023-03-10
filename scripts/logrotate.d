# Created for WsprryPi version "0.0.1"
/var/log/wsprrypi/*.log {
    rotate 10
    weekly
    compress
    missingok
    notifempty
    sharedscripts
    postrotate
        systemctl reload your-app
    endscript
}
