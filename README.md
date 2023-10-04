# Light Controller Firmware

## Name
LED strip controller firmware

## Description
ESP32 Firmware for an LED indicator for an EV charging station. This device is called the "Station Controller".  It registers via MQTT as an IOT device and receives updates via a Proxy Broker to drive LED status.

Firmware is for the ESP32-wrover-ie chip.

## Visuals
Overview of system (including this module) is in docs/SWDesign.md

## Installation

## Usage

## Support
Tell people where they can go to for help. It can be any combination of an issue tracker, a chat room, an email address, etc.

## Roadmap
## Contributing

### Prerequisites
To develop on this firmware you will need to install the ESP32 SDK by following [the ESP32 install instructions](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/linux-macos-setup.html)

Once installed, switch to the *5.1* branch
`git checkout release/v5.1`

### Getting the code

Clone the repo:
```
git clone https://gitlab.appliedlogix.com/mn8-energy/light-controller-firmware.git
```

### Compiling/flashing/monitoring

* Make sure you have sourced the ESP32 environment `source /path/to/esp/version/export.sh`
* Build using `idf.py build`
* Load on the board using `idf.py -p /dev/... flash`
* Monitor the board using `idf.py -p /dev/... monitor`

<!-- 
You can also document commands to lint the code or run tests. These steps help to ensure high code quality and reduce the likelihood that the changes inadvertently break something. Having instructions for running tests is especially helpful if it requires external setup, such as starting a Selenium server for testing in a browser. -->

### Developers' notes

#### *ESP_LOGX* macros will output the string to the UART associated with the current task. 

For instance, the REPL console UART will output every ESP_LOGX strings invoked within its own task, while the main loop will output to UART 0, the debug port.  Should we output everything to the same console UART?  Could get noisy for the user.

## Authors and acknowledgment
<!-- Show your appreciation to those who have contributed to the project. -->

## License
Property of MNE8 Energy.

## Project status
<!-- If you have run out of energy or time for your project, put a note at the top of the README saying that development has slowed down or stopped completely. Someone may choose to fork your project or volunteer to step in as a maintainer or owner, allowing your project to keep going. You can also make an explicit request for maintainers. -->
