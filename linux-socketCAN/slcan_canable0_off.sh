#!/bin/sh
sudo ifconfig canable0 down
sudo slcand -S 921600 -c ttyOP_slcan canable0
sudo killall slcand
