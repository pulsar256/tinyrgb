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
- HSL Fader Mode 
- Fixed RGB Mode

The modes can be configured using a simple and compact serial protocol which makes some sacrifices
in order to maintain "human" compatibility while being designed to be remote controlled by a rich
GUI application. 

The implementation supports the following commands sent via a serial modem link (8 9600 8n1 None):
```
"SRGB:RRRGGGBBB"      set r / g / b values, implicit change to mode 002 - fixed RGB/HSL.
"SW:WWW"              set white level (unsupported by the current hardware)
"SO:±RR±GG±BB"        set offset r / g / b for RGB Fade Mode (+/-99)
"SM:RRRGGGBBB"        set maximum r / g / b for RGB Fade Mode
"SHSL:HHHSSSLL"       set the HSL values. Pass -01 to ignore a value.
"SMD:MMM"             set mode (001 - random RGB fader, 002 - fixed RGB/HSL, 003 - HSL fader)
"SD:DDD"              set delay for fade modes.
"SAV:VVV"             enables (VVV > 0) or disables (VVV == 0) the eeprom autosave function.
"status"              get current RGB values and mode
"help"                help screen.
```

Setting HSL values does not put the controller into a fixed color mode. If you want to tweak partial HSL registers
while the colorcycling is active, you can pass a negative value to any color component to ignore it. For example 
adjusting the saturation while not disturbing the hue position can be executed as: "SHSL:-01128-01"

If you want to implement a continuous color cycling or other effects on the controller side, disabling the eeprom 
(via "SAV:000") is highly recommended to prevent the eeprom wearing. This mode is not persistent.

RGB offsets and maximum caps only apply to mode 002 (RGB Fader)

##Current Status 
###Hardware

The hardware can be considered to be beta at best. Current issues are:

- [ ] 7805 regulator needs to be Rotated by 180° to prevent issues with heatsink mounting
- [ ] Better decoupling
- [ ] Consider to upgrade the PCB Size to 5x10cm.

 
###Firmware

The firmware controls the RGB channels via PWM and stores its last state in the eeprom to retain
its settings on a power cycle. See the protocol specification for further details on implemented
features and modes.


Open issues:

- [ ] Provide unix makefiles and integration with avrdude
- [ ] try to improve power consumption in pairing mode, 7805 regulator will be running quite hot otherwise.
- [ ] change the HC-05 BT Module name to TinyRGB on first boot.
