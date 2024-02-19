#!/bin/bash

sudo systemctl stop wspr
sudo systemctl stop shutdown-button
sudo cp -f ./wspr /usr/local/bin
sudo cp -f ./shutdown-button.py /usr/local/bin
sudo cp -f ./wspr.ini /usr/local/etc
[[ -d "/var/log/wspr" ]] || mkdir "/var/log/wspr"
sudo cp -f ./logrotate.d /etc/logrotate.d/wspr
sudo systemctl daemon-reload
sudo systemctl start wspr
sudo systemctl start shutdown-button
