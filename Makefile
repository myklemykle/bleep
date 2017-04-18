# Setup for Arduino-Makefile: https://github.com/sudar/Arduino-Makefile
# (as installed by homebrew under Mac OS X)

ARDUINO_DIR = /Applications/Utilities/Arduino1.8.2.app/Contents/Java
ARDMK_DIR = /usr/local/opt/arduino-mk
BOARD_TAG = teensy36

# # Use the tools distributed with Arduino & Teensyduino:
AVR_TOOLS_DIR = $(ARDUINO_DIR)/Contents/Java/hardware/tools/arm

# stuff we would turn on via the arduino IDE
#USB_TYPE = USB_MIDI_AUDIO_SERIAL
#USB_TYPE = USB_MIDI_AUDIO
USB_TYPE = USB_MIDI_SERIAL

#### Arduino-Makefile! 
include $(ARDMK_DIR)/Teensy.mk


TARGET = bleep

program: build-$(BOARD_TAG)/$(TARGET).hex
	teensy_loader_cli -mmcu=mk66fx1m0 -v -w build-$(BOARD_TAG)/$(TARGET).hex
