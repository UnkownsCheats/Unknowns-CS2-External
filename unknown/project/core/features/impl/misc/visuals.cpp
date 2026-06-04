#include <stdafx.hpp>
#include <core/features/features.hpp>

namespace features::misc {

	void visuals::tick( )
	{
		const auto local_pawn = systems::g_local.pawn( );
		if ( !local_pawn )
			return;

		// Custom FOV
		if ( settings::g_misc.m_camera.custom_fov )
		{
			const auto local_controller = systems::g_local.controller( );
			if ( local_controller )
			{
				g::memory.write<std::uint32_t>( local_controller + SCHEMA( "CBasePlayerController", "m_iDesiredFOV"_hash ), static_cast< std::uint32_t >( settings::g_misc.m_camera.fov ) );
			}
		}

		// No Flash / Flash Transparency
		if ( settings::g_esp.m_player.m_visuals.no_flash )
		{
			g::memory.write<float>( local_pawn + SCHEMA( "C_CSPlayerPawnBase", "m_flFlashMaxAlpha"_hash ), settings::g_esp.m_player.m_visuals.flash_alpha );
		}
		else
		{
			g::memory.write<float>( local_pawn + SCHEMA( "C_CSPlayerPawnBase", "m_flFlashMaxAlpha"_hash ), 255.0f );
		}

		// No Smoke Logic (Memory Writes)
		if ( settings::g_esp.m_player.m_visuals.no_smoke )
		{
			const auto projects = systems::g_collector.projectiles( );
			for ( const auto& proj : projects )
			{
				if ( proj.subtype == systems::collector::projectile_subtype::smoke_grenade )
				{
					// Completely removed
					g::memory.write<std::int32_t>( proj.entity + SCHEMA( "C_SmokeGrenadeProjectile", "m_nSmokeEffectTickBegin"_hash ), 0 );
				}
			}
		}

		// Player Glow (from Vortix)`n`tif ( settings::g_esp.m_glow.enabled )`n`t{`n`t`tfor ( const auto& player : systems::g_collector.players( ) )`n`t\t{`n`t`t\tif ( player.pawn == local_pawn )`n`t`t\t\tcontinue;`n`n`t`t\tconst auto is_enemy = systems::g_local.is_enemy( player.team );`n`t`t\t`n`t`t\tif ( ( is_enemy && settings::g_esp.m_glow.enemies ) || ( !is_enemy && settings::g_esp.m_glow.teammates ) )`n`t`t\t{`n`t`t\t\tconst auto& color = is_enemy ? settings::g_esp.m_glow.enemy_color : settings::g_esp.m_glow.teammate_color;`n`t`t`t`t`n`t`t\t\t// Write glow color override (ARGB format)`n`t`t\t\tconst auto glow_color = ( static_cast< std::uint32_t >( color.a ) << 24 ) |`n`t`t\t\t\t( static_cast< std::uint32_t >( color.r ) << 16 ) |`n`t`t\t\t\t( static_cast< std::uint32_t >( color.g ) << 8 ) |`n`t`t\t\t\tstatic_cast< std::uint32_t >( color.b );`n`t`t`t`t`n`t`t\t\tg::memory.write<std::uint32_t>( player.pawn + SCHEMA( "C_BaseEntity", "m_glowColorOverride"_hash ), glow_color );`n`t`t\t\tg::memory.write<int>( player.pawn + SCHEMA( "C_BaseEntity", "m_bGlowing"_hash ), 1 );`n`t`t\t}`n`t\t}`n`t}`n`n`t// Visual No Recoil
		if ( settings::g_combat.m_rcs.visual_no_recoil )
		{
			g::memory.write<math::vector3>( local_pawn + SCHEMA( "C_CSPlayerPawn", "m_aimPunchAngle"_hash ), { 0.0f, 0.0f, 0.0f } );
			g::memory.write<math::vector3>( local_pawn + SCHEMA( "C_CSPlayerPawn", "m_aimPunchAngleVel"_hash ), { 0.0f, 0.0f, 0.0f } );
			g::memory.write<math::vector3>( local_pawn + SCHEMA( "C_CSPlayerPawnBase", "m_vecPunchAngle"_hash ), { 0.0f, 0.0f, 0.0f } );
			g::memory.write<math::vector3>( local_pawn + SCHEMA( "C_CSPlayerPawnBase", "m_vecPunchAngleVel"_hash ), { 0.0f, 0.0f, 0.0f } );
		}

		// Glow - Write to renderable attribute system
		if ( settings::g_esp.m_glow.enabled )
		{
			for ( const auto& player : systems::g_collector.players( ) )
			{
				const bool is_enemy = systems::g_local.is_enemy( player.team );
				if ( is_enemy && !settings::g_esp.m_glow.enemies )
					continue;
				if ( !is_enemy && !settings::g_esp.m_glow.teammates )
					continue;

				const auto glow_color = is_enemy ? settings::g_esp.m_glow.enemy_color : settings::g_esp.m_glow.teammate_color;

				// Write to m_clrRender for glow color
				g::memory.write( player.pawn + SCHEMA( "C_BaseModelEntity", "m_clrRender"_hash ), glow_color );

				// Also try to set glow in renderable
				const auto game_scene = g::memory.read<std::uintptr_t>( player.pawn + SCHEMA( "C_BaseEntity", "m_pGameSceneNode"_hash ) );
				if ( game_scene )
				{
					g::memory.write( game_scene + 0x60, glow_color );  // Scene node render color
				}
			}
		}

		// Chams - Override material colors
		if ( settings::g_esp.m_chams.enabled )
		{
			for ( const auto& player : systems::g_collector.players( ) )
			{
				const bool is_enemy = systems::g_local.is_enemy( player.team );
				if ( is_enemy && !settings::g_esp.m_chams.enemies )
					continue;
				if ( !is_enemy && !settings::g_esp.m_chams.teammates )
					continue;

				// Chams in CS2 requires material system manipulation
				// For now, use color override on render
				const auto cham_color = is_enemy ? settings::g_esp.m_chams.enemy_visible : settings::g_esp.m_chams.teammate_visible;
				g::memory.write( player.pawn + SCHEMA( "C_BaseModelEntity", "m_clrRender"_hash ), cham_color );

				const auto game_scene = g::memory.read<std::uintptr_t>( player.pawn + SCHEMA( "C_BaseEntity", "m_pGameSceneNode"_hash ) );
				if ( game_scene )
				{
					g::memory.write( game_scene + 0x60, cham_color );
				}
			}
		}
	}

