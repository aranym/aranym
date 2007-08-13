#!/bin/sh

# 
# Network setup script of MacAranym
# based on the QEMU network script by Mattias Nissler 
#
# Download TUN TAP driver from
#
#	http://www-user.rhrk.uni-kl.de/~nissler/tuntap/
#

DEVICE=$1
IP_HOST=$2
IP_ATARI=$3
NETMASK=$4
MTU=$5

FW_INTERFACE=en1
FWNATD_PORT=8668
IPFW_RULENUM=00200

# optional dns forwarding
DNS_FORWARD=1
DNSFW_RULENUM=00100
DNSFWNATD_PORT=8669
NAMESERVER=172.16.1.2

# no need to change anything below this line

# bring the interface up
/sbin/ifconfig $DEVICE $IP_HOST netmask $NETMASK mtu $MTU up

# enable forwarding
sysctl -w 'net.inet.ip.forwarding=1'

# delete old rules
/sbin/ipfw delete $IPFW_RULENUM

# install the firewall nat rules
/sbin/ipfw add $IPFW_RULENUM divert $FWNATD_PORT all from $IP_ATARI to any out via $FW_INTERFACE 
/sbin/ipfw add $IPFW_RULENUM divert $FWNATD_PORT all from any to any in via $FW_INTERFACE

# kill an old natd if necessary
if test -f /var/run/natd_fw.pid; then
	kill -9 `cat /var/run/natd_fw.pid`
fi

# start natd
/usr/sbin/natd -p $FWNATD_PORT -n $FW_INTERFACE -use_sockets -same_ports
sleep 1
cat /var/run/natd.pid > /var/run/natd_fw.pid

if test -n $DNS_FORWARD; then
	# delete old DNS forwarding rules
	/sbin/ipfw delete $DNSFW_RULENUM

	# install new rules
	/sbin/ipfw add $DNSFW_RULENUM divert $DNSFWNATD_PORT udp from $IP_ATARI to $IP_HOST 53 in via $DEVICE
	/sbin/ipfw add $DNSFW_RULENUM divert $DNSFWNATD_PORT udp from any 53 to $IP_ATARI out via $DEVICE

	# kill an old natd if necessary
	if test -f /var/run/natd_dnsfw.pid; then
		kill -9 `cat /var/run/natd_dnsfw.pid`
	fi

	sleep 5

	# start natd for forwarding
	/usr/sbin/natd -p $DNSFWNATD_PORT -n $DEVICE -redirect_port udp $NAMESERVER:53 $IP_HOST:53
	sleep 1
	cat /var/run/natd.pid > /var/run/natd_dnsfw.pid
fi

exit 0

