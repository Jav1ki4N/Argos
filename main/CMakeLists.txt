
file(GLOB CPPSRCS ./*.cpp)
file(GLOB HPPSRCS ./*.hpp)

idf_component_register(SRCS "${CPPSRCS}"
                       INCLUDE_DIRS "."
                    )
