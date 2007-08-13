Setting up ethernet on the MAC (0.9.5 beta)
===========================================


1. Install TUN/TAP driver
-------------------------
Download the tuntap driver from

http://www-user.rhrk.uni-kl.de/~nissler/tuntap/

unpack the binary version and use the tuntap_installer.



2. Prepare aratapif.sh
----------------------
Edit the aratapif.sh and modify the following parameters
to your requirements.

FW_INTERFACE : the interface which is your normal MAC network device
               may be en0 for a wired RJ45 and en1 for a WLAN connection
NAMESERVER	 : Point this to your normal name server

Than open a terminal window and  copy the file aratapif.sh to the directory /usr/local/bin

$ cd {Directory where aratapif.sh is located}
$ sudo cp aratapif.sh /usr/local/bin



3. Setup sudoers file
---------------------
Now modify the rights to execute the aratapif.sh

$ sudo visudo

Add the following line to the file

ALL ALL = NOPASSWD: /usr/local/bin/aratapif.sh

to do this. Press the following key codes:

- Shift+G
- o
- Copy the above line via clipboard
- ESC
- x<RETURN>


4. Configuring MacAranym
------------------------
Configure your MacAranym config file and modify the ETH0 section

[ETH0]
Type = ptp
Tunnel = tap0
HostIP = 192.168.1.8
AtariIP = 192.168.1.9
Netmask = 255.255.255.0
MAC = 00:41:45:54:48:30

Set the HostIP and AtariIP to a network that is outside of your normal
network range. i.E. if your default network is 172.16.1.xxx than you may
use i.E. 192.168.1.x, or if your normal network is 192.168.1.x you may
use 172.16.1.xxx addresses.


5. Prepare Guest OS side
------------------------
Edit the /etc/resolv.conf in the ATARI file system and point it to your
host system (in the above example to 192.168.1.8)


6. Testing the connection
-------------------------
Boot up MacAranym. After startup open a terminal window on Mac side
and try to send a ping to the Guest IP address, i.E.

$ ping 192.168.1.9


6. Joy the Ethernet connection
------------------------------
;-)



Troubleshooting
---------------
If the device /dev/tap0 does not having the correct access flags
modify the file /Library/StartupItems/tap/tap and add a chmod to
the StartService section. After that it should look like the one
below:


StartService ()
{
        ConsoleMessage "Initializing tap devices"

        if [ -d /Library/Extensions/tap.kext ]; then
                kextload /Library/Extensions/tap.kext
                chmod a+rw /dev/tap0
        fi
}


