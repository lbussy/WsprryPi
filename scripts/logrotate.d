# Copyright (C) 2023-2025 Lee C. Bussy (@LBussy)
# Created for WsprryPi project, version 1.2.2-0e626d8 [1.2.2_devel].

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

/var/log/wspr/shutdown_watch.*.log {
    rotate 12
    monthly
    compress
    missingok
    notifempty
    sharedscripts
    postrotate
        systemctl restart shutdown_watch
    endscript
}
