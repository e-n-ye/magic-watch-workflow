cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME my_watch_f411)

# Include toolchain file
include("cmake/gcc-arm-none-eabi.cmake")

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Optional static-analysis executable override. The cppcheck target resolves
# the tool when it is invoked, so normal configure/build does not require it.
set(WATCH_CPPCHECK_EXECUTABLE "" CACHE FILEPATH "Optional path to the cppcheck executable")
set(WATCH_CLANG_FORMAT_EXECUTABLE "" CACHE FILEPATH "Optional path to the clang-format executable")

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Keep the generated default handler available for CubeMX regeneration, but
# route the vector-table symbol to the hand-written diagnostic implementation.
set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/stm32f4xx_it.c
    PROPERTIES COMPILE_DEFINITIONS HardFault_Handler=watch_generated_hard_fault_handler
)

# The application vector table is linked at the first address after the
# reserved bootloader region. Keep this definition on the generated interface
# target so SystemInit receives it when its source is compiled.
target_compile_definitions(stm32cubemx INTERFACE
    USER_VECT_TAB_ADDRESS
    VECT_TAB_OFFSET=0x00010000U
)

if(CMAKE_BUILD_TYPE STREQUAL "Diagnostic")
    target_compile_definitions(stm32cubemx INTERFACE
        WATCH_DIAGNOSTIC=1
        USE_FULL_ASSERT=1
    )
endif()

# Add hand-written board and application sources
add_subdirectory(user)
add_subdirectory(bootloader)

target_link_options(${CMAKE_PROJECT_NAME} PRIVATE
    "-T${CMAKE_CURRENT_SOURCE_DIR}/STM32F411XX_FLASH.ld"
    "-Wl,-Map=${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.map"
)

add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY}
        -O ihex
        "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>"
        "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.hex"
    COMMAND ${CMAKE_OBJCOPY}
        -O binary
        "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>"
        "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.bin"
    COMMAND ${CMAKE_COMMAND}
        -DSIZE_EXECUTABLE=${CMAKE_SIZE}
        -DELF_FILE=$<TARGET_FILE:${CMAKE_PROJECT_NAME}>
        -DFLASH_BUDGET=409600
        -DRAM_BUDGET=131072
        -DTARGET_LABEL=${CMAKE_PROJECT_NAME}
        -P ${CMAKE_SOURCE_DIR}/cmake/check_memory_budget.cmake
    VERBATIM
)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/user
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
    f411_watch_user

    # Add user defined libraries
)
