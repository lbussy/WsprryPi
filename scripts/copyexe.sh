#!/bin/bash

sudo systemctl stop wspr
sudo systemctl stop shutdown-button
sudo cp -f ./wspr /usr/local/bin
sudo cp -f ./shutdown-button.py /usr/local/bin
sudo cp -f ./wspr.ini /usr/local/etc
[[ -d "/var/log/wsprrypi" ]] || mkdir "/var/log/wsprrypi"
sudo cp -f ./logrotate.d /etc/logrotate.d/wsprrypi
sudo systemctl daemon-reload
sudo systemctl start wspr
sudo systemctl start shutdown-button
