#!/bin/bash

echo "clean working directory"
bash clean.sh

echo
echo "build echo-server with make"
make -C ..

pipe=foo
log=foo.log 
artfs=foreground-test-results.txt
freq=5

echo
echo "launch background signaller"
bash background-signaller.sh $pipe &

echo
echo "launch echo-server"
echo "is daemon   | NO"
echo "source pipe | ${pipe}"
echo "log file    | ${log}"
echo "stat freq   | ${freq} seconds"
echo "verbose     | YES"
./../echo-server --src $pipe --log $log -N $freq -H

echo
echo "process was daemonized, wait till it ends..."
sleep 7

echo
echo "load artifacts into file ${artfs}"
> $artfs
cat $log > $artfs
