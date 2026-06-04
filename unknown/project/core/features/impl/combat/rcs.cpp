#include <stdafx.hpp>
#include <core/features/features.hpp>

namespace features::combat {

	void rcs::tick( )
	{
		if ( !settings::g_combat.m_rcs.enabled )
			return;

		const auto local_pawn = systems::g_local.pawn( );
		if ( !local_pawn )
			return;

		const auto shots_fired = g::memory.read<std::int32_t>( local_pawn + SCHEMA( "C_CSPlayerPawn", "m_iShotsFired"_hash ) );
		if ( shots_fired <= 0 )
		{
			this->m_last_punch = {};
			this->m_rcs_error = {};
			return;
		}

		const auto aim_punch = g::memory.read<math::vector3>( local_pawn + SCHEMA( "C_CSPlayerPawn", "m_aimPunchAngle"_hash ) );
		
		if ( shots_fired > 1 )
		{
			// Calculate delta punch
			const auto delta = ( aim_punch - this->m_last_punch );

			// Get conversion factor: degrees to pixels
			constexpr auto m_yaw{ 0.022f };
			const auto sensitivity = systems::g_convars.get<float>( CONVAR( "sensitivity"_hash ) );
			const auto fov_adjust = g::memory.read<float>( local_pawn + SCHEMA( "C_BasePlayerPawn", "m_flFOVSensitivityAdjust"_hash ) );
			const auto deg_per_pixel = sensitivity * m_yaw * fov_adjust;

			if ( deg_per_pixel > 0.0f )
			{
				// Recoil is usually 2x punch. We want to move mouse opposite to punch increase.
				// Punch up (delta.x -ve) -> move mouse down (+ve Y in screen/input)
				// Punch right (delta.y +ve) -> move mouse left (-ve X in screen/input)
				const auto move_x = -( delta.y * settings::g_combat.m_rcs.factor * 2.0f ) / deg_per_pixel;
				const auto move_y = ( delta.x * settings::g_combat.m_rcs.factor * 2.0f ) / deg_per_pixel;

				this->m_rcs_error.x += move_x;
				this->m_rcs_error.y += move_y;

				const auto dx = static_cast< int >( this->m_rcs_error.x );
				const auto dy = static_cast< int >( this->m_rcs_error.y );

				this->m_rcs_error.x -= static_cast< float >( dx );
				this->m_rcs_error.y -= static_cast< float >( dy );

				if ( dx != 0 || dy != 0 )
				{
					g::input.inject_mouse( dx, dy, input::move );
				}
			}
		}
		
		this->m_last_punch = aim_punch;
	}

} // namespace features::combat
