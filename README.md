# Nixdorf LK-3000
Information about the Nixdorf LK-3000 personal computer.<br>

The cartridges use the Mostek MK3870 CPU and MK3600 ROM.  The CPU contains its own ROM and RAM which can be [dumped](https://github.com/blackjetrock/dumper_m3780) thanks to the existence of a TEST pin!<br>

![Nixdorf LK-3000 personal computer](/Images/Nixdorf_LK-3000.png)

## Videos
- [Part 1: first look & disassembly](https://youtu.be/9jknng2B5vs)
- [Part 2: let's start hacking it](https://youtu.be/iGiR7imZPvE)

## [Arduino Code](/Arduino/Test/Test.ino)
The first new software written for the LK-3000 in 40 years?!  Some test code to see how it works - replaced the cartridge connector with a 16-way pin header for easy wiring up to an Arduino.<br>

Check also the [July 1980 issue of Popular Electronics](/Documentation/Poptronics_July_1980.pdf) that describes how to re-use the LK-3000.

## [Dumping the MK3870 firmware](https://hackaday.com/tag/mk3870/)

## [Schematic](/LK3000_Schematics)
A schematic (KiCad 9) of the LK-3000.  The machine is essentially just a keyboard & display with the cartridges containing the processor, ROM and RAM (as appropriate).

## [Keyboard](/LK3000_Keyboard)
With the machine out of the case for testing/development with the Arduino, the original keyboard is not so easy to use.  So here's a simple replacement using Gateron KS-33 low profile key switches.

## [Breakout Cartridge](/LK3000_Breakout_Cartridge)
A design for a cartridge with breakout pin header so an external device can be connected for development/testing.

![LK-3000 breakout cartridge](/Images/Nixdorf_LK-3000_Breakout_Cartridge.png)

## [Prototyping Cartridge](/LK3000_Prototyping_Cartridge)
A design for a cartridge with prototyping area.

![LK-3000 prototyping cartridge](/Images/Nixdorf_LK-3000_Prototyping_Cartridge.png)

## [Logic Analyser Traces](/Saleae)
Signalling traces taken from the cartridge interface.  These were captured with the [Saleae logic analyser](https://www.saleae.com/) - their Logic 2 software is free to download & should open the file in "demo mode" (no need for the actual hardware).

## DL-1414 Display Modules
The LK-3000 uses four 4-character display modules (16 characters in total).  These parts are obsolete now but can still be found ... including in the LK-3000 obviously.<br>

![DL1414 Display Module](/Images/Nixdorf_LK3000_DL1414.jpg)

There is an [Arduino library](https://github.com/marecl/HPDL1414) available to drive the modules directly.

## Cartridges
These are the cartridges that I'm aware of:
- LK-0280: Olympic Facts (Winter, Lake Placid 1980)
- LK-0680: Olympic Facts (Summer, Moscow 1980)
- LK-1001: Filing System
- LK-3050: English-Spanish
- LK-3060: English-French
- LK-3070: English-Italian
- LK-3080: English-German
- LK-3090: English-Swedish
- LK-3100: English-Polish
- LK-3110: English-Portuguese
- LK-3120: English-Russian
- LK-3130: English-Greek
- LK-3140: English-Arabic
- LK-3145: English-Arabic II
- LK-3160: English-Japanese
- LK-3200: English-Spanish-French-German-Italian-Greek
- LK-3500: Electronic Notepad
- LK-3900: Calculator

### LK-3050 Spanish Language 
Simple cartridge with processor (containing its own ROM & RAM) plus language ROM.
![LK-3050 Cartridge PCB](/Images/Nixdorf_LK3000_LK3050_PCB.png)

### LK-3500 Electronic Notepad
A more sophisticated cartridge with processor & rechargeable (NiCd) battery-backed RAM.
![LK-3500 Cartridge PCB](/Images/Nixdorf_LK3000_LK3500_PCB.png)
