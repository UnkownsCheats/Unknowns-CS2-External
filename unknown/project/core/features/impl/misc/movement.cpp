#include <stdafx.hpp>

namespace features::misc {

	void movement::tick( )
	{
		if ( !systems::g_local.valid( ) || !systems::g_local.alive( ) )
		{
			return;
		}

		const auto pawn = systems::g_local.pawn( );
		const auto flags = g::memory.read<std::uint32_t>( pawn + SCHEMA( "C_BaseEntity", "m_fFlags"_hash ) );
		const auto on_ground = ( flags & 1 );

		// Bunnyhop - Perfect timed (press on land)
		static bool was_on_ground = true;
		if ( settings::g_misc.m_movement.bhop )
		{
			// Perfect bhop: press jump exactly when landing
			if ( GetAsyncKeyState( VK_SPACE ) && !was_on_ground && on_ground )
			{
				g::input.inject_keyboard( VK_SPACE, true );
			}
			// Also hold space while on ground for normal jumping
			else if ( GetAsyncKeyState( VK_SPACE ) && on_ground )
			{
				g::input.inject_keyboard( VK_SPACE, true );
			}
			// Release when in air
			else if ( !on_ground )
			{
				g::input.inject_keyboard( VK_SPACE, false );
			}
		}
		was_on_ground = on_ground;

		// Speed - Override every tick to bypass game reset
		if ( settings::g_misc.m_movement.speed_multiplier > 1.0f )
		{
			const auto ms_offset = SCHEMA( "C_BasePlayerPawn", "m_pMovementServices"_hash );
			const auto s_offset = SCHEMA( "CPlayer_MovementServices", "m_flMaxspeed"_hash );

			if ( ms_offset > 0 && s_offset > 0 )
			{
				const auto movement_services = g::memory.read<std::uintptr_t>( pawn + ms_offset );
				if ( movement_services )
				{
					const auto target_speed = 250.0f * settings::g_misc.m_movement.speed_multiplier;
					// Always write to override game reset
					g::memory.write( movement_services + s_offset, target_speed );
				}
			}
		}

		// Fast Stop - Direct velocity override for instant stop
		if ( settings::g_misc.m_movement.fast_stop )
		{
			const auto vel = g::memory.read<math::vector3>( pawn + SCHEMA( "C_BaseEntity", "m_vecAbsVelocity"_hash ) );
			const auto speed = vel.length_2d( );
			
			const bool w = GetAsyncKeyState( 'W' );
			const bool s = GetAsyncKeyState( 'S' );
			const bool a = GetAsyncKeyState( 'A' );
			const bool d = GetAsyncKeyState( 'D' );

			if ( speed > 5.0f && !w && !s && !a && !d && on_ground )
			{
				// Direct velocity override for instant stop
				const auto velocity_offset = SCHEMA( "C_BaseEntity", "m_vecAbsVelocity"_hash );
				if ( velocity_offset > 0 )
				{
					g::memory.write<math::vector3>( pawn + velocity_offset, math::vector3( 0, 0, 0 ) );
				}
			}
		}

		// Spinbot
		if ( settings::g_misc.m_movement.spinbot )
		{
			static const auto view_render = g::memory.find_vtable_instance( g::modules.client, "CViewRender" );
			if ( view_render )
			{
				const auto view = view_render + 0x10;

				// Store persistent spin angle to bypass game reset
				static float spin_angle_y = 0.0f;
				static bool spin_initialized = false;

				// Read current angles (need pitch for aimbot/triggerbot to work)
				const auto current_angles = g::memory.read<math::vector3>( view + 0xc );

				// Initialize angle from current view to prevent snapping
				if ( !spin_initialized )
				{
					if ( current_angles.y != 0.0f || current_angles.x != 0.0f )
					{
						spin_angle_y = current_angles.y;
						spin_initialized = true;
					}
				}

				// Increment spin angle
				if ( spin_initialized )
				{
					spin_angle_y += settings::g_misc.m_movement.spinbot_speed * ( zdraw::get_framerate( ) / 64.0f );
					spin_angle_y = math::helpers::normalize_yaw( spin_angle_y );
				}

				// Write to view render camera angles (overrides game reset)
				// Preserve pitch (x) so aimbot/triggerbot still work
				g::memory.write( view + 0xc, math::vector3( current_angles.x, spin_angle_y, 0.0f ) );

				// Also write to player's m_angEyeAngles for server-side consistency
				// Preserve pitch here too
				if ( pawn )
				{
					const auto eye_angles_offset = SCHEMA( "C_CSPlayerPawn", "m_angEyeAngles"_hash );
					if ( eye_angles_offset > 0 )
					{
						g::memory.write( pawn + eye_angles_offset, math::vector3( current_angles.x, spin_angle_y, 0.0f ) );
					}
				}
			}
		}

		// Anti-Aim
		if ( settings::g_misc.m_movement.anti_aim && pawn )
		{
			static float aa_yaw = 0.0f;
			static float aa_pitch = 0.0f;
			static float desync_value = 0.0f;
			static int tick_counter = 0;

			const auto view_render = g::memory.find_vtable_instance( g::modules.client, "CViewRender" );
			if ( view_render )
			{
				const auto view = view_render + 0x10;
				const auto current_angles = g::memory.read<math::vector3>( view + 0xc );

					tick_counter++;

				aa_pitch = settings::g_misc.m_movement.anti_aim_pitch;

				switch ( settings::g_misc.m_movement.anti_aim_type )
				{
				case 0: // Jitter
					if ( tick_counter % 4 < 2 )
						aa_yaw = current_angles.y + settings::g_misc.m_movement.anti_aim_yaw_offset;
					else
						aa_yaw = current_angles.y - settings::g_misc.m_movement.anti_aim_yaw_offset;
					break;
				case 1: // Spin
					aa_yaw += 15.0f * ( zdraw::get_framerate( ) / 64.0f );
					aa_yaw = math::helpers::normalize_yaw( aa_yaw );
					break;
				case 2: // Static
					aa_yaw = current_angles.y + settings::g_misc.m_movement.anti_aim_yaw_offset;
					break;
				case 3: // Desync
					aa_yaw = current_angles.y + settings::g_misc.m_movement.anti_aim_yaw_offset;

					// Desync: jitter the fake angle
					if ( settings::g_misc.m_movement.anti_aim_fake_jitter )
					{
						if ( tick_counter % 2 == 0 )
							desync_value = settings::g_misc.m_movement.anti_aim_desync;
						else
							desync_value = -settings::g_misc.m_movement.anti_aim_desync;
					}
					else
					{
						desync_value = settings::g_misc.m_movement.anti_aim_desync;
					}

					// Write fake angles to eye angles for desync
					{
						const auto eye_angles_offset = SCHEMA( "C_CSPlayerPawn", "m_angEyeAngles"_hash );
						if ( eye_angles_offset > 0 )
						{
							g::memory.write( pawn + eye_angles_offset, math::vector3( aa_pitch, aa_yaw + desync_value, 0.0f ) );
						}
					}
					break;
				default:
					aa_pitch = current_angles.x;
					aa_yaw = current_angles.y;
					break;
				}

				// Apply anti-aim to view
				g::memory.write( view + 0xc, math::vector3( aa_pitch, aa_yaw, 0.0f ) );
			}
		}

		// Fake Lag (basic - choke commands)
		if ( settings::g_misc.m_movement.fake_lag )
		{
			// This requires netchannel manipulation which is complex
			// Basic implementation: we would need to find and modify the netchannel
			// For now, this is a placeholder - actual implementation requires more complex memory work
		}

		// Backtrack (basic - record positions)
		if ( settings::g_misc.m_movement.backtrack )
		{
			// This requires netvar manipulation and command recording
			// Would need to store command history and replay ticks
			// This is complex and requires more infrastructure
		}

		// Third Person - Fixed camera behind player
		const auto& tp = settings::g_misc.m_movement.m_third_person;
		static bool tp_active = false;
		static bool tp_key_pressed = false;

		const bool key_down = ( tp.key > 0 && GetAsyncKeyState( tp.key ) & 0x8000 );
		if ( key_down && !tp_key_pressed )
		{
			tp_active = !tp_active;
			tp_key_pressed = true;
		}
		else if ( !key_down )
		{
			tp_key_pressed = false;
		}

		const bool should_tp = tp.enabled && tp_active;
		
		// Third person via camera services override (more reliable in CS2)
		if ( should_tp )
		{
			// Method 1: Use camera services for third person
			const auto camera_services_offset = SCHEMA( "C_BasePlayerPawn", "m_pCameraServices"_hash );
			if ( camera_services_offset > 0 )
			{
				const auto camera_services = g::memory.read<std::uintptr_t>( pawn + camera_services_offset );
				if ( camera_services )
				{
					// Force third person camera offset - position behind player
					const auto cam_offset = SCHEMA( "CCSPlayer_CameraServices", "m_vecCameraOffset"_hash );
					if ( cam_offset > 0 )
					{
						// Camera offset relative to player (forward, up)
						g::memory.write<math::vector3>( camera_services + cam_offset, math::vector3( 0.0f, 150.0f, 50.0f ) );
					}
				}
			}

			// Method 2: Also set observer mode as backup
			const auto observer_services_offset = SCHEMA( "C_BasePlayerPawn", "m_pObserverServices"_hash );
			const auto observer_mode_offset = SCHEMA( "CPlayer_ObserverServices", "m_iObserverMode"_hash );
			const auto observer_target_offset = SCHEMA( "CPlayer_ObserverServices", "m_hObserverTarget"_hash );

			if ( observer_services_offset > 0 && observer_mode_offset > 0 )
			{
				const auto observer_services = g::memory.read<std::uintptr_t>( pawn + observer_services_offset );
				if ( observer_services )
				{
					// Use mode 6 (Roam) for free third person camera
					g::memory.write<int>( observer_services + observer_mode_offset, 6 );

					if ( observer_target_offset > 0 )
					{
						const auto controller = systems::g_local.controller( );
						if ( controller )
						{
							const auto pawn_handle = g::memory.read<std::uint32_t>( controller + SCHEMA( "CCSPlayerController", "m_hPlayerPawn"_hash ) );
							if ( pawn_handle )
							{
								g::memory.write<std::uint32_t>( observer_services + observer_target_offset, pawn_handle );
							}
						}
					}
				}
			}
		}
		else
		{
			// Reset to first person when disabled
			const auto observer_services_offset = SCHEMA( "C_BasePlayerPawn", "m_pObserverServices"_hash );
			const auto observer_mode_offset = SCHEMA( "CPlayer_ObserverServices", "m_iObserverMode"_hash );

			if ( observer_services_offset > 0 && observer_mode_offset > 0 )
			{
				const auto observer_services = g::memory.read<std::uintptr_t>( pawn + observer_services_offset );
				if ( observer_services )
				{
					g::memory.write<int>( observer_services + observer_mode_offset, 0 );
				}
			}
		}
	}


} // namespace features::misc