	void visuals::on_render( )
	{
		// Player specific ESP overlays
		const auto players = systems::g_collector.players( );
		for ( const auto& player : players )
		{
			if ( !systems::g_local.is_enemy( player.team ) || player.invulnerable )
				continue;

			if ( settings::g_esp.m_player.m_visuals.m_china_hat.enabled )
				this->draw_china_hat( player );

			if ( settings::g_esp.m_player.m_head_direction.enabled )
				this->draw_head_direction( player );
		}

		// Spectator List
		if ( settings::g_esp.m_spectator_list.enabled )
		{
			this->draw_spectator_list( );
		}

		// Bullet Tracers
		if ( settings::g_esp.m_bullet_tracers.enabled )
		{
			this->draw_bullet_tracers( );
		}

		// Hit Marker
		if ( settings::g_esp.m_hit_marker.enabled )
		{
			this->draw_hit_marker( );
		}

		if ( !settings::g_esp.m_player.m_visuals.smoke_visualizer )
			return;

		const auto projects = systems::g_collector.projectiles( );
		for ( const auto& proj : projects )
		{
			if ( proj.subtype == systems::collector::projectile_subtype::smoke_grenade )
			{
				this->draw_smoke_visualizer( proj.origin );
			}
		}
	}

	void visuals::draw_spectator_list( )
	{
		const auto& cfg = settings::g_esp.m_spectator_list;
		const auto local_pawn = systems::g_local.pawn( );
		if ( !local_pawn )
			return;

		// Get local pawn handle
		const auto local_controller = systems::g_local.controller( );
		if ( !local_controller )
			return;

		const auto local_pawn_handle = g::memory.read<std::uint32_t>( local_controller + SCHEMA( "CCSPlayerController", "m_hPlayerPawn"_hash ) );
		if ( !local_pawn_handle )
			return;

		// Collect spectators
		std::vector<std::string> spectators;
		const auto raw_entities = systems::g_entities.all( );

		for ( const auto& entity : raw_entities )
		{
			if ( entity.type != systems::entities::type::player )
				continue;

			// Skip local player controller
			if ( entity.ptr == local_controller )
				continue;

			// Get the pawn for this controller
			const auto pawn_handle = g::memory.read<std::uint32_t>( entity.ptr + SCHEMA( "CCSPlayerController", "m_hPlayerPawn"_hash ) );
			if ( !pawn_handle )
				continue;

			// Get their observer services
			const auto pawn = systems::g_entities.lookup( pawn_handle );
			if ( !pawn )
				continue;

			const auto observer_services_offset = SCHEMA( "C_BasePlayerPawn", "m_pObserverServices"_hash );
			if ( observer_services_offset == 0 )
				continue;

			const auto observer_services = g::memory.read<std::uintptr_t>( pawn + observer_services_offset );
			if ( !observer_services )
				continue;

			// Check their observer target
			const auto observer_target_offset = SCHEMA( "CPlayer_ObserverServices", "m_hObserverTarget"_hash );
			if ( observer_target_offset == 0 )
				continue;

			const auto observer_target = g::memory.read<std::uint32_t>( observer_services + observer_target_offset );

			// If they're observing us
			if ( observer_target == local_pawn_handle )
			{
				// Get their name
				const auto name_ptr = g::memory.read<std::uintptr_t>( entity.ptr + SCHEMA( "CCSPlayerController", "m_sSanitizedPlayerName"_hash ) );
				if ( name_ptr )
				{
					char name_buffer[ 128 ] = { 0 };
					g::memory.read( name_ptr, name_buffer, sizeof( name_buffer ) - 1 );
					if ( name_buffer[ 0 ] )
					{
						spectators.push_back( std::string( name_buffer ) );
					}
				}
			}
		}

		// Draw the spectator list
		const auto pos_x = cfg.x;
		auto pos_y = cfg.y;

		// Background
		zdraw::rect_filled( pos_x, pos_y, 200.0f, 20.0f, zdraw::rgba( 20, 20, 20, 200 ) );
		zdraw::rect( pos_x, pos_y, 200.0f, 20.0f, zdraw::rgba( 100, 100, 100, 255 ), 1.0f );

		// Title
		zdraw::text<zdraw::tstyles::outlined>( pos_x + 5, pos_y + 3, "Spectators:", zdraw::rgba( 255, 255, 255, 255 ) );
		zdraw::pop_font( );

		pos_y += 20.0f;

		if ( spectators.empty( ) )
		{
			zdraw::text<zdraw::tstyles::outlined>( pos_x + 5, pos_y + 3, "None", zdraw::rgba( 150, 150, 150, 255 ) );
			zdraw::pop_font( );
		}
		else
		{
			for ( const auto& name : spectators )
			{
				zdraw::rect_filled( pos_x, pos_y, 200.0f, 18.0f, zdraw::rgba( 20, 20, 20, 200 ) );
				zdraw::text<zdraw::tstyles::outlined>( pos_x + 5, pos_y + 2, name.c_str( ), zdraw::rgba( 200, 200, 200, 255 ) );
				zdraw::pop_font( );
				pos_y += 18.0f;
			}
		}
	}

