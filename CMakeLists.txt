cmake_minimum_required(VERSION 3.12...3.15)

project(LudumDare46Project
    VERSION 0.1
    DESCRIPTION "LudumDare46"
    LANGUAGES CXX)
	
if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
	set(CMAKE_CXX_STANDARD 17)
	set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
	set(CMAKE_CXX_EXTENSIONS OFF)
	enable_testing()
endif()

add_subdirectory(Enlivengine)

add_subdirectory(LudumDare46-Common)

add_subdirectory(LudumDare46-Server)
add_subdirectory(LudumDare46-Client)
