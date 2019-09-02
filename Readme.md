# Secure Mood Tracker

## Purpose

To win a contest? [insert link to Hackster.io contest]

Not really.  Well, maybe?  It certainly was the initiating incident for this project.  

The project is an example of how to not only obtain telemetry data with an MT3620 based Azure Sphere device, but also how to consume and visualize the data in an extremly secure manner.

The project will track the mood of the office and report it to the cloud for further analysis.  

The project will have the following features:

* Display inviting a passer by to indicate their current mood by pressing one of 3 buttons.
* Good: Press the Green button.
* Average: Press the Yellow button.
* Bad(less than average, etc...): Press the Red button.
* The buttons will flash randomly to attrack attention
* There will be a proximity detector on the device to detect when there is motion in front of the device.  That way we can have some sort of data of how many times a person was in range of the device and a button was or was not pushed.
* Current temperature and humidity.  (Current IoT international by laws mandate this type of measurement.)
* Will also provide any other builtin sensor data that may be useful when the data is further analyzed.

## Design Considerations

* Must be created using the Azure Sphere MT3620 Starter Kit <https://www.element14.com/community/community/designcenter/azure-sphere-starter-kits>
* Device must be secure from end to end.

## Challenges

There are several challenges in this design.

### Not enough GPIO on the dev board

There are a number of GPIO pins on the Azure Sphere module, most of them are not directly accessible.

The dev board is setup to use the mikroBUS Click Modules.  It has a number of buttons, LEDS, Grove connectors, and more that consume most if not all of the easily available GPIO pins.

I have decided to solve that problem with the addition of a MCP23017 GPIO extender.  This gives me and additional 16 pins of GPIO, which is way more than required.

There is a Click module that holds MCP23017, I decided that using a generica module would be a more educational experience.  

There is a also a problem with libraries to access the expanded GPIO.  They don't currently exist in the Azure Sphere eco system.  There are libraries for the Click module using the Avent development system, but none specifically for the Azure Sphere module.

I the development board team has provided a reference application that accesses the various I2C modules on the dev board.  I plan on using that as a pattern to build up my own libarary to operate the MCP23017 over I2C.

### Attracking people to the device

The lights in the buttons can be controlled independently of the switch activation.  The lights will flash in various patterns to attract attention.

The device will be placed in a high traffic area.

The display will rotate with with data and instructions.

When the proximity sensor goes active the instructions will be display.

## Requirements

### Software

* Azure subscription
* Visual Studio 2019 Community Edition
* See the startup stuff...

### Hardware

* MT3620 Developement Board
* Red LED Mini Arcade Button http://adafru.it/3430
* Yellow LED Mini Arcade Button http://adafru.it/3430
* Green LED Mini Arcade Button http://adafru.it/3430
* MCP23017 breakout board
* 3 x 2N222 NPN transistors
* 3 x 220 ohm resistors
* USB Micro b jack to USB micro b plug http://adafru.it/5217
* Mini breadboard
* 3D printed case
* Power supply withe a micro b connector

### Tools

* Soldering Iron
* Digital Multimeter
* Jumper wires

## Build

### Software Setup

### Setup the MT3620

### Prep the buttons

### Connect up the MCP23017

### Connect the Oled

### Main code

### Print the enclosure
