# Wsspry Pi Scripts

Working area for new scripting.

## Instructions for Shutdown

Copy shutdown.py to /usr/bin/shutdown.py
`sudo cp ./shutdown.py /usr/bin/shutdown.py`

`sudo chown root:root /usr/bin/shutdown.py`

`sudo chmod 0755 /usr/bin/shutdown.py`

Copy shutdown.service to /etc/systemd/system/shutdown.service
`sudo cp ./shutdown.service /etc/systemd/system/shutdown.service`

`sudo chown root:root /etc/systemd/system/shutdown.service`

`sudo chmod 0644 /etc/systemd/system/shutdown.service`

Install shutdown.service:
`sudo systemctl daemon-reload`

`sudo systemctl enable shutdown`

`sudo systemctl start shutdown`


``` bash
sudo cp ./shutdown.py /usr/bin/shutdown.py
sudo chown root:root /usr/bin/shutdown.py
sudo chmod 0755 /usr/bin/shutdown.py
sudo cp ./scripts/shutdown.service /etc/systemd/system/shutdown.service
sudo chown root:root /etc/systemd/system/shutdown.service
sudo chmod 0644 /etc/systemd/system/shutdown.service
sudo systemctl daemon-reload
sudo systemctl enable shutdown
sudo systemctl restart shutdown
```
