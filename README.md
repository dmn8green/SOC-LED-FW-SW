# Light Controller Firmware

## Name
Choose a self-explaining name for your project.

## Description
ESP32 Firmware for the MN8 Light bar controller.
Firmware is for the ESP32-wrover-ie chip.

## Badges
On some READMEs, you may see small images that convey metadata, such as whether or not all the tests are passing for the project. You can use Shields to add some to your README. Many services also have instructions for adding a badge.

## Visuals
Depending on what you are making, it can be a good idea to include screenshots or even a video (you'll frequently see GIFs rather than actual videos). Tools like ttygif can help, but check out Asciinema for a more sophisticated method.

## Installation

## Usage

## Support
Tell people where they can go to for help. It can be any combination of an issue tracker, a chat room, an email address, etc.

## Roadmap
If you have ideas for releases in the future, it is a good idea to list them in the README.

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
<!-- For open source projects, say how it is licensed. -->

## Project status
<!-- If you have run out of energy or time for your project, put a note at the top of the README saying that development has slowed down or stopped completely. Someone may choose to fork your project or volunteer to step in as a maintainer or owner, allowing your project to keep going. You can also make an explicit request for maintainers. -->
