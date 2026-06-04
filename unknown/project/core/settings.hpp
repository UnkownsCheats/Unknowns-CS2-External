#pragma once

namespace settings {

	struct combat
	{
		struct aimbot
		{
			bool enabled{ true };
			int key{ VK_XBUTTON2 };
			int type{ 0 };

			int fov{ 2 };
			int smoothing{ 10 };

			bool autowall{ true };
			float min_damage{ 90.0f };

			bool head_only{ true };
			bool visible_only{ true };

			bool draw_fov{ true };
			zdraw::rgba fov_color{ 225, 225, 225, 125 };

			bool predictive{ true };
		};

		struct triggerbot
		{
			bool enabled{ true };
			int key{ VK_XBUTTON2 };

			float hitchance{ 75.0f };
			int delay{ 10 };

			bool autowall{ true };
			float min_damage{ 90.0f };

			bool autostop{ false };
			bool early_autostop{ false };

			bool predictive{ true };
		};

		struct rcs
		{
			bool enabled{ false };
			float factor{ 1.0f }; // 0.0 to 1.0
			bool visual_no_recoil{ false };
		} m_rcs{};

		struct group_config
		{
			aimbot aimbot{};
			triggerbot triggerbot{};
		};

		static constexpr std::uint32_t k_group_count{ 6 };

		std::array<group_config, k_group_count> groups{};

		group_config& get( std::uint32_t weapon_type )
		{
			const auto idx = weapon_type - cstypes::pistol;
			return this->groups[ idx < k_group_count ? idx : 2 ];
		}

		const group_config& get( std::uint32_t weapon_type ) const
		{
			const auto idx = weapon_type - cstypes::pistol;
			return this->groups[ idx < k_group_count ? idx : 2 ];
		}
	};

	struct esp
	{
		bool bypass_capture{ false };

		struct player
		{
			bool enabled{ true };

			struct box
			{
				enum class style0 : std::uint8_t { full, cornered };

				bool enabled{ true };
				style0 style{ style0::cornered };
				bool fill{ true };
				bool outline{ true };
				float corner_length{ 10.0f };

				zdraw::rgba visible_color{ 140, 150, 235, 255 };
				zdraw::rgba occluded_color{ 110, 115, 170, 180 };
			} m_box{};

			struct skeleton
			{
				enum class occluded_style0 : std::uint8_t { solid, dotted };

				bool enabled{ true };
				occluded_style0 occluded_style{ occluded_style0::dotted };
				float thickness{ 1.0f };


				zdraw::rgba visible_color{ 170, 175, 220, 255 };
				zdraw::rgba occluded_color{ 130, 135, 180, 180 };
			} m_skeleton{};

			struct hitboxes
			{
				bool enabled{ false };

				zdraw::rgba visible_color{ 150, 160, 240, 10 };
				zdraw::rgba occluded_color{ 115, 120, 185, 10 };

				bool fill{ true };
				bool outline{ true };
			} m_hitboxes{};

			struct health_bar
			{
				enum class position : std::uint8_t { left, top, bottom };

				bool enabled{ true };
				position position{ position::left };
				bool outline{ true };
				bool gradient{ true };
				bool show_value{ true };

				zdraw::rgba full_color{ 140, 150, 235, 255 };
				zdraw::rgba low_color{ 75, 80, 180, 255 };
				zdraw::rgba background_color{ 15, 16, 22, 150 };
				zdraw::rgba outline_color{ 15, 16, 22, 255 };
				zdraw::rgba text_color{ 195, 200, 215, 255 };
			} m_health_bar{};

			struct ammo_bar
			{
				enum class position : std::uint8_t { left, top, bottom };

				bool enabled{ true };
				position position{ position::bottom };
				bool outline{ true };
				bool gradient{ true };
				bool show_value{ false };

				zdraw::rgba full_color{ 140, 150, 235, 255 };
				zdraw::rgba low_color{ 75, 80, 180, 255 };
				zdraw::rgba background_color{ 15, 16, 22, 150 };
				zdraw::rgba outline_color{ 15, 16, 22, 255 };
				zdraw::rgba text_color{ 195, 200, 215, 255 };
			} m_ammo_bar{};

			struct info_flags
			{
				enum flag : std::uint8_t
				{
					none = 0,
					money = 1 << 0,
					armor = 1 << 1,
					kit = 1 << 2,
					scoped = 1 << 3,
					defusing = 1 << 4,
					flashed = 1 << 5,
					ping = 1 << 6,
					distance = 1 << 7
				};

				bool enabled{ true };
				std::uint8_t flags{ flag::money | flag::armor | flag::kit | flag::scoped | flag::defusing | flag::flashed | flag::ping };

				zdraw::rgba money_color{ 120, 230, 160, 255 };
				zdraw::rgba armor_color{ 195, 200, 215, 255 };
				zdraw::rgba kit_color{ 140, 150, 235, 255 };
				zdraw::rgba scoped_color{ 195, 200, 215, 255 };
				zdraw::rgba defusing_color{ 140, 150, 235, 255 };
				zdraw::rgba flashed_color{ 255, 210, 120, 255 };
				zdraw::rgba distance_color{ 90, 95, 130, 255 };

