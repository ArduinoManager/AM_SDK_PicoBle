# AM_SDK_PicoBle

This C++ library is designed to interface with the Arduino Manager app on iOS and macOS.

 * Supported Boards: Pico W & Pico 2 W
 * Protocol: BLE

## Arduino Manager

Arduino Manager allows you to control and receive data from any Arduino or Arduino compatible microcontroller and Raspberry Pico. It provides:

* Over 30 customizable widgets (such as switch, knob, slider, display, gauge, bar and more)
* A built-in Code Generator for quickly and easily generating the code

More information available here:

- iOS: https://sites.google.com/site/fabboco/home/arduino-manager-for-iphone-ipad
- macOS: https://sites.google.com/site/fabboco/home/arduino-manager-for-mac


### SD Library

This library leverages the no-OS-FatFS-SD-SDIO-SPI-RPi-Pico library to manage the SD Card.

https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico

## How to configure a manual generated project

1) Open Visual Studio Code

2) Using the Raspberry Pico extension create a new Project:

    - Project Name: [Project Name]
    - Board Type: Pico W or Pico 2 W
    - Location: any folder of choice
    - Pico SDK Version: 2.1.0 or greater
    - Stdio Support: Console over UART, Console over USB or both, depending on your setup
    - Wireless Options: Pico W onboard LED
    - Generate C++ code
    - DebugProbe or SWD depending on your setup

3) Create a new folder named lib

4) Right click the lib folder and select Open in Integrated Terminal

5) Enter the following commands:

```
git clone --recurse-submodules https://github.com/ArduinoManager/AM_SDK_PicoBle
git clone --recurse-submodules https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico.git
cd no-OS-FatFS-SD-SDIO-SPI-RPi-Pico
git switch --detach tags/v3.6.2
```

6) Copy the content of:

```
lib/examples/skeleton/skeleton.cpp
```

to your main file as starting point

7) Make the following changes to CMakeLists.txt

```

pico_sdk_init()
add_subdirectory(lib/AM_SDK_PicoWiFi/src build)
…
…

target_link_libraries(…
      pico_stdlib
      hardware_adc
      hardware_pwm
      AM_PicoBle
      )
…
…

target_include_directories(example2 PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/..
  lib/AM_SDK_PicoBle/include
)

```

8) Delete build folder if exists

9) From the Raspberry Pico extension select Configure CMake

10) Press F5 to run the program

## How to configure a Arduino Manager Code Generator project

1) Generate the code using the Arduino Manager Code Generator

2) Open Visual Studio Code and using the Raspberry Pico extension import the project

3) Create a new folder named lib

4) Right click the lib folder and select Open in Integrated Terminal

5) enter the following commands:

```
git clone --recurse-submodules https://github.com/ArduinoManager/AM_SDK_PicoBle
git clone --recurse-submodules https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico.git
cd no-OS-FatFS-SD-SDIO-SPI-RPi-Pico
git switch --detach tags/v3.6.2
```
6) From the Raspberry Pico extension select Configure CMake

7) Press F5 to run the program

## How to Debug the Library

1) Open the file

```
 lib/AM_SDK_PicoBle/src/CMakeLists.txt
```
2) Uncomment the required defines:

```
    # DEBUG         # Uncomment this line to debug the library
    # DUMP_ALARMS   # Uncomment this line to dump the alarms
    # DEBUG_SD      # Uncomment this line to debug SD Card operations
```

## How to Debug the SD Library

1) Open the file
```
 no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/include/my_debug.h
 ```
2) add the following lines

```
    #define USE_PRINTF 1
    #define USE_DBG_PRINTF 1
```

## Update the library

Go to the AM_SDK_PicoBle folder and enter:

```
git pull origin main
```