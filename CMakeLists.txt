cmake_minimum_required(VERSION 3.1)
project(ipmi-fru-parser)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED on)

include("cmake/sdbus.cmake")

add_library(writefrudata SHARED
    frup.c
    writefrudata.C
)
target_link_libraries(writefrudata ${SYSTEMD_LIBRARY})
install(TARGETS writefrudata LIBRARY DESTINATION lib/host-ipmid)

add_library(strgfnhandler SHARED
    strgfnhandler.C
)
target_link_libraries(strgfnhandler writefrudata)
install(TARGETS strgfnhandler LIBRARY DESTINATION lib)

add_executable(phosphor-read-eeprom
    argument.C
    readeeprom.C
)
target_link_libraries(phosphor-read-eeprom writefrudata)
target_link_libraries(phosphor-read-eeprom ${SYSTEMD_LIBRARY})

install(TARGETS phosphor-read-eeprom RUNTIME DESTINATION bin)
