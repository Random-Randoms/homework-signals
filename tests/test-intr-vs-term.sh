#!/bin/bash

echo "clean working directory"
bash clean.sh

echo
echo "build echo-server with make"
make -C ..

pipe=foo
log=foo.log 
artfs=intr-vs-term-test-results.txt
freq=5

echo
echo "launch echo-server"
echo "is daemon   | YES"
echo "source pipe | ${pipe}"
echo "log file    | ${log}"
echo "stat freq   | ${freq} seconds"
echo "verbose     | NO"
./../echo-server --daemon --src $pipe --log $log -N $freq

echo
echo "stop echo-server, put giant file in the pipe, term and continue echo-server"
pkill -SIGSTOP echo-server
cat  foo_source > $pipe&
pkill -SIGTERM echo-server
pkill -SIGCONT echo-server

echo
echo "load artifacts into file $artfs"
> $artfs
echo "stop with terminate:" >> $artfs
cat $log >> $artfs
echo 
echo "clean log"
rm -f foo.log

echo
echo "launch echo-server"
./../echo-server --daemon --src $pipe --log $log -N $freq

echo
echo "stop echo-server, put giant file in the pipe, interrupt and continue the process"
pkill -SIGSTOP echo-server
cat  foo_source > $pipe&
pkill -SIGINT echo-server
pkill -SIGCONT echo-server

echo
echo "load artifacts into file $artfs"
echo "stop with interrupt:" >> $artfs
cat $log >> $artfs
