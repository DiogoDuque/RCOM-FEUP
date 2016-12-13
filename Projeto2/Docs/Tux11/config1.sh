/etc/init.d/networking restart
ip route flush table all
ifconfig eth0 172.16.10.1/24

route add default gw 172.16.10.254
