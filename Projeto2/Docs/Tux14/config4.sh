/etc/init.d/networking restart
ifconfig eth0 172.16.10.254/24
ifconfig eth1 172.16.11.253/24

route add default gw 172.16.11.254
route add -net 172.16.10.0/24 gw 0.0.0.0
route add -net 172.16.11.0/24 gw 0.0.0.0

echo 1 > /proc/sys/net/ipv4/ip_forward
