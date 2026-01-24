set(SOURCES 
            "main.c"
            "firmware/template.c"
            "firmware/link_list_menu.c"

            )

set(INCLUDE_DIRS "." "boards" "firmware")

if(DEFINED CONFIG_LVGL_VERSION_MAJOR)
    message("add v9 display files")
    file(GLOB DISPLAY_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/display/v9/*.c
    )
    list(APPEND SOURCES ${DISPLAY_SOURCES})
    list (APPEND INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/display/v9)
else()
    message("add v8 display files")
    file(GLOB DISPLAY_SOURCES 
        ${CMAKE_CURRENT_SOURCE_DIR}/display/v8/*.c
    )
    list(APPEND SOURCES ${DISPLAY_SOURCES})
    list (APPEND INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/display/v8)
endif()


# Add board files according to BOARD_TYPE
# Set default assets if the board uses partition table V2
if(CONFIG_BOARD_TYPE_3_5_IlI9488_FT6236)
    set(BOARD_TYPE "3-5-ili9488-ft6236")
elseif(CONFIG_BOARD_TYPE_1_28_GC9A01_CST816S)
    set(BOARD_TYPE "1-28-gc9a01-cst816s")
elseif(CONFIG_BOARD_TYPE_2_8_ST7798V_GT911)
    set(BOARD_TYPE "2-8-st7798v-gt911")
elseif(CONFIG_BOARD_TYPE_4_3_UNKNOWN_GT911)
    set(BOARD_TYPE "4-3-unknown-gt911")
elseif(CONFIG_BOARD_TYPE_7_0_UNKNOWN_GT911)
    set(BOARD_TYPE "7-0-unknown-gt911")
elseif(CONFIG_BOARD_TYPE_7_0_JD9165_GT911)
    set(BOARD_TYPE "7-0-jd9165-gt911")
elseif(CONFIG_BOARD_TYPE_1_8_SH8601_FT3168)
    set(BOARD_TYPE "1-8-sh8601-ft3168")
endif()

message("current board type: ${BOARD_TYPE}")


file(GLOB BOARD_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/boards/${BOARD_TYPE}/*.c
)
list(APPEND SOURCES ${BOARD_SOURCES})


file(GLOB DEV_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/firmware/${BOARD_TYPE}/release.c
)
file (GLOB ASSETS_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/firmware/${BOARD_TYPE}/assets/*.c
)
list(APPEND SOURCES ${ASSETS_SOURCES})
list(APPEND SOURCES ${DEV_SOURCES})


idf_component_register(SRCS ${SOURCES}
                    # EMBED_FILES ${LANG_SOUNDS} ${COMMON_SOUNDS}
                    INCLUDE_DIRS ${INCLUDE_DIRS}
                    )

# spiffs_create_partition_image(storage ../spiffs FLASH_IN_PROJECT)