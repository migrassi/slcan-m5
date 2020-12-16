#!/bin/sh
sudo slcan_attach -f -s5 -o -n canable0 /dev/rfcomm0
sudo slcand -S 115200 rfcomm0 canable0  
sudo ifconfig canable0 up
