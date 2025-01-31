#!/bin/sh
#
# Script for loading the bootloader into a modem in standard mode
#
# Script parameter - tty port name, default - /dev/ttyUSB0
#
PORT=$1
if [ -z "$PORT" ]; then PORT=/dev/ttyUSB0; fi

LDR=$2
if [ -z "$LDR" ]; then LDR=loaders/NPRG8200p.bin; fi


echo Diagnostic port: $PORT

# We are waiting for the port to appear in the system
while [ ! -c $PORT ]
 do
  sleep 1
 done

# command to switch to download mode
echo entering download mode...
./qcommand -p $PORT -c "c b" >/dev/null

# We are waiting for the port to disappear from the system
while [ -c $PORT ]
 do
  true
 done
echo diagnostic port removed

# We are waiting for the new port to appear, download already
while [ ! -c $PORT ]
 do
  sleep 1
 done

# Loading the bootloader
echo download mode entered
sleep 2
./qdload -p $PORT  -i -t -k1 -d20 -a100000 $LDR
