## v2.9.0 (2024-02-24)

### Feat

- On loss of connectivity, after a reboot leds will stay dark

## v2.8.0 (2024-02-22)

### Feat

- When a device reboots due to a network issue, it won't be breating yellow, it will show no light

### Fix

- Adding a webserver to support firmware update build: removed cicd.  esp32 broke the build process in newer esp package and our code won't compile in docker anymore, no time to create a different package

## v2.7.8 (2024-02-20)

### Fix

- Fix what would be an issue with newer compiler
- Adding a webserver to support firmware update

## v2.7.7 (2024-01-19)

### Fix

- Fixed an issue with the command line to set led when not specifying charge

## v1.1.0 (2023-11-14)

## v1.0.0 (2023-10-23)
