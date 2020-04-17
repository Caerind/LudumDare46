set(SRC_LUDUMDARE46_CLIENT
	ClientSocket.hpp
	ConnectingState.hpp
	GameMap.cpp
	GameMap.hpp
	GameSingleton.cpp
	GameSingleton.hpp
	GameState.cpp
	GameState.hpp
	main.cpp
	Player.hpp
)
source_group("" FILES ${SRC_LUDUMDARE46_CLIENT})

add_executable(LudumDare46Client ${SRC_LUDUMDARE46_CLIENT})
target_link_libraries(LudumDare46Client PUBLIC LudumDare46Common)