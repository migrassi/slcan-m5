#!/bin/sh
sudo ifconfig canable0 down
sudo slcand -S 115200 -c rfcomm0 canable0
sudo killall slcand
