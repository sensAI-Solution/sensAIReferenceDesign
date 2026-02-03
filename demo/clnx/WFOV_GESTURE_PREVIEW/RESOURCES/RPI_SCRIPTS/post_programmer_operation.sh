#!/bin/sh
#
echo "Set PROGRAMN HIGH..."
pinctrl set 25 op dh
sleep 0.1
echo "Set PROGRAMN LOW..."
pinctrl set 25 dl
sleep 0.1 # This will pause the script for 100 milliseconds (0.1 seconds)
echo "Set PROGRAMN HIGH...."
pinctrl set 25 ip pn

