set(SRC_LUDUMDARE46_CLIENT
	ClientSocket.hpp
	ConnectingState.cpp
	ConnectingState.hpp
	GameMap.cpp
	GameMap.hpp
	GameSingleton.cpp
	GameSingleton.hpp
	GameState.cpp
	GameState.hpp
	IntroState.cpp
	IntroState.hpp
	main.cpp
	MenuState.cpp
	MenuState.hpp
	Player.hpp
	ProfState.cpp
	ProfState.hpp
)
source_group("" FILES ${SRC_LUDUMDARE46_CLIENT})

add_executable(LudumDare46Client ${SRC_LUDUMDARE46_CLIENT})
target_link_libraries(LudumDare46Client PUBLIC LudumDare46Common)