	void visuals::draw_hit_marker( )
	{
		const auto& cfg = settings::g_esp.m_hit_marker;
		const auto display = zdraw::get_display_size( );
		const auto center_x = display.first * 0.5f;
		const auto center_y = display.second * 0.5f;

		// Simple hit marker - draws an X at crosshair
		const float size = 10.0f;
		const float gap = 4.0f;

		// Draw X shape
		zdraw::line( center_x - size, center_y - size, center_x - gap, center_y - gap, cfg.color, 2.0f );
		zdraw::line( center_x + size, center_y - size, center_x + gap, center_y - gap, cfg.color, 2.0f );
		zdraw::line( center_x - size, center_y + size, center_x - gap, center_y + gap, cfg.color, 2.0f );
		zdraw::line( center_x + size, center_y + size, center_x + gap, center_y + gap, cfg.color, 2.0f );
	}

	void visuals::draw_bullet_tracers( )
	{
		const auto& cfg = settings::g_esp.m_bullet_tracers;
		const auto local_pawn = systems::g_local.pawn( );
		if ( !local_pawn )
			return;

		// Get weapon and eye position
		const auto weapon = systems::g_local.weapon( );
		if ( !weapon )
			return;

		// Get eye position from local pawn
		const auto game_scene = g::memory.read<std::uintptr_t>( local_pawn + SCHEMA( "C_BaseEntity", "m_pGameSceneNode"_hash ) );
		if ( !game_scene )
			return;

		const auto eye_pos = g::memory.read<math::vector3>( game_scene + SCHEMA( "CGameSceneNode", "m_vecAbsOrigin"_hash ) );
		
		// Get eye angles
		const auto eye_angles = g::memory.read<math::vector3>( local_pawn + SCHEMA( "C_CSPlayerPawn", "m_angEyeAngles"_hash ) );

		// Calculate forward direction
		math::vector3 forward;
		eye_angles.to_directions( &forward, nullptr, nullptr );

		// Calculate end point (tracer goes 2000 units - actual bullet range)
		const auto tracer_end = eye_pos + ( forward * 2000.0f );

		// Add impact to tracker
		features::misc::g_impacts.add_impact( eye_pos, tracer_end, cfg.tracer_color );

		// Also draw tracers for enemies shooting
		for ( const auto& player : systems::g_collector.players( ) )
		{
			if ( systems::g_local.is_enemy( player.team ) && player.health > 0 )
			{
				// Get their eye position
				const auto enemy_game_scene = g::memory.read<std::uintptr_t>( player.pawn + SCHEMA( "C_BaseEntity", "m_pGameSceneNode"_hash ) );
				if ( !enemy_game_scene )
					continue;

				const auto enemy_eye_pos = g::memory.read<math::vector3>( enemy_game_scene + SCHEMA( "CGameSceneNode", "m_vecAbsOrigin"_hash ) );
				const auto enemy_eye_angles = g::memory.read<math::vector3>( player.pawn + SCHEMA( "C_CSPlayerPawn", "m_angEyeAngles"_hash ) );

				math::vector3 enemy_forward;
				enemy_eye_angles.to_directions( &enemy_forward, nullptr, nullptr );

				const auto enemy_tracer_end = enemy_eye_pos + ( enemy_forward * 2000.0f );

				// Add impact to tracker
				features::misc::g_impacts.add_impact( enemy_eye_pos, enemy_tracer_end, cfg.tracer_color );
			}
		}
	}

