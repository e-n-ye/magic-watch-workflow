set(BOOTLOADER_TARGET f411_bootloader)

add_executable(${BOOTLOADER_TARGET}
    main.c
    manifest.c
    ota.c
    storage.c
    system.c
    ${CMAKE_SOURCE_DIR}/startup_stm32f411xe.s
    ${CMAKE_SOURCE_DIR}/../../../products/f411_watch/storage/watch_ota_install.c
    ${CMAKE_SOURCE_DIR}/../../../products/f411_watch/storage/watch_ota_metadata.c
    ${CMAKE_SOURCE_DIR}/../../../products/f411_watch/storage/watch_ota_package.c
    ${CMAKE_SOURCE_DIR}/../../../products/f411_watch/storage/watch_ota_trial.c
    ${CMAKE_SOURCE_DIR}/../../../products/f411_watch/storage/watch_w25q128.c
)

add_library(f411_tinycrypt STATIC
    ${CMAKE_SOURCE_DIR}/third_party/tinycrypt/lib/source/ecc.c
    ${CMAKE_SOURCE_DIR}/third_party/tinycrypt/lib/source/ecc_dsa.c
    ${CMAKE_SOURCE_DIR}/third_party/tinycrypt/lib/source/sha256.c
    ${CMAKE_SOURCE_DIR}/third_party/tinycrypt/lib/source/utils.c
)

target_include_directories(f411_tinycrypt
    PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/tinycrypt/lib/include
)

target_compile_options(f411_tinycrypt
    PRIVATE
        -Os
)

target_link_libraries(${BOOTLOADER_TARGET}
    PRIVATE
        f411_tinycrypt
)

target_include_directories(${BOOTLOADER_TARGET}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32F4xx/Include
        ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Include
        ${CMAKE_SOURCE_DIR}/../../../products/f411_watch/storage
        ${CMAKE_SOURCE_DIR}/third_party/tinycrypt/lib/include
)

target_compile_definitions(${BOOTLOADER_TARGET}
    PRIVATE
        STM32F411xE
        WATCH_OTA_PACKAGE_NO_DEFAULT_CSPRNG
)

target_link_options(${BOOTLOADER_TARGET}
    PRIVATE
        "-T${CMAKE_CURRENT_SOURCE_DIR}/STM32F411XX_BOOTLOADER.ld"
        "-Wl,-Map=${CMAKE_BINARY_DIR}/${BOOTLOADER_TARGET}.map"
        "-Wl,--gc-sections"
        "-Wl,--print-memory-usage"
)

set_target_properties(${BOOTLOADER_TARGET}
    PROPERTIES
        OUTPUT_NAME ${BOOTLOADER_TARGET}
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
        ADDITIONAL_CLEAN_FILES "${CMAKE_BINARY_DIR}/${BOOTLOADER_TARGET}.map"
)

add_custom_command(TARGET ${BOOTLOADER_TARGET} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY}
        -O ihex
        "$<TARGET_FILE:${BOOTLOADER_TARGET}>"
        "${CMAKE_BINARY_DIR}/${BOOTLOADER_TARGET}.hex"
    COMMAND ${CMAKE_OBJCOPY}
        -O binary
        "$<TARGET_FILE:${BOOTLOADER_TARGET}>"
        "${CMAKE_BINARY_DIR}/${BOOTLOADER_TARGET}.bin"
    COMMAND ${CMAKE_COMMAND}
        -DSIZE_EXECUTABLE=${CMAKE_SIZE}
        -DELF_FILE=$<TARGET_FILE:${BOOTLOADER_TARGET}>
        -DFLASH_BUDGET=65536
        -DRAM_BUDGET=131072
        -DTARGET_LABEL=${BOOTLOADER_TARGET}
        -P ${CMAKE_SOURCE_DIR}/cmake/check_memory_budget.cmake
    VERBATIM
)
