# Copyright (C) 2023-2024 Lee C. Bussy (@LBussy)
# Created for WsprryPi project, version 1.2.1-9f78347 [new_release_proc].

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
