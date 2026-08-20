idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS ".")

target_compile_options(${COMPONENT_LIB} PUBLIC -Wno-missing-field-initializers)

# Function to get component library
function(get_component_library component_name output_var)
    # Get the exact component name
    idf_build_get_property(build_components BUILD_COMPONENTS)
    set(TARGET_COMPONENT "")
    foreach(COMPONENT ${build_components})
        if(COMPONENT MATCHES "${component_name}" OR COMPONENT MATCHES "espressif__${component_name}")
            set(TARGET_COMPONENT ${COMPONENT})
            break()
        endif()
    endforeach()

    # Get the component library
    if(TARGET_COMPONENT STREQUAL "")
        message(FATAL_ERROR "Component '${component_name}' not found.")
    else()
        idf_component_get_property(COMPONENT_LIB ${TARGET_COMPONENT} COMPONENT_LIB)
        set(${output_var} ${COMPONENT_LIB} PARENT_SCOPE)
    endif()
endfunction()

# get_component_library("lvgl" LVGL_LIB)
# target_compile_options(
#     ${LVGL_LIB}
#     PUBLIC
#         "-Wno-attributes"
# )

set(STORAGE_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../spiffs")
set(STORAGE_STAGING_DIR "${CMAKE_BINARY_DIR}/storage_spiffs")
set(STORAGE_STAGING_STAMP "${CMAKE_BINARY_DIR}/storage_spiffs.stamp")

file(GLOB_RECURSE STORAGE_SOURCE_FILES CONFIGURE_DEPENDS LIST_DIRECTORIES false
    "${STORAGE_SOURCE_DIR}/*")
file(MAKE_DIRECTORY "${STORAGE_STAGING_DIR}")

# Keep the complete 30 px Chinese font in SPIFFS instead of linking another
# multi-megabyte LVGL font into the application image. The UI loads it into
# PSRAM and falls back to the component's static basic font if loading fails.
idf_build_get_property(build_components BUILD_COMPONENTS)
set(XIAOZHI_FONTS_DIR "")
foreach(component IN LISTS build_components)
    if(component MATCHES "xiaozhi-fonts")
        idf_component_get_property(XIAOZHI_FONTS_DIR ${component} COMPONENT_DIR)
        break()
    endif()
endforeach()

if(NOT XIAOZHI_FONTS_DIR)
    message(FATAL_ERROR "xiaozhi-fonts component was not resolved")
endif()

set(XIAOZHI_COMMON_FONT
    "${XIAOZHI_FONTS_DIR}/cbin/font_puhui_common_30_4.bin")
if(NOT EXISTS "${XIAOZHI_COMMON_FONT}")
    message(FATAL_ERROR "xiaozhi-fonts is missing font_puhui_common_30_4.bin")
endif()

set(XIAOZHI_COMMON_FONT_STAGED
    "${STORAGE_STAGING_DIR}/xiaozhi/font.bin")

add_custom_command(
    OUTPUT "${STORAGE_STAGING_STAMP}"
    COMMAND "${CMAKE_COMMAND}" -E rm -rf "${STORAGE_STAGING_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${STORAGE_STAGING_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
        "${STORAGE_SOURCE_DIR}" "${STORAGE_STAGING_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory
        "${STORAGE_STAGING_DIR}/xiaozhi"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${XIAOZHI_COMMON_FONT}" "${XIAOZHI_COMMON_FONT_STAGED}"
    COMMAND "${CMAKE_COMMAND}" -E touch "${STORAGE_STAGING_STAMP}"
    DEPENDS ${STORAGE_SOURCE_FILES} "${XIAOZHI_COMMON_FONT}"
    COMMENT "Staging SPIFFS resources"
    VERBATIM
)
add_custom_target(storage_spiffs_stage DEPENDS "${STORAGE_STAGING_STAMP}")

spiffs_create_partition_image(
    storage "${STORAGE_STAGING_DIR}"
    FLASH_IN_PROJECT
    DEPENDS storage_spiffs_stage
)
