## install ROOT [v6.30 currently checked] 

install libusb library:

`sudo apt-get install libusb-1.0-0-dev`

## download/copy CAEN libraries: 
https://www.caen.it/download/

CAENVMELib (v3.4.4), CAENComm (v1.6.0), CAENDigitizer (v2.17.3) and driver for DT5720 (CAENUSBdrvB-1.5.4)

### install driver (CAENUSBdrvB-1.5.4) // it can be tricky

previously check BIOS setting of Secure Boot (should be OFF) 

update Linux

`sudo apt install dwarves`

check kernel version

`uname -srn`

Currently driver works really well at the versions <= 6.2

compile and install driver
`sudo make`
`sudo make install`

if everything fine, you will see two lines:

`“installing CAENUSBdrvB driver .. please wait”`

`“installation done”`

check installed Driver by command: **lsmod | grep CAEN** (in the output you should find CAENUSBdrvB line)

now you can check USB port number where connected digitizer:
- switch on digitizer
- type commands :

`cd /dev/usb`

`ls`

and you should see output with few variables, for us **v1713_N** is interested. N – is number of USB port which should be filled in configure file in line:
`OPEN USB N 0`

### To avoid some errors during driver installation:
>Skipping BTF generation for 	/home/user/Downloads/CAENUSBdrvB-1.5.4/CAENUSBdrvB.ko due to unavailability of vmlinux or any problems with vmlinux after updates

`sudo cp /sys/kernel/btf/vmlinux /usr/lib/modules/`uname -r`/build/`

_modeprobe errore is usually connected with secure boot of BIOS settings._

## install CAEN libraries 
`sudo ./install_64`

in /lib folder of every library (CAENVMELib, CAENComm and CAENDigitizer)

## copy and complie DTRoot
`git clone -b master https://github.com/DrKazlou/DTRoot.git`

- go to DTRoot folder and compile program by typing **make**
- launch DTRoot: ./DTRoot
- Have fun.
