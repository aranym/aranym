ifconfig eth0 addr `./araethip --get-atari-ip eth0` netmask `./araethip --get-netmask eth0` up
route add default eth0 gw `./araethip --get-host-ip eth0`
