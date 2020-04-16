set(SRC_LUDUMDARE46_CLIENT
	main.cpp
)
source_group("" FILES ${SRC_LUDUMDARE46_CLIENT})

add_executable(LudumDare46Client ${SRC_LUDUMDARE46_CLIENT})
target_link_libraries(LudumDare46Client PUBLIC LudumDare46Common)