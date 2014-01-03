TinyRGB
=======

##Abstract

TinyRGB is a small learning project of mine. The goal is to replace / provide remote controlled drivers of RGB strips
by a small Bluetooth-Enabled controller and driver. 

##Design Considerations
The board layout has been restricted to 5x5cm so you can get your PCBs produced for instance by Seedstudio for
very little investment. The ATTiny4313 microcontroller has been chosen with the Arduino-Community in mind. While 
this chip is not compatible with your Arduino-IDE per se, it is very closely related to it and should make it 
a little bit more accessible. The HC-05 Bluetooth module used is one of the most popular ones across the hacker communities
and can be sourced for few Dollar/Euro from eBay. 

##Current Status 
###Hardware

The hardware can be considered to be beta at best. Current issues are:

- [ ] 7805 regulator needs to be Rotated by 180° to prevent issues with heatsink mounting
- [ ] Better decoupling
- [ ] Consider to upgrade the PCB Size to 5x10cm.
 
###Firmware

The firmware is developed using the AVR-GCC toolchain and is work in progress:

- [x] Provide RGB / HSV methods to control the RGB Values
- [x] RGB fading / demo mode 
- [x] Establish simple serial communication over Bluetooth (Echo service)
- [ ] Serial protocol specification to control and query the RGB values / modes.
- [ ] Serial protocol implementation
- [ ] Provide unix makefiles and integration with avrdude




