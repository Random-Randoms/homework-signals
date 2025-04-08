#!/bin/bash

echo "clean working directory"
bash clean.sh

echo
echo "build echo-server with make"
make -C ..

pipe=foo
log=foo.log 
artfs=daemon-test-results.txt
freq=5

echo
echo "launch echo-server"
echo "is daemon   | YES"
echo "source pipe | ${pipe}"
echo "log file    | ${log}"
echo "stat freq   | ${freq} seconds"
echo "verbose     | NO"
id=$(./../echo-server --daemon --src $pipe --log $log -N $freq)

echo
echo "put line into the pipe"
echo "bombardiro crocodilo" > $pipe

echo
echo "put LF-terminated line into the pipe"
cat foo_source > $pipe

echo
echo "send SIGUSR1 to the file "
kill -s SIGUSR1 $id

echo
echo "sleep for 11 seconds to get statistics twice"
sleep 11s

echo
echo "try to quit the process"
kill -s SIGQUIT $id

echo
echo "term the process"
kill -s SIGTERM $id

echo
echo "load artifacts into file $artfs"
> $artfs
cat $log > $artfs
