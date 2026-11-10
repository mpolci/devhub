cmake_minimum_required(VERSION 3.21)

project(devhub_fw VERSION 0.1.0 LANGUAGES C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(TP ${CMAKE_CURRENT_SOURCE_DIR}/third_party)
set(HAL ${TP}/stm32g0xx_hal_driver)
set(CMSIS_DEV ${TP}/cmsis_device_g0)
set(TUSB ${TP}/tinyusb)

# --- Vendor code (built without project warnings) ---------------------------
add_library(vendor STATIC
  ${CMSIS_DEV}/Source/Templates/system_stm32g0xx.c
  ${HAL}/Src/stm32g0xx_hal.c
  ${HAL}/Src/stm32g0xx_hal_cortex.c
  ${HAL}/Src/stm32g0xx_hal_rcc.c
  ${HAL}/Src/stm32g0xx_hal_rcc_ex.c
  ${HAL}/Src/stm32g0xx_hal_gpio.c
  ${HAL}/Src/stm32g0xx_hal_pwr.c
  ${HAL}/Src/stm32g0xx_hal_pwr_ex.c
  ${HAL}/Src/stm32g0xx_hal_dma.c
  ${HAL}/Src/stm32g0xx_hal_i2c.c
  ${HAL}/Src/stm32g0xx_hal_i2c_ex.c
  ${HAL}/Src/stm32g0xx_hal_flash.c
  ${HAL}/Src/stm32g0xx_hal_flash_ex.c
  ${TUSB}/src/tusb.c
  ${TUSB}/src/common/tusb_fifo.c
  ${TUSB}/src/device/usbd.c
  ${TUSB}/src/class/hid/hid_device.c
  ${TUSB}/src/class/cdc/cdc_device.c
  ${TUSB}/src/portable/st/stm32_fsdev/dcd_stm32_fsdev.c
  ${TUSB}/src/portable/st/stm32_fsdev/fsdev_common.c
)
target_include_directories(vendor PUBLIC
  ${TP}/cmsis_core/CMSIS/Core/Include
  ${CMSIS_DEV}/Include
  ${HAL}/Inc
  ${TUSB}/src
  src/board
  src/usb
)
target_compile_definitions(vendor PUBLIC STM32G0B1xx USE_HAL_DRIVER)
target_compile_options(vendor PRIVATE -w)

# --- Application -------------------------------------------------------------
add_executable(devhub
  ${CMSIS_DEV}/Source/Templates/gcc/startup_stm32g0b1xx.s
  src/main.c
  src/board/board.c
  src/board/syscalls.c
  src/drivers/i2c_bus.c
  src/drivers/uart.c
  src/drivers/ucs2114.c
  src/drivers/usb2517.c
  src/drivers/flash_store.c
  src/app/ports.c
  src/app/relay.c
  src/app/buttons.c
  src/app/portctl.c
  src/app/hub_ctrl.c
  src/app/defaults.c
  src/app/system.c
  src/proto/proto.c
  src/usb/usb_descriptors.c
  src/usb/usb_dev.c
  src/console/console.c
  src/console/commands.c
)
target_include_directories(devhub PRIVATE src)
target_compile_definitions(devhub PRIVATE
  FW_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
  FW_VERSION_MINOR=${PROJECT_VERSION_MINOR}
  FW_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)
target_compile_options(devhub PRIVATE -Wall -Wextra -Wshadow -Wno-unused-parameter)
target_link_libraries(devhub PRIVATE vendor)
target_link_options(devhub PRIVATE
  -T${CMAKE_CURRENT_SOURCE_DIR}/cmake/stm32g0b1ceux.ld
  -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/devhub.map
  -Wl,--print-memory-usage
  -Wl,--no-warn-rwx-segments
)
set_target_properties(devhub PROPERTIES SUFFIX .elf)

add_custom_command(TARGET devhub POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:devhub> ${CMAKE_CURRENT_BINARY_DIR}/devhub.bin
  COMMAND ${CMAKE_SIZE} $<TARGET_FILE:devhub>
)

# --- Convenience targets -------------------------------------------------------
add_custom_target(flash
  COMMAND openocd -f interface/stlink.cfg -f target/stm32g0x.cfg
          -c "program $<TARGET_FILE:devhub> verify reset exit"
  DEPENDS devhub USES_TERMINAL
  COMMENT "Flash over SWD (ST-Link, OpenOCD)"
)
add_custom_target(dfu
  COMMAND dfu-util -a 0 -s 0x08000000:leave -D ${CMAKE_CURRENT_BINARY_DIR}/devhub.bin
  DEPENDS devhub USES_TERMINAL
  COMMENT "Flash over USB DFU (ST system bootloader)"
)
