#!/bin/bash

sudo rm -fr /var/www/html/wspr
sudo mkdir /var/www/html/wspr
sudo cp -R /home/pi/WsprryPi/data/* /var/www/html/wspr/
sudo ln -sf /usr/local/etc/wspr.ini /var/www/html/wspr/wspr.ini
sudo chown -R www-data:www-data /var/www/html/wspr/
