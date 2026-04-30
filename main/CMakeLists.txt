
file(GLOB CPPSRCS ./*.cpp)
file(GLOB HPPSRCS ./*.hpp)

idf_component_register(SRCS "input.cpp" "network.cpp" "main.cpp" "${CPPSRCS}"
                       INCLUDE_DIRS "."
                    )