				[[nodiscard]] bool has( flag f ) const { return this->flags & f; }
			} m_info_flags{};

			struct name
			{
				bool enabled{ true };
				zdraw::rgba color{ 195, 200, 215, 230 };
			} m_name{};

			struct weapon
			{
				enum class display_type : std::uint8_t { text, icon, text_and_icon };

				bool enabled{ true };
				display_type display{ display_type::icon };

				zdraw::rgba text_color{ 195, 200, 215, 210 };
				zdraw::rgba icon_color{ 195, 200, 215, 230 };
			} m_weapon{};
			struct offscreen_arrows
			{
				bool enabled{ true };
				float size{ 10.0f };
				float radius{ 100.0f };
				zdraw::rgba color{ 140, 150, 235, 255 };
			} m_offscreen_arrows{};

			struct ranking
			{
				bool enabled{ false };
				zdraw::rgba color{ 255, 215, 0, 255 };
			} m_ranking{};

			struct chams
			{
				bool enabled{ true };
				zdraw::rgba color{ 140, 150, 235, 100 };
				bool wireframe{ false };
			} m_chams{};

			struct headshot_dot
			{
				bool enabled{ false };
				zdraw::rgba color{ 255, 255, 255, 255 };
			} m_headshot_dot{};

			struct head_direction
			{
				bool enabled{ false };
				float length{ 30.0f };
				zdraw::rgba color{ 200, 200, 200, 200 };
			} m_head_direction{};

			struct footstep_esp
			{
				enum class style0 : std::uint8_t { ripples, shoe_prints };

				bool enabled{ false };
				style0 style{ style0::ripples };
				zdraw::rgba color{ 140, 150, 235, 255 };
				float duration{ 1.5f };
				float max_radius{ 25.0f };
			} m_footstep_esp{};

			struct sound_esp
			{
				bool enabled{ false };
				bool gunshots{ true };
				bool landfall{ true };

				zdraw::rgba shot_color{ 235, 110, 110, 255 };
				zdraw::rgba land_color{ 240, 240, 110, 255 };

				float duration{ 1.5f };
				float max_radius{ 50.0f };
			} m_sound_esp{};

			struct visual_utility
			{
				bool no_flash{ false };
				float flash_alpha{ 0.0f };

				bool no_smoke{ false };
				bool smoke_visualizer{ false };
				
				enum class smoke_style : std::uint8_t { wireframe, solid, gradient, full };
				smoke_style style{ smoke_style::wireframe };
				
				zdraw::rgba smoke_color{ 180, 180, 200, 80 };
				float smoke_radius{ 155.0f };

				struct china_hat
				{
					bool enabled{ false };
					float radius{ 10.0f };
					zdraw::rgba color{ 140, 150, 235, 120 };
				} m_china_hat{};
			} m_visuals{};
		} m_player{};



		struct item
		{
			bool enabled{ true };
			float max_distance{ 40.0f };

			struct icon
			{
				bool enabled{ true };
				zdraw::rgba color{ 195, 200, 215, 200 };
			} m_icon{};

			struct name
			{
				bool enabled{ false };
				zdraw::rgba color{ 195, 200, 215, 180 };
			} m_name{};

			struct ammo
			{
				bool enabled{ true };
				zdraw::rgba color{ 140, 150, 235, 200 };
				zdraw::rgba empty_color{ 180, 80, 80, 200 };
			} m_ammo{};

			struct filters
			{
				bool rifles{ true };
				bool smgs{ true };
				bool shotguns{ true };
				bool snipers{ true };
				bool pistols{ true };
				bool heavy{ true };
				bool grenades{ true };
				bool utility{ true };
			} m_filters{};
		} m_item{};

		struct projectile
		{
			bool enabled{ true };

			bool show_icon{ true };
			bool show_name{ true };
			bool show_timer_bar{ true };
			bool show_inferno_bounds{ true };

			zdraw::rgba default_color{ 195, 200, 215, 200 };
			zdraw::rgba color_he{ 220, 150, 150, 220 };
			zdraw::rgba color_flash{ 230, 220, 150, 220 };
			zdraw::rgba color_smoke{ 160, 200, 180, 220 };
			zdraw::rgba color_molotov{ 220, 170, 130, 220 };
			zdraw::rgba color_decoy{ 170, 175, 200, 200 };

			zdraw::rgba timer_high_color{ 140, 150, 235, 255 };
			zdraw::rgba timer_low_color{ 220, 100, 100, 255 };
			zdraw::rgba bar_background{ 15, 16, 22, 150 };
		} m_projectile{};

		struct bomb
		{
			bool enabled{ true };
			zdraw::rgba color{ 220, 80, 80, 255 };
			zdraw::rgba timer_color{ 195, 200, 215, 255 };
		} m_bomb{};

		struct radar
		{
			bool enabled{ false };
			float x{ 20.0f }, y{ 20.0f };
			float size{ 150.0f };
			float scale{ 1.0f };
			bool show_teammates{ true };
		} m_radar{};

