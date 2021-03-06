set(target "apps_gateway_${PLATFORM}_${SOFTDEVICE}")

add_executable(${target}
    "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/address_book.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/app_state.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/configurator.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/provisioner.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/protocol.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/protocol_usb.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/protocol_common.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/protocol_config_client.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ring_packet_queue.c"
    "${CMAKE_SOURCE_DIR}/../lib/custom_log/src/custom_log.c"
    "${CMAKE_SOURCE_DIR}/mesh/stack/src/mesh_stack.c"
    "${CMAKE_SOURCE_DIR}/examples/nrf_mesh_weak.c"
    "${MBTLE_SOURCE_DIR}/examples/common/src/rtt_input.c"
    ${BLE_SOFTDEVICE_SUPPORT_SOURCE_FILES}
    ${MESH_CORE_SOURCE_FILES}
    ${MESH_BEARER_SOURCE_FILES}
    ${GENERIC_ONOFF_CLIENT_SOURCE_FILES}
    ${CONFIG_CLIENT_SOURCE_FILES}
    ${CONFIG_SERVER_SOURCE_FILES}
    ${HEALTH_SERVER_SOURCE_FILES}
    ${HEALTH_CLIENT_SOURCE_FILES}
    "${CMAKE_SOURCE_DIR}/../lib/battery_level/src/battery_level_client.c"
    ${ACCESS_SOURCE_FILES}
    ${MESH_APP_TIMER_SOURCE_FILES}
    ${PROV_PROVISIONER_SOURCE_FILES}
    ${PROV_COMMON_SOURCE_FILES}
    ${PROV_BEARER_ADV_SOURCE_FILES}
    ${${PLATFORM}_SOURCE_FILES}
    ${${nRF5_SDK_VERSION}_SOURCE_FILES}
    "${SDK_ROOT}/modules/nrfx/drivers/src/nrfx_gpiote.c"
    "${SDK_ROOT}/modules/nrfx/drivers/src/nrfx_uart.c"
    "${SDK_ROOT}/modules/nrfx/drivers/src/nrfx_uarte.c"
    "${SDK_ROOT}/modules/nrfx/drivers/src/nrfx_clock.c"
    "${SDK_ROOT}/modules/nrfx/drivers/src/nrfx_power.c"
    "${SDK_ROOT}/modules/nrfx/drivers/src/nrfx_power_clock.c"
    "${SDK_ROOT}/modules/nrfx/drivers/src/prs/nrfx_prs.c"
    "${SDK_ROOT}/integration/nrfx/legacy/nrf_drv_clock.c"
    "${SDK_ROOT}/integration/nrfx/legacy/nrf_drv_power.c"
    "${SDK_ROOT}/integration/nrfx/legacy/nrf_drv_uart.c"
    "${SDK_ROOT}/components/libraries/button/app_button.c"
    "${SDK_ROOT}/components/libraries/uart/app_uart_fifo.c"
    "${SDK_ROOT}/components/libraries/fifo/app_fifo.c"
    "${SDK_ROOT}/components/libraries/usbd/app_usbd.c"
    "${SDK_ROOT}/components/libraries/usbd/class/cdc/acm/app_usbd_cdc_acm.c"
    "${SDK_ROOT}/components/libraries/usbd/app_usbd_core.c"
    "${SDK_ROOT}/components/libraries/usbd/app_usbd_serial_num.c"
    "${SDK_ROOT}/components/libraries/usbd/app_usbd_string_desc.c"
    "${SDK_ROOT}/components/drivers_nrf/usbd/nrf_drv_usbd.c"
    "${SDK_ROOT}/components/libraries/atomic_fifo/nrf_atfifo.c"
    "${SDK_ROOT}/components/libraries/atomic/nrf_atomic.c"
    "${SDK_ROOT}/external/utf_converter/utf.c"
    )

target_include_directories(${target} PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/support"
    "${CMAKE_SOURCE_DIR}/../lib/custom_log/include"
    "${CMAKE_SOURCE_DIR}/examples/common/include"
    ${BLE_SOFTDEVICE_SUPPORT_INCLUDE_DIRS}
    ${GENERIC_ONOFF_CLIENT_INCLUDE_DIRS}
    ${CONFIG_CLIENT_INCLUDE_DIRS}
    ${CONFIG_SERVER_INCLUDE_DIRS}
    ${HEALTH_SERVER_INCLUDE_DIRS}
    ${HEALTH_CLIENT_INCLUDE_DIRS}
    "${CMAKE_SOURCE_DIR}/../lib/battery_level/include"
    ${MESH_INCLUDE_DIRS}
    ${${SOFTDEVICE}_INCLUDE_DIRS}
    ${${PLATFORM}_INCLUDE_DIRS}
    ${${nRF5_SDK_VERSION}_INCLUDE_DIRS}
    "${SDK_ROOT}/components/libraries/button"
    "${SDK_ROOT}/components/libraries/uart"
    "${SDK_ROOT}/components/libraries/fifo"
    "${SDK_ROOT}/components/libraries/atomic_fifo"
    "${SDK_ROOT}/components/libraries/usbd"
    "${SDK_ROOT}/components/libraries/usbd/class/cdc"
    "${SDK_ROOT}/components/libraries/usbd/class/cdc/acm"
    "${SDK_ROOT}/components/drivers_nrf/usbd"
    "${SDK_ROOT}/integration/nrfx/legacy"
    "${SDK_ROOT}/modules/nrfx/drivers/include"
    "${SDK_ROOT}/external/utf_converter"
    )

set_target_link_options(${target}
    ${CMAKE_CURRENT_SOURCE_DIR}/support/ble_app_uart_gcc_nrf52)

target_compile_options(${target} PUBLIC
    ${${ARCH}_DEFINES})

target_compile_definitions(${target} PUBLIC
    ${USER_DEFINITIONS}
    -DUSE_APP_CONFIG
    -DCONFIG_APP_IN_CORE
    -DDEBUG
    ${${PLATFORM}_DEFINES}
    ${${SOFTDEVICE}_DEFINES}
    ${${BOARD}_DEFINES})

target_link_libraries(${target}
    rtt_${PLATFORM}
    uECC_${PLATFORM})

create_hex(${target})
add_flash_target(${target})
