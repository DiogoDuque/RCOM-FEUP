ifconfig eth0 172.16.11.1/24
route add default gw 172.16.11.254
route add -net 172.16.10.0/24 gw 176.16.11.254
route add -net 172.16.11.0/24 gw 0.0.0.0
echo 0 > /proc/sys/net/ipv4/conf/eth0/accept_redirects
echo 0 > /proc/sys/net/ipv4/conf/all/accept_redirects