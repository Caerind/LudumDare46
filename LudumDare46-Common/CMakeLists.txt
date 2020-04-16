set(SRC_LUDUMDARE46_COMMON
	Common.cpp
	Common.hpp
)
source_group("" FILES ${SRC_LUDUMDARE46_COMMON})

add_library(LudumDare46Common STATIC ${SRC_LUDUMDARE46_COMMON})
target_include_directories(LudumDare46Common PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_link_libraries(LudumDare46Common PUBLIC Enlivengine)