	void visuals::draw_smoke_visualizer( const math::vector3& origin )
	{
		const auto& cfg = settings::g_esp.m_player.m_visuals;
		const auto radius = cfg.smoke_radius;
		const auto base_color = cfg.smoke_color;
		const auto style = cfg.style;

		auto draw_ring = [ & ]( const math::vector3& ring_origin, float r, const zdraw::rgba& color )
			{
				constexpr int segments = 32;
				std::vector<float> points;
				points.reserve( segments * 2 );

				for ( int i = 0; i < segments; ++i )
				{
					const auto angle = ( static_cast< float >( i ) / segments ) * 2.0f * std::numbers::pi_v<float>;
					const auto world_point = ring_origin + math::vector3{ r * std::cos( angle ), r * std::sin( angle ), 0.0f };

					const auto screen_point = systems::g_view.project( world_point );
					if ( systems::g_view.projection_valid( screen_point ) )
					{
						points.push_back( screen_point.x );
						points.push_back( screen_point.y );
					}
				}

				if ( points.size( ) >= 6 )
				{
					zdraw::polyline( std::span<const float>( points.data( ), points.size( ) ), color, true, 1.0f );
				}
			};

		if ( style == settings::esp::player::visual_utility::smoke_style::wireframe )
		{
			// Horizontal
			draw_ring( origin, radius, base_color );
			
			// Simple cross rings
			auto draw_vertical_ring = [ & ]( bool rot )
				{
					constexpr int segments = 32;
					std::vector<float> points;
					for ( int i = 0; i < segments; ++i )
					{
						const auto angle = ( static_cast< float >( i ) / segments ) * 2.0f * std::numbers::pi_v<float>;
						math::vector3 wp;
						if ( rot ) wp = origin + math::vector3{ radius * std::cos( angle ), 0.0f, radius * std::sin( angle ) };
						else wp = origin + math::vector3{ 0.0f, radius * std::cos( angle ), radius * std::sin( angle ) };

						const auto sp = systems::g_view.project( wp );
						if ( systems::g_view.projection_valid( sp ) )
						{
							points.push_back( sp.x );
							points.push_back( sp.y );
						}
					}
					if ( points.size( ) >= 6 )
						zdraw::polyline( std::span<const float>( points.data( ), points.size( ) ), base_color, true, 1.0f );
				};

			draw_vertical_ring( true );
			draw_vertical_ring( false );
		}
		else
		{
			// Solid, Gradient, or Full
			int ring_count = 8;
			if ( style == settings::esp::player::visual_utility::smoke_style::gradient )
				ring_count = 16;
			else if ( style == settings::esp::player::visual_utility::smoke_style::full )
				ring_count = 32; // Much denser for "full" look
			
			for ( int i = 0; i <= ring_count; ++i )
			{
				const float h_frac = ( static_cast< float >( i ) / ring_count ) * 2.0f - 1.0f; // -1 to 1
				const float r = radius * std::sqrt( 1.0f - h_frac * h_frac );
				const float z = radius * h_frac;
				
				zdraw::rgba ring_color = base_color;
				if ( style == settings::esp::player::visual_utility::smoke_style::gradient )
				{
					// Fade out at poles
					float alpha_mult = 1.0f - std::abs( h_frac );
					ring_color.a = static_cast< std::uint8_t >( base_color.a * alpha_mult );
				}

				draw_ring( origin + math::vector3{ 0, 0, z }, r, ring_color );
			}
		}
	}

