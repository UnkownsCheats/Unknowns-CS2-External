#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

class offsets
{
public:
	bool initialize( );

	[[nodiscard]] std::uintptr_t get( std::string_view module, std::string_view key ) const;
	[[nodiscard]] const std::string* repo_file( std::string_view name ) const;

	std::uintptr_t csgo_input{ 0x237B5B0 };
	std::uintptr_t entity_list{ 0x250C5B0 };
	std::uintptr_t game_entity_system{ 0x250C5B0 };
	std::uintptr_t game_entity_system_highest_entity_index{ 0x2090 };
	std::uintptr_t game_rules{ 0x1A21D60 };
	std::uintptr_t global_vars{ 0x2085788 };
	std::uintptr_t glow_manager{ 0x2363580 };
	std::uintptr_t local_player_controller{ 0x2345D60 };
	std::uintptr_t local_player_pawn{ 0x2090880 };
	std::uintptr_t planted_c4{ 0x2374278 };
	std::uintptr_t prediction{ 0x2090790 };
	std::uintptr_t sensitivity{ 0x2364098 };
	std::uintptr_t sensitivity_sensitivity{ 0x58 };
	std::uintptr_t view_angles{ 0x237BC38 };
	std::uintptr_t view_matrix{ 0x236C2F0 };
	std::uintptr_t view_render{ 0x236B4F8 };
	std::uintptr_t weapon_c4{ 0x22E4518 };

	std::uintptr_t build_number{ 0x60CC74 };
	std::uintptr_t network_game_client{ 0x90A1A0 };
	std::uintptr_t network_game_client_client_tick_count{ 0x378 };
	std::uintptr_t network_game_client_delta_tick{ 0x24C };
	std::uintptr_t network_game_client_is_background_map{ 0x2C141F };
	std::uintptr_t network_game_client_local_player{ 0xF8 };
	std::uintptr_t network_game_client_max_clients{ 0x240 };
	std::uintptr_t network_game_client_server_tick_count{ 0x24C };
	std::uintptr_t network_game_client_sign_on_state{ 0x230 };
	std::uintptr_t window_height{ 0x90E5C4 };
	std::uintptr_t window_width{ 0x90E5C0 };

	std::uintptr_t input_system{ 0x42B50 };
	std::uintptr_t game_types{ 0x1DF0B0 };
	std::uintptr_t sound_system{ 0x512360 };
	std::uintptr_t sound_system_engine_view_data{ 0x7C };

private:
	std::unordered_map<std::string, std::string> m_repo_files{};
	std::unordered_map<std::string, std::uintptr_t> m_values{};
};
