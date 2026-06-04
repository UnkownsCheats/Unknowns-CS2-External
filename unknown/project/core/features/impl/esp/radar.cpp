#include <stdafx.hpp>

namespace features::esp {

	void radar::on_render( )
	{
		const auto& cfg = settings::g_esp.m_radar;
		if ( !cfg.enabled )
		{
			return;
		}

		const auto radar_pos = math::vector2{ cfg.x, cfg.y };
		const auto radar_size = cfg.size;
		const auto radar_center = radar_pos + math::vector2{ radar_size * 0.5f, radar_size * 0.5f };

		// Background
		zdraw::rect_filled( radar_pos.x, radar_pos.y, radar_size, radar_size, zdraw::rgba( 20, 20, 20, 160 ) );
		zdraw::rect( radar_pos.x, radar_pos.y, radar_size, radar_size, zdraw::rgba( 100, 100, 100, 255 ), 1.0f );
		
		// Crosshair on radar
		zdraw::line( radar_center.x, radar_pos.y, radar_center.x, radar_pos.y + radar_size, zdraw::rgba( 150, 150, 150, 100 ) );
		zdraw::line( radar_pos.x, radar_center.y, radar_pos.x + radar_size, radar_center.y, zdraw::rgba( 150, 150, 150, 100 ) );

		if ( !systems::g_local.valid( ) )
		{
			return;
		}

		// Local Player facing direction
		const auto view_angles = systems::g_view.angles( );
		const auto forward_rad = math::helpers::deg_to_rad( view_angles.y - 90.0f );
		zdraw::line( radar_center.x, radar_center.y, radar_center.x + std::cos( forward_rad ) * 15.0f, radar_center.y + std::sin( forward_rad ) * 15.0f, zdraw::rgba( 255, 255, 255, 200 ) );

		const auto local_pawn = systems::g_local.pawn( );
		if ( !local_pawn )
		{
			return;
		}

		const auto game_scene = g::memory.read<std::uintptr_t>( local_pawn + SCHEMA( "C_BaseEntity", "m_pGameSceneNode"_hash ) );
		if ( !game_scene )
		{
			return;
		}

		const auto local_origin = g::memory.read<math::vector3>( game_scene + SCHEMA( "CGameSceneNode", "m_vecAbsOrigin"_hash ) );

		for ( const auto& p : systems::g_collector.players( ) )
		{
			if ( !cfg.show_teammates && !systems::g_local.is_enemy( p.team ) )
			{
				continue;
			}

			const auto delta = p.origin - local_origin;
			
			// Map relative to local player
			auto rel_x = delta.y;
			auto rel_y = -delta.x;

			// Rotate by local player view angles
			const auto angle = math::helpers::deg_to_rad( view_angles.y - 90.0f );
			const auto cos_a = std::cos( -angle );
			const auto sin_a = std::sin( -angle );

			const auto rotated_x = rel_x * cos_a - rel_y * sin_a;
			const auto rotated_y = rel_x * sin_a + rel_y * cos_a;

			const auto final_x = rotated_x * 0.05f * cfg.scale;
			const auto final_y = rotated_y * 0.05f * cfg.scale;

			// Clip to radar bounds
			if ( std::abs( final_x ) > radar_size * 0.5f || std::abs( final_y ) > radar_size * 0.5f )
			{
				continue;
			}

			const auto dot_pos = radar_center + math::vector2{ final_x, final_y };
			const auto is_enemy = systems::g_local.is_enemy( p.team );
			const auto color = is_enemy ? zdraw::rgba( 235, 90, 90, 255 ) : zdraw::rgba( 90, 160, 235, 255 );
			
			zdraw::rect_filled( dot_pos.x - 2, dot_pos.y - 2, 4, 4, color );
			zdraw::rect( dot_pos.x - 3, dot_pos.y - 3, 6, 6, zdraw::rgba( 0, 0, 0, 180 ), 1.0f );
		}
	}

} // namespace features::esp
