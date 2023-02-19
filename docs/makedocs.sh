#!/bin/bash

sudo chown -R pi:pi ./_build/html/

make clean
make html

sudo chown -R www-data:www-data ./_build/html/
