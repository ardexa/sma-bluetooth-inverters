

# Purpose
The purpose of this project is to collect from SMA Bluetooth enabled Inverters and send the data to your cloud using Ardexa. Data from SMA solar inverters is read using a bluetooth connection and a Linux device such as a Raspberry Pi, or an X86 intel powered computer. 

## How does it work
This application is written in C++. It is based on the work of these people and projects:
a. https://github.com/sbf-/SBFspot
b.	https://github.com/sma-bluetooth/sma-bluetooth
c.	http://blog.jamesball.co.uk/2013/02/understanding-sma-bluetooth-protocol-in.html
d.	https://github.com/simonswine/opensunny

This application will query any number of connected SMA bluetooth inverters. Data will be written to log files on disk in a directory specified via the command line. Usage and command line parameters are as follows.

Usage: sudo ardexa-sma-bt -c conf file path [-p password -l log directory] [-d] [-v] [-i]
```
-l (optional) <directory> name for the location of the directory in which the logs will be written.  Defaults to: "/opt/ardexa/sma-bt/logs/"
-c (mandatory) <file path> fullpath of the SMA file (containing the address list of all bluetooth inverters)
-d (optional) if specified, debug will be turned on
-i (optional) discovery. Print a listing of all available SMA Bluetooth inverters and exit.
-v (optional) prints the version and exit.
-p (optional) sets the inverter password. Default password is "0000"
```

The `-c` option is a file that contains a list of all the SMA bluetooth devices that need to be queried. An example of this file is attached to this repository.

## Bluetooth Adapter
The Raspberry Pi or Linux machine must have a bluetooth adapater. To check the correct functioning of the adapter, install the `bluez` package, as follows:
- see this as a reference: https://www.pcsuggest.com/linux-bluetooth-setup-hcitool-bluez/
```
sudo apt-get update
sudo apt-get install -y bluetooth bluez bluez-tools rfkill
hcitool dev .... list local devices (ie; dongles connected to the local computer)
sudo hciconfig hci0 up
hcitool scan
hcitool info C4:D9:87:8F:3B:43  ... to get more information on device C4:D9:87:8F:3B:43
```

## Building the Ardexa software
To use this application, do the following. It will install the `ardexa-sma-bt` binary in the directory `/usr/local/bin`
```
sudo apt-get update
sudo apt-get install -y libbluetooth-dev
cd
git clone https://github.com/ardexa/sma-bluetooth-inverters.git
cd sma-bluetooth-inverters
mkdir build
cd build
cmake ..
make
sudo make install  ... will install the `ardexa-sma-bt` binary in /usr/local/bin
```

## Collecting to the Ardexa cloud
Collecting to the Ardexa cloud is free for up to 3 Raspberry Pis (or equivalent). Ardexa provides free agents for ARM, Intel x86 and MIPS based processors. To collect the data to the Ardexa cloud do the following:    
a. Create a `RUN` scenario to schedule the Ardexa SMA Bluetooth program to run at regular intervals (say every 180 seconds/3 minutes).    
b. Then use a `CAPTURE` scenario to collect the csv (comma separated) data from the directory `/opt/ardexa/sma-bt/logs/{inverter-name}/latest.csv`. This file contains a header entry (as the first line) that describes the CSV elements of the file.

## Help
Contact Ardexa, and we'll do our best efforts to help.

