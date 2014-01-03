TinyRGB
=======

##Abstract

TinyRGB is a small learning project of mine. The goal is to replace / provide remote controlled drivers of 
RGB strips by a small Bluetooth-Enabled controller and driver. 

##Design Considerations
The board layout has been restricted to 5x5cm so you can get your PCBs produced for instance by Seeedstudio for
very little investment. The ATTiny4313 microcontroller has been chosen with the Arduino-Community in mind. While 
this chip is not compatible with your Arduino-IDE per se, it is very closely related to it and should make it 
a little bit more accessible. The HC-05 Bluetooth module used is one of the most popular ones across the hacker 
communities and can be sourced for few Dollar/Euro from eBay. 

##Protocol Specification

The TinyRGB Firmware supports 3 basic modes: 

- Random RGB Fade Mode
- Buggy (Jitter) HSV Fader Mode 
- Fixed RGB Mode

The modes can be configured using a simple and compact serial protocol which makes some sacrifices
in order to maintain "human" compatibility while being designed to be remote controlled by a rich
GUI application. 

The implementation supports the following commands sent via a serial modem link (8 9600 8n1 None):
```
"SRGB:RRRGGGBBB"      set r / g / b values, implicit change to mode 002 - fixed RGB.
"SW:WWW"              set white level (unsupported by the current hardware)
"SO:±RR±GG±BB"        set offset r / g / b for RGB Fade Mode (+/-99)
"SM:RRRGGGBBB"        set maximum r / g / b for RGB Fade Mode
"SSV:SSSVVV"          set SV for HSV Fade mode
"SMD:MMM"             set mode (001 - random rgb fade, 002 - fixed RGB, 003 - HSV Fade (buggy))
"SD:DDD"              set delay for fade modes.
"SAV:VVV"             enables (VVV > 0) or disables (VVV == 0) the eeprom autosave function.
"status"              get current rgb values and mode
"help"                help screen.
```

Setting all HSV registers is not exposed via remote API yet due to bugs in the float-less implementation of the 
HSV -> RGB algorithm. The ATTiny2313 family does not have a FPU unit and SoftFPU is out of question considering 
the limited resources on the chip. Thus, the conversion from HSV to RGB should take place on the controller side 
(for instance an mobile application). The HSV support is likely to be dropped completely and replaced by a 
RGB colorwheel scroller. 

If you want to implement a continuous color cycling or other effects on the controller side, disabling the eeprom 
(via "SAV:000") is highly recommended to prevent the eeprom wearing. This mode is not persistent.

##Current Status 
###Hardware

The hardware can be considered to be beta at best. Current issues are:

- [ ] 7805 regulator needs to be Rotated by 180° to prevent issues with heatsink mounting
- [ ] Better decoupling
- [ ] Consider to upgrade the PCB Size to 5x10cm.

 
###Firmware

The firmware is developed using the AVR-GCC tool chain and is somewhere between alpha and beta stages. 

- [x] Provide RGB / HSV methods to control the RGB Values
- [x] RGB fading / demo mode 
- [x] Establish simple serial communication over Bluetooth (Echo service)
- [x] Serial protocol specification to control and query the RGB values / modes.
- [x] Serial protocol implementation
- [x] (re)store last state from/in eeprom
- [ ] Fix HSV converter, behaves wonky in HSV fade mode with SV values other than 254/255 (unlikely)
- [ ] Provide unix makefiles and integration with avrdude
- [ ] try to improve power consumption in pairing mode, 7805 regulator will be running quite hot otherwise.


