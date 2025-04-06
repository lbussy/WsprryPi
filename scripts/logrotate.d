# Copyright (C) 2023-2025 Lee C. Bussy (@LBussy)
# Created for WsprryPi project, version 1.2.2-f4b0009 [main].

/var/log/wsprrypi/wsprrypi.*.log {
/var/log/wsprrypi/wsprrypi.*.log {
    rotate 14
    daily
    compress
    missingok
    notifempty
    sharedscripts
    postrotate
        systemctl restart wsprrypi
        systemctl restart wsprrypi
    endscript
}

/var/log/wspr/shutdown_watch.*.log {
/var/log/wspr/shutdown_watch.*.log {
    rotate 12
    monthly
    compress
    missingok
    notifempty
    sharedscripts
    postrotate
        systemctl restart shutdown_watch
        systemctl restart shutdown_watch
    endscript
}
