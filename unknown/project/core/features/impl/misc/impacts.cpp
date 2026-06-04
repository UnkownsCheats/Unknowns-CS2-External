#include <stdafx.hpp>

namespace features::misc {

	void impacts::on_render( )
	{
		const auto& cfg = settings::g_esp.m_bullet_tracers;
		if ( !cfg.enabled )
			return;

		const auto global_vars = g::memory.read<std::uintptr_t>( g::offsets.global_vars );
		if ( !global_vars )
			return;

		const auto current_time = g::memory.read<float>( global_vars + 0x30 );

		// Remove old impacts
		m_impacts.erase(
			std::remove_if( m_impacts.begin( ), m_impacts.end( ),
				[ current_time, &cfg ]( const impact_data& data )
				{
					return ( current_time - data.time ) > cfg.tracer_duration;
				} ),
			m_impacts.end( )
		);

		// Draw impacts
		for ( const auto& impact : m_impacts )
		{
			const auto screen_start = systems::g_view.project( impact.start );
			const auto screen_end = systems::g_view.project( impact.end );

			if ( systems::g_view.projection_valid( screen_start ) && systems::g_view.projection_valid( screen_end ) )
			{
				if ( cfg.tracers )
				{
					const auto alpha = std::clamp( 1.0f - ( current_time - impact.time ) / cfg.tracer_duration, 0.0f, 1.0f );
					const auto faded_color = zdraw::rgba( impact.color.r, impact.color.g, impact.color.b, static_cast< std::uint8_t >( impact.color.a * alpha ) );
					
					zdraw::line( screen_start.x, screen_start.y, screen_end.x, screen_end.y, faded_color, 2.0f );
				}

				if ( cfg.impacts )
				{
					const auto alpha = std::clamp( 1.0f - ( current_time - impact.time ) / cfg.tracer_duration, 0.0f, 1.0f );
					const auto faded_color = zdraw::rgba( cfg.impact_color.r, cfg.impact_color.g, cfg.impact_color.b, static_cast< std::uint8_t >( cfg.impact_color.a * alpha ) );
					
					zdraw::rect_filled( 
						screen_end.x - cfg.impact_size * 0.5f, 
						screen_end.y - cfg.impact_size * 0.5f, 
						cfg.impact_size, 
						cfg.impact_size, 
						faded_color 
					);
				}
			}
		}
	}

	void impacts::add_impact( const math::vector3& start, const math::vector3& end, const zdraw::rgba& color )
	{
		const auto global_vars = g::memory.read<std::uintptr_t>( g::offsets.global_vars );
		if ( !global_vars )
			return;

		const auto current_time = g::memory.read<float>( global_vars + 0x30 );

		impact_data data{};
		data.start = start;
		data.end = end;
		data.color = color;
		data.time = current_time;

		m_impacts.push_back( data );
	}

} // namespace features::misc
