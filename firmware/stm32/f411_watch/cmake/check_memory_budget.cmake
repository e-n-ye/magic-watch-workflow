if(NOT DEFINED SIZE_EXECUTABLE OR NOT DEFINED ELF_FILE OR NOT DEFINED FLASH_BUDGET
   OR NOT DEFINED RAM_BUDGET OR NOT DEFINED TARGET_LABEL)
    message(FATAL_ERROR "Memory budget check requires SIZE_EXECUTABLE, ELF_FILE, FLASH_BUDGET, RAM_BUDGET and TARGET_LABEL")
endif()

execute_process(
    COMMAND "${SIZE_EXECUTABLE}" -B "${ELF_FILE}"
    RESULT_VARIABLE size_result
    OUTPUT_VARIABLE size_output
    ERROR_VARIABLE size_error
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(NOT size_result EQUAL 0)
    message(FATAL_ERROR "${TARGET_LABEL}: unable to read ELF size: ${size_error}")
endif()

string(REGEX MATCH "([0-9]+)[ \t]+([0-9]+)[ \t]+([0-9]+)" size_line "${size_output}")
if(NOT size_line)
    message(FATAL_ERROR "${TARGET_LABEL}: unexpected size output:\n${size_output}")
endif()

set(text_bytes "${CMAKE_MATCH_1}")
set(data_bytes "${CMAKE_MATCH_2}")
set(bss_bytes "${CMAKE_MATCH_3}")
math(EXPR flash_bytes "${text_bytes} + ${data_bytes}")
math(EXPR ram_bytes "${data_bytes} + ${bss_bytes}")

message(STATUS "${TARGET_LABEL}: text=${text_bytes} B data=${data_bytes} B bss=${bss_bytes} B flash=${flash_bytes}/${FLASH_BUDGET} B ram=${ram_bytes}/${RAM_BUDGET} B")

if(flash_bytes GREATER FLASH_BUDGET)
    message(FATAL_ERROR "${TARGET_LABEL}: flash budget exceeded (${flash_bytes} > ${FLASH_BUDGET} bytes)")
endif()

if(ram_bytes GREATER RAM_BUDGET)
    message(FATAL_ERROR "${TARGET_LABEL}: RAM budget exceeded (${ram_bytes} > ${RAM_BUDGET} bytes)")
endif()