	void visuals::draw_china_hat( const systems::collector::player& player )
	{
		const auto& cfg = settings::g_esp.m_player.m_visuals.m_china_hat;
		const auto bones = systems::g_bones.get( player.bone_cache );
		if ( !cfg.enabled || !bones.is_valid( ) )
			return;

		// Hitbox 6 is usually the head in CS2
		math::vector3 head_pos{};
		bool found_head = false;
		for ( const auto& hb : player.hitboxes )
		{
			if ( hb.index == 6 && hb.bone >= 0 ) // head hitbox
			{
				head_pos = bones.get_position( hb.bone );
				found_head = true;
				break;
			}
		}

		if ( !found_head )
			return;

		const float radius = cfg.radius;
		const float height = cfg.radius * 0.8f; // Make it a bit flatter than a perfect cone
		
		math::vector3 apex = head_pos;
		apex.z += height + 5.0f; // Offset slightly above the actual bone

		math::vector2 screen_apex;
		if ( !systems::g_view.projection_valid( screen_apex = systems::g_view.project( apex ) ) )
			return;

		constexpr int segments = 16;
		std::vector<math::vector2> screen_points;
		screen_points.reserve( segments );

		for ( int i = 0; i < segments; ++i )
		{
			const float angle = ( static_cast< float >( i ) / segments ) * 2.0f * std::numbers::pi_v<float>;
			math::vector3 rim_point = head_pos;
			rim_point.x += radius * std::cos( angle );
			rim_point.y += radius * std::sin( angle );
			rim_point.z += 5.0f; // Base of the hat

			const auto sp = systems::g_view.project( rim_point );
			if ( !systems::g_view.projection_valid( sp ) )
				return; // If any part of the base is off-screen, skip to avoid weird stretching

			screen_points.push_back( sp );
		}

		// Draw the filled cone (triangles from apex to rim)
		for ( std::size_t i = 0; i < screen_points.size( ); ++i )
		{
			const auto& p1 = screen_points[ i ];
			const auto& p2 = screen_points[ ( i + 1 ) % screen_points.size( ) ];
			zdraw::triangle_filled( screen_apex.x, screen_apex.y, p1.x, p1.y, p2.x, p2.y, cfg.color );
		}

		// Draw the rim outline
		std::vector<float> rim_line;
		rim_line.reserve( segments * 2 );
		for ( const auto& p : screen_points )
		{
			rim_line.push_back( p.x );
			rim_line.push_back( p.y );
		}
		
		if ( rim_line.size( ) >= 6 )
		{
			zdraw::rgba outline_col{ cfg.color.r, cfg.color.g, cfg.color.b, static_cast< std::uint8_t >( std::min( 255, cfg.color.a + 100 ) ) };
			zdraw::polyline( std::span<const float>( rim_line.data( ), rim_line.size( ) ), outline_col, true, 1.0f );
			
			// Draw lines from apex to rim for the "hat" look
			for ( const auto& p : screen_points )
			{
				zdraw::line( screen_apex.x, screen_apex.y, p.x, p.y, outline_col, 1.0f );
			}
		}
	}

	void visuals::draw_head_direction( const systems::collector::player& player )
	{
		const auto& cfg = settings::g_esp.m_player.m_head_direction;
		const auto bones = systems::g_bones.get( player.bone_cache );
		if ( !cfg.enabled || !bones.is_valid( ) )
			return;

		math::vector3 head_pos{};
		bool found_head = false;
		for ( const auto& hb : player.hitboxes )
		{
			if ( hb.index == 6 && hb.bone >= 0 ) // head hitbox
			{
				head_pos = bones.get_position( hb.bone );
				found_head = true;
				break;
			}
		}

		if ( !found_head )
			return;

		// Read eye angles from the pawn
		const auto eye_angles = g::memory.read<math::vector3>( player.pawn + SCHEMA( "C_CSPlayerPawn", "m_angEyeAngles"_hash ) );
		
		math::vector3 forward{};
		eye_angles.to_directions( &forward, nullptr, nullptr );

		const math::vector3 end_pos = head_pos + ( forward * cfg.length );

		const auto screen_start = systems::g_view.project( head_pos );
		const auto screen_end = systems::g_view.project( end_pos );

		if ( systems::g_view.projection_valid( screen_start ) && systems::g_view.projection_valid( screen_end ) )
		{
			zdraw::line( screen_start.x, screen_start.y, screen_end.x, screen_end.y, cfg.color, 1.5f );
			
			// Optional: draw a small box at the end to make it look like an arrow/pointer
			zdraw::rect_filled( screen_end.x - 1.5f, screen_end.y - 1.5f, 3.0f, 3.0f, cfg.color );
		}
	}

} // namespace features::misc
