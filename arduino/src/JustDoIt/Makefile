### ARDUINO_DIR
### Path to the Arduino application and resources directory.
### For Arduino IDE 1.6.x ++
ARDUINO_DIR   = /Applications/Arduino.app/Contents/Java

### PROJECT_DIR
### This is the path to where you have created/cloned your project
PROJECT_DIR       = $(shell dirname $(shell dirname $(shell pwd)))

### ARDMK_DIR
### Path to the Arduino-Makefile directory.
ARDMK_DIR         = $(PROJECT_DIR)/Arduino-Makefile

### USER_LIB_PATH
### Path to where the your project's libraries are stored.
USER_LIB_PATH    :=  $(PROJECT_DIR)/lib

### BOARD_TAG & BOARD_SUB
### Only BOARD_TAG is needed. It must be set to the board you are currently using. (i.e uno, mega2560, etc.)
### for the Arduino Nano, only BOARD_TAG is mandatory and BOARD_SUB can be equal to anything
BOARD_TAG         = nano_33_iot

### MONITOR_PORT
### The port your board is connected to. Using an '*' tries all the ports and finds the right one.
MONITOR_PORT      = /dev/tty.usbmodem*

### MONITOR_BAUDRATE
### It must be set to Serial baudrate value you are using.
MONITOR_BAUDRATE  = 115200

### Include packages for boards installed via Arduino IDE (nano 33 iot)
ARDUINO_PACKAGE_DIR := $(HOME)/Library/Arduino15/packages

### OBJDIR
### This is were you put the binaries you just compile using 'make'
#CURRENT_DIR       = $(shell basename $(CURDIR))
OBJDIR            = $(PROJECT_DIR)/build/justdoit

### path to Arduino.mk, inside the ARDMK_DIR, don't touch.
include $(ARDMK_DIR)/Sam.mk

