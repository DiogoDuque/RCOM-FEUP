/etc/init.d/networking restart
#ifconfig eth0 up
ifconfig eth0 172.16.10.1/24

route add default gw 172.16.10.254
route add -net 172.16.10.0/24 gw 0.0.0.0
route add -net 172.16.11.0/24 gw 176.16.10.254
