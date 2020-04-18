set(SRC_LUDUMDARE46_SERVER
	main.cpp
	Player.hpp
	Server.cpp
	Server.hpp
	ServerSocket.hpp
)
source_group("" FILES ${SRC_LUDUMDARE46_SERVER})

add_executable(LudumDare46Server ${SRC_LUDUMDARE46_SERVER})
target_link_libraries(LudumDare46Server PUBLIC LudumDare46Common)