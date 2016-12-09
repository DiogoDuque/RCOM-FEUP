/etc/init.d/networking restart
ifconfig eth0 172.16.10.1/24

route add default gw 172.16.10.254
