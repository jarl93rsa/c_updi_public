# c_updi_public
C UPDI driver for supported avr devices.

Almost entirely a direct translation of PyUPDI (https://github.com/mraardvark/pyupdi), rewritten in c, in a structure that better suits my needs and projects.
So many thanks to mraardvark for that work.
If you're looking for this you probably already know how to program over updi usb-uart if not look at PyUPDI for the connection diagram

Main.c contains example usage of the C_UPDI showing how to read and write flash and fuses, get the SIB, erase the device etc

Building on windows: gcc main.c -DUPDI_WIN32 win32\file.c win32\serial.c win32\time.c log.c updi.c -o main

Porting to a new platform should only require changes to file, serial, time files if I havn't stuffed up, which should then be placed in a new directory and the build command changed accordingly
e.g. gcc main.c -DUPDI_LINUX linux\file.c linux\serial.c linux\time.c log.c updi.c -o main
And make sure theres an #ifdef for your new platform in updi.h
A linux implementation will come soon when I get time to rewrite those basic functions, i've only needed a windows implementation thus far.

The log files are to handle output from the updi process to make implementing into various different projects easier. The provided log.c is more or less just a basic printf() implementation, with a bool VERBOSE to control which function's output are considered. 
The reason for this log feature is so that if you were to include c_updi into for example a GUI for firmware updating you could change the log functions to output to your GUI easily without havig to trawl through updi.c and change all outputs there.

I hope this is as useful to you as it has been for me.

Any issues or questions contact me on 
tyjean.com
jarl93rsa@gmail.com