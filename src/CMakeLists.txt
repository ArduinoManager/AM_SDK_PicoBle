add_library(AM_SDK_PicoBle INTERFACE)

#pico_generate_pio_header(no-OS-FatFS-SD-SDIO-SPI-RPi-Pico ${CMAKE_CURRENT_LIST_DIR}/sd_driver/SDIO/rp2040_sdio.pio)

target_compile_definitions(AM_SDK_PicoBle INTERFACE
    PICO_MAX_SHARED_IRQ_HANDLERS=8u
)
# target_compile_options(no-OS-FatFS-SD-SDIO-SPI-RPi-Pico INTERFACE -ffile-prefix-map=${CMAKE_CURRENT_LIST_DIR}=)

target_sources(AM_SDK_PicoBle INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/AM_SDK_PicoBle.cpp
    ${CMAKE_CURRENT_LIST_DIR}/AM_SDManager.cpp
    ${CMAKE_CURRENT_LIST_DIR}/AM_Alarms.cpp
    ${CMAKE_CURRENT_LIST_DIR}/hw_config.cpp
)

add_subdirectory(../../no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/src build)

target_include_directories(AM_SDK_PicoBle INTERFACE
    include
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/build/build/generated/AM_SDK_PicoBle_gatt_header
)

target_link_libraries(
    pico_stdlib INTERFACE
    pico_btstack_cyw43 INTERFACE
    pico_cyw43_arch_none INTERFACE
    pico_btstack_ble INTERFACE
    no-OS-FatFS-SD-SDIO-SPI-RPi-Pico INTERFACE
)

pico_btstack_make_gatt_header(AM_SDK_PicoBle INTERFACE "${CMAKE_CURRENT_LIST_DIR}/gap_configuration.gatt")


target_compile_definitions(AM_SDK_PicoBle INTERFACE
    #DEBUG         # Uncomment this line to debug the library
    #DUMP_ALARMS   # Uncomment this line to dump the alarms
    #DEBUG_SD      # Uncomment this line to debug SD Card operations
)