		struct spectator_list
		{
			bool enabled{ false };
			float x{ 20.0f }, y{ 200.0f };
		} m_spectator_list{};

		struct bullet_tracers
		{
			bool enabled{ true };
			bool tracers{ true };  // Draw beam from gun to hit
			bool impacts{ true };  // Draw impact points
			zdraw::rgba tracer_color{ 255, 200, 50, 200 };
			zdraw::rgba impact_color{ 255, 50, 50, 200 };
			float tracer_duration{ 0.5f };  // seconds
			float impact_size{ 5.0f };
		} m_bullet_tracers{};

		struct glow
		{
			bool enabled{ true };
			bool enemies{ true };
			bool teammates{ false };
			zdraw::rgba enemy_color{ 180, 160, 255, 255 };
			zdraw::rgba teammate_color{ 50, 150, 255, 255 };
		} m_glow{};

		struct chams
		{
			bool enabled{ true };
			bool enemies{ true };
			bool teammates{ false };
			bool through_walls{ true };
			zdraw::rgba enemy_visible{ 255, 50, 50, 255 };
			zdraw::rgba enemy_occluded{ 200, 50, 50, 150 };
			zdraw::rgba teammate_visible{ 50, 150, 255, 255 };
			zdraw::rgba teammate_occluded{ 50, 100, 200, 150 };
		} m_chams{};

		struct hit_marker
		{
			bool enabled{ true };
			bool show_damage{ true };
			zdraw::rgba color{ 255, 255, 255, 255 };
		} m_hit_marker{};

		struct skin_changer
		{
			bool enabled{ false };
			int ak47{ 180 };
			int m4a4{ 309 };
			int m4a1s{ 548 };
			int awp{ 344 };
			int deagle{ 172 };
			int glock{ 38 };
			int knife{ 571 };
			int usp{ 311 };
		} m_skin_changer{};
	};


	struct misc
	{
		struct grenades
		{
			bool enabled{ true };

			zdraw::rgba line_color{ 170, 175, 220, 200 };
			float line_thickness{ 2.0f };
			bool line_gradient{ true };

			bool show_bounces{ true };
			zdraw::rgba bounce_color{ 195, 200, 215, 255 };
			float bounce_size{ 2.0f };

			zdraw::rgba detonate_color{ 140, 150, 235, 255 };
			float detonate_size{ 4.0f };

			bool per_type_colors{ false };
			zdraw::rgba color_he{ 190, 140, 140, 200 };
			zdraw::rgba color_flash{ 200, 195, 150, 200 };
			zdraw::rgba color_smoke{ 150, 185, 165, 200 };
			zdraw::rgba color_molotov{ 195, 155, 130, 200 };
			zdraw::rgba color_decoy{ 160, 165, 185, 200 };

			bool local_only{ true };
			float fade_duration{ 0.3f };
		} m_grenades{};

		struct crosshair
		{
			bool enabled{ false };
			float length{ 8.0f };
			float gap{ 4.0f };
			float thickness{ 1.0f };
			bool outline{ true };
			zdraw::rgba color{ 255, 255, 255, 255 };
		} m_crosshair{};

		struct movement
		{
			bool bhop{ true };
			bool auto_strafe{ false };
			bool fast_stop{ false };
			float speed_multiplier{ 1.0f };

			struct third_person
			{
				bool enabled{ false };
				int key{ 0 };
			} m_third_person{};

			bool spinbot{ false };
			float spinbot_speed{ 10.0f };

			// Anti-aim
			bool anti_aim{ false };
			int anti_aim_type{ 0 };  // 0 = jitter, 1 = spin, 2 = static, 3 = desync
			float anti_aim_pitch{ 0.0f };  // -89 to 89
			float anti_aim_yaw_offset{ 180.0f };
			bool anti_aim_fake_jitter{ false };
			float anti_aim_desync{ 30.0f };

			// Fake lag
			bool fake_lag{ false };
			int fake_lag_ticks{ 1 };

			// Backtrack
			bool backtrack{ false };
			int backtrack_ticks{ 12 };
		} m_movement{};

		struct sound_enhancement
		{
			bool enabled{ false };
			float volume_multiplier{ 1.0f };
		} m_sound_enhancement{};

		struct skin_changer
		{
			bool enabled{ false };
			bool randomize{ false };
			
			// Weapons (paintkit IDs - use game IDs)
			int ak47{ 180 };
			int m4a4{ 309 };
			int m4a1s{ 548 };
			int awp{ 344 };
			int desert_eagle{ 172 };
			int glock{ 38 };
			int usp_s{ 311 };
			int p250{ 296 };
			int five_seven{ 225 };
			int magnum{ 64 };
			int nova{ 43 };
			int mp9{ 17 };

			// Knife
			int knife{ 571 };  // Karambit
			
			// Gloves
			int gloves{ 10085 };  // Sporty
		} m_skin_changer{};

		struct camera
		{
			bool custom_fov{ false };
			float fov{ 90.0f };
		} m_camera{};
	};




	inline combat g_combat{};
	inline esp g_esp{};
	inline misc g_misc{};

} // namespace settings