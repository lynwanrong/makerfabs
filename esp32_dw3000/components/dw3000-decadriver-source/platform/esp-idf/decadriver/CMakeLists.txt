
set(srcs
     ../../../dwt_uwb_driver/deca_interface.c
     ../../../dwt_uwb_driver/deca_rsl.c
     ../../../dwt_uwb_driver/lib/qmath/src/qmath.c
     ../../deca_compat.c
     deca_port.c dw3000_hw.c dw3000_spi.c ../../dw3000_spi_trace.c)


file(GLOB_RECURSE EXAMPLES_SRC
    ${CMAKE_CURRENT_LIST_DIR}/../../../examples/*.c
)

# message(STATUS "EXAMPLES_SRC contains:")
# foreach(item IN LISTS EXAMPLES_SRC)
#     message(STATUS "  ${item}")
# endforeach()


list(APPEND srcs ${EXAMPLES_SRC})

set(incl
     . ../.. ../../../dwt_uwb_driver ../../../examples
     ../../../dwt_uwb_driver/lib/qmath/include)

if (CONFIG_DW3000_CHIP_DW3000)
    list(APPEND srcs ../../../dwt_uwb_driver/dw3000/dw3000_device.c)
    list(APPEND incl ../../../dwt_uwb_driver/dw3000)
elseif(CONFIG_DW3000_CHIP_DW3720)
    list(APPEND srcs ../../../dwt_uwb_driver/dw3720/dw3720_device.c)
    list(APPEND incl ../../../dwt_uwb_driver/dw3720)
endif()

idf_component_register(SRCS ${srcs}
                       PRIV_INCLUDE_DIRS priv
                       INCLUDE_DIRS ${incl}
                       REQUIRES driver)
