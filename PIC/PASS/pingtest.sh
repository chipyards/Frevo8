#! /usr/bin/bash
ping -l 10 -n 3 $1
arp -d '*'
ping -l 20 -n 3 $1
ping -l 21 -n 3 $1
arp -d '*'
ping -l 32 -n 3 $1
ping -l 31 -n 3 $1
arp -d '*'
ping -l 40 -n 3 $1
ping -l 41 -n 3 $1
arp -d '*'
ping -l 50 -n 3 $1
ping -l 51 -n 3 $1
arp -d '*'
ping -l 64 -n 3 $1
ping -l 65 -n 3 $1
arp -d '*'
ping -l 70 -n 3 $1
ping -l 71 -n 3 $1
arp -d '*'
ping -l 170 -n 3 $1
ping -l 171 -n 3 $1
arp -d '*'
ping -l 180 -n 3 $1
ping -l 181 -n 3 $1
arp -d '*'
ping -l 190 -n 3 $1
ping -l 191 -n 3 $1
arp -d '*'
ping -l 200 -n 3 $1
ping -l 209 -n 3 $1
ping -l 210 -n 3 $1
arp -d '*'
echo '********** ping trop gros *********'
ping -l 211 -n 1 $1
ping -l 210 -n 3 $1
echo '********** ping trop gros *********'
ping -l 250 -n 1 $1
ping -l 209 -n 3 $1
echo '********** ping trop gros *********'
ping -l 1400 -n 1 $1
ping -l 206 -n 3 $1
echo '********** ping trop gros *********'
ping -l 10000 -n 1 $1
ping -l 205 -n 3 $1
