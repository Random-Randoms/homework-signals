# Lab
Author: Daniil Savinov B09, 08/04/2025.

Code is located in `main.c`.

# Usage
## Build
Echo-server can be built with Makefile's `all` target.

## Launch
Following launch parameters can be provided with flags:
| Parameter             | Flag                  | Default value         |
| --------------------- | --------------------- | --------------------- |
| source pipe           | `--src <filename>`    | fails if not provided | 
| log file              | `--log <filename>`    | `default.log`         |
| statistsics frequency | `-N <integer number>` | 1 second              |
| verbosity             | `-V`                  | non verbose           |

## Interaction
After being launched, the app starts reading pipe in blocks of `1KB`.

### Echo
Each read block of bytes will be written in the console, if app is in foreground mode, or in the log file if it is a daemon.

### Statistics
After each `N` seconds it will print statistics in the following format:
```
[reason] opens: <number>, bytes: <number>, alarms: <number>
```
Where `reason` is the reason why statistics were printed (can be `daemon` after daemonizing, `user` after user signal, and `alarm` after alarm), `opens` is a number of pipe opens, `bytes` is a number of total read bytes, and `alarms` is the number of alarm triggerings.

### Signals
App reacts on several signals:
| Signal               | Reaction                              |
| -------------------- | ------------------------------------- |
| `SIGINT`, `SIGTERM`  | finish safely                         |
| `SIGHUP`             | daemonize                             |
| `SIGQUIT`            | ignore                                |
| `SIGUSR1`            | print statistics (with `user` reason) |

# Tests
## Daemon test
Code of the test is in the [`daemon-test.sh`](tests/daemon-test.sh) file. It is supposed to be run from [`tests`](tests) directory.

Test output if following:
```Shell
$ bash daemon-test.sh
> clean working directory
> make: Entering directory '/home/unix/os-homeworks/homework-signals'
> rm -f main.o
> rm -f echo-server
> make: Leaving directory '/home/unix/os-homeworks/homework-signals'
> 
> build echo-server with make
> make: Entering directory '/home/unix/os-homeworks/homework-signals'
> gcc -c main.c -o main.o
> gcc main.o -o echo-server
> make: Leaving directory '/home/unix/os-homeworks/homework-signals'
> 
> launch echo-server
> is daemon   | YES
> source pipe | foo
> log file    | foo.log
> stat freq   | 5 seconds
> verbose     | NO
> 
> put line into the pipe
> 
> put LF-terminated line into the pipe
> 
> send SIGUSR1 to the file
> 
> sleep for 11 seconds to get statistics twice
> 
> try to quit the process
> 
> term the process
> 
> load artifacts into file daemon-test-results.txt
```

Content of echo-server log is in the [`daemon-test-results.txt`](tests/daemon-test-results.txt).

## Foreground test
Code of the test is in the [`foreground-test.sh`](tests/foreground-test.sh) file. It is supposed to be run from [`tests`](tests) directory.

Test output if following:
```Shell
$ bash foreground-test.sh
> clean working directory
> make: Entering directory '/home/unix/os-homeworks/homework-signals'
> rm -f main.o
> rm -f echo-server
> make: Leaving directory '/home/unix/os-homeworks/homework-signals'
> 
> build echo-server with make
> make: Entering directory '/home/unix/os-homeworks/homework-signals'
> gcc -c main.c -o main.o
> gcc main.o -o echo-server
> make: Leaving directory '/home/unix/os-homeworks/homework-signals'
> 
> launch background signaller
> 
> launch echo-server
> is daemon   | NO
> source pipe | foo
> log file    | foo.log
> stat freq   | 5 seconds
> verbose     | YES
> [alarm ] opens:    0, bytes:          0, alarms:    1
> bombardiro crocodilo
> [user  ] opens:    1, bytes:         21, alarms:    1
> daemon id: 3920
> 
> process was daemonized, wait till it ends...
> 
> load artifacts into file foreground-test-results.txt
```

Content of echo-server log is in the [`foreground-test-results.txt`](tests/foreground-test-results.txt).


# What was hard about the lab
The tricky part was to test the foreground mode --- with terminal bound to our process, I was not able to directly send signals to it, so I decided to launch foreground process, called [`background-signaller.sh`](tests/background-signaller.sh), that sends signals for me from the background.
