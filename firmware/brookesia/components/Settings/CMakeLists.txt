set(PROJ_SRC ${CMAKE_CURRENT_LIST_DIR})

file(GLOB_RECURSE PROJ_SRCS_C ${PROJ_SRC}/*.c)
file(GLOB_RECURSE PROJ_SRCS_CPP ${PROJ_SRC}/*.cpp)

idf_component_register(
    SRCS  ${PROJ_SRCS_C} ${PROJ_SRCS_CPP}
    INCLUDE_DIRS ${PROJ_SRC}
    REQUIRES brookesia_core esp32_p4_wifi6_touch_lcd_x driver esp_wifi esp_wifi_remote esp_hosted lwip bsp_extra
)

set_source_files_properties(
    ${PROJ_SRCS_CPP}
    PROPERTIES
        COMPILE_FLAGS "-Wno-missing-field-initializers"
)
