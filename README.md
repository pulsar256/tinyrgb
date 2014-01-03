TinyRGB
=======

##Abstract

TinyRGB is a small learning project of mine. The goal is to replace / provide remote controlled drivers of RGB strips
by a small Bluetooth-Enabled controller and driver. 

##Design Considerations
The board layout has been restricted to 5x5cm so you can get your PCBs produced for instance by Seeedstudio for
very little investment. The ATTiny4313 microcontroller has been chosen with the Arduino-Community in mind. While 
this chip is not compatible with your Arduino-IDE per se, it is very closely related to it and should make it 
a little bit more accessible. The HC-05 Bluetooth module used is one of the most popular ones across the hacker communities
and can be sourced for few Dollar/Euro from eBay. 

##Protocol Specification

The TinyRGB Firmware supports 3 basic modes: 

- Random RGB Fade Mode
- Buggy HSV Fader
- Fixed RGB Mode

The modes can be configured using a simple and compact serial protocol which makes some sacrifices
in order to maintain "human" compatibility while being designed to be remote controlled by a rich
GUI application. 

The Implemenation supports the following commands sent via a serial modem link (8 9600 8n1 None):
```
"SRGB:RRRGGGBBB"      set r / g / b values, implicit change to mode 002 - fixed RGB.
"SW:WWW"              set white level (unsupported by the current hardware)
"SO:±RR±GG±BB"        set offset r / g / b for RGB Fade Mode (+/-99)
"SM:RRRGGGBBB"        set maximum r / g / b for RGB Fade Mode
"SSV:SSSVVV"          set SV for HSV Fade mode
"SMD:MMM"             set mode (001 - random rgb fade, 002 - fixed RGB, 003 - HSV Fade (buggy))
"SD:DDD"              set delay for fade modes.
"status"              get current rgb values and mode
"help"                help screen.
```

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
- [x] Serial protocol specification to control and query the RGB values / modes.
- [x] Serial protocol implementation
- [x] (re)store last state from/in eeprom
- [ ] Fix HSV converter, behaves wonky in hsv fade mode with SV values other than 254/255
- [ ] Provide unix makefiles and integration with avrdude
- [ ] try to improve power consumption in pairing mode, 7805 regulator will be running quite otherwise.


