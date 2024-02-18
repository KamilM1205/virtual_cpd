cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

project(com_port_driver)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4 /WX")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/")
find_package(WDK REQUIRED)

wdk_add_driver(cpd
	KMDF 1.15
	src/com_port_driver.c
	src/virtual_device.c
	)

target_include_directories(cpd PRIVATE ${PROJECT_SOURCE_DIR}/include)