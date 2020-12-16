#!/bin/sh
sudo slcan_attach -f -s5 -o -n canable0 /dev/ttyOP_slcan
sudo slcand -S 921600 ttyOP_slcan canable0  
sudo ifconfig canable0 up
