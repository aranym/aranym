Setting up ethernet on the MAC (0.9.5 beta)
===========================================


1. Install TUN/TAP driver
-------------------------
Download the tuntap driver from

http://www-user.rhrk.uni-kl.de/~nissler/tuntap/

unpack the binary version and use the tuntap_installer.

(If you don't want to use the installer there is a description how
you can install the drivers manually at the end of this document)

2. Prepare aratapif.sh
----------------------
Edit the aratapif.sh and modify the following parameters
to your requirements.

FW_INTERFACE : the interface which is your normal MAC network device
               may be en0 for a wired RJ45 and en1 for a WLAN connection
NAMESERVER	 : Point this to your normal name server

Copy the file into the directory where your "config" file is located.

3. Configuring MacAranym
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
use i.e. 192.168.1.x, or if your normal network is 192.168.1.x you may
use i.e. 172.16.1.xxx addresses.

If you want to disable ethernet, set "Type=none".

4. Prepare Guest OS side
------------------------
Edit the /etc/resolv.conf in the ATARI file system and point it to your
host system (in the above example to 192.168.1.8)


5. Testing the connection
-------------------------
Boot up MacAranym. After startup open a terminal window on Mac side
and try to send a ping to the Guest IP address, i.E.

$ ping 192.168.1.9


6. Joy your Ethernet connection
-------------------------------
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


Manuall installation (Experts)
------------------------------
If you don't like to use the installer for the TUNTAP extension you can
install them manually.

1. download the binary TUNTAP package for your system (panther/tiger).
2. unpack the archive to a temporary folder
3. right click the package tap_kext.pkg and select "Show package contents"
4. Go into the folder "Contents" and unpack the file Archive.pax.gz
by using a right click and selection "Open with ..." -> "BOMArchiveHelper"
5. Enter the directory Archive/Library/Extensions and copy tap.kext to 
a folder that you like
6. do steps 3 to 5 also with tun_kext.pkg and get tun.kext from it.
7. create a file i.E. load.sh in your destination directory where you
have put tun.kext and tap.kext with the following content:

#!/bin/sh

chown -R root:wheel tun.kext
chown -R root:wheel tap.kext
kextload tap.kext
kextload tun.kext
chmod a+rw /dev/tap[0-9]*

8. for unloading the device drivers from the kernel you need another
script (i.E. unload.sh) with the following content:

#!/bin/sh

/sbin/ipfw delete 00200

# kill an old natd if necessary
if test -f /var/run/natd_fw.pid; then
	kill -9 `cat /var/run/natd_fw.pid`
fi
# kill an old natd if necessary
if test -f /var/run/natd_dnsfw.pid; then
	kill -9 `cat /var/run/natd_dnsfw.pid`
fi
	
kextunload tap.kext
kextunload tun.kext

9. Loading the drivers before starting MacAranym

Open a terminal and change to the directory in which you have placed
your files.

$ cd mytuntapfolder

Start the load script with sudo

$ sudo ./load.sh

when prompted for the password use your user password.

9. for unloading the drivers after finishing MacAranym execute:

$ sudo ./unload.sh

