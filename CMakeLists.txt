cmake_minimum_required(VERSION 2.8.9)

project(zvte C)

add_definitions(-std=c99)

add_executable(zvte zvte.c config.h)

include(FindPkgConfig)

pkg_check_modules (GTK gtk+-3.0)
pkg_check_modules (VTE vte-2.91)

include_directories(${GTK_INCLUDE_DIRS} ${VTE_INCLUDE_DIRS})
link_directories(${GTK_LIBRARY_DIRS} ${VTE_LIBRARY_DIRS})
target_link_libraries(zvte ${GTK_LIBRARIES} ${VTE_LIBRARIES})

install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/zvte DESTINATION bin)
