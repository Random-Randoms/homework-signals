#!/bin/bash

# sleep to get one alarm
sleep 6

# put string into the pipe
echo "bombardiro crocodilo" > $1

# print stats
pkill -SIGUSR1 echo-server
sleep 1

# try to quit
pkill -SIGQUIT echo-server
sleep 1

# hang up
pkill -SIGHUP echo-server
sleep 1

# put string into the pipe
sleep 2
echo "tralalero tralala" > $1

# print stats
pkill -SIGUSR1 echo-server
sleep 3

# interrupt
pkill -SIGINT echo-server
sleep 1
