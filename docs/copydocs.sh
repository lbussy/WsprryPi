#!/bin/bash

make clean
make html
sudo rm -fr /var/www/html/wspr/docs
sudo mkdir /var/www/html/wspr/docs
sudo cp -R /home/pi/WsprryPi/docs/_build/html /var/www/html/wspr/docs
sudo chown -R www-data:www-data /var/www/html/wspr/docs
