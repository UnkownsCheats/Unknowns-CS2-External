#include <stdafx.hpp>

namespace features::esp {

	void bomb::on_render( )
	{
		const auto& cfg = settings::g_esp.m_bomb;
		if ( !cfg.enabled )
		{
			return;
		}

		const auto bomb_opt = systems::g_collector.bomb( );
		if ( !bomb_opt.has_value( ) )
		{
			return;
		}

		const auto& bomb = bomb_opt.value( );
		const auto global_vars = g::memory.read<std::uintptr_t>( g::offsets.global_vars );
		if ( !global_vars )
		{
			return;
		}

		const auto current_time = g::memory.read<float>( global_vars + 0x30 );
		const auto remaining = bomb.blow_time - current_time;

		if ( remaining <= 0.0f )
		{
			return;
		}

		const auto screen = systems::g_view.project( bomb.origin );
		const auto ds = zdraw::get_display_size( );
		const auto x_center = static_cast< float >( ds.first ) * 0.5f;

		// Pulsing logic
			// Refined discrete pulse rhythm to match actual bomb beeps
			auto interval = 1.0f;
			if ( remaining < 5.0f ) interval = 0.25f;
			else if ( remaining < 10.0f ) interval = 0.5f;

			const auto pulse = ( std::sin( current_time * ( std::numbers::pi_v<float> * 2.0f / interval ) ) + 1.0f ) * 0.5f;
			
			// Pulse the icon/status but keep timer constant
			const auto pulse_color = zdraw::rgba{ 255, static_cast< std::uint8_t >( 255 * ( 1.0f - pulse ) ), static_cast< std::uint8_t >( 255 * ( 1.0f - pulse ) ), 255 };
			const auto timer_color = zdraw::rgba{ 255, 255, 255, 255 }; // Constant white for timer

		if ( !systems::g_view.projection_valid( screen ) )
		{
			const auto& fonts = g::render.fonts( );
			const auto timer_str = std::format( "{:.2f}s", remaining );
			const auto [tw, th] = zdraw::measure_text( timer_str, fonts.mochi_24 );

			const auto display = zdraw::get_display_size( );
			const auto center_x = display.first * 0.5f;
			const auto center_y = display.second * 0.2f;

			// Icon (Pulsing)
			zdraw::text<zdraw::tstyles::outlined>( center_x - 30, center_y - 20, "o", pulse_color, fonts.weapons_15 );
			
			// Status (Pulsing)
			const auto status_str = bomb.is_being_defused ? "BEING DEFUSED" : "PLANTED";
			const auto [sw, sh] = zdraw::measure_text( status_str, fonts.mochi_12 );
			zdraw::text<zdraw::tstyles::outlined>( center_x - sw * 0.5f, center_y - 15, status_str, pulse_color, fonts.mochi_12 );

			// Timer (Larger, Constant)
			zdraw::text<zdraw::tstyles::outlined>( center_x - tw * 0.5f, center_y + 5, timer_str, timer_color, fonts.mochi_24 );

			if ( bomb.is_being_defused )
			{
				const auto defuse_remaining = bomb.defuse_countdown - current_time;
				if ( defuse_remaining > 0.0f )
				{
					const auto defuse_str = std::format( "DEFUSE: {:.1f}s", defuse_remaining );
					const auto [defuse_w, defuse_h] = zdraw::measure_text( defuse_str, fonts.mochi_12 );
					zdraw::text<zdraw::tstyles::outlined>( center_x - defuse_w * 0.5f, center_y + th + 10, defuse_str, zdraw::rgba( 100, 150, 255, 255 ), fonts.mochi_12 );
				}
			}
			return;
		}

		// Draw world icon with pulse
		const auto radius = 8.0f + 4.0f * pulse;
		zdraw::circle_filled( screen.x, screen.y, radius, cfg.color );
		zdraw::circle( screen.x, screen.y, radius, zdraw::rgba( 0, 0, 0, 200 ), 1.0f );
		
		char buf[ 32 ]{};
		sprintf_s( buf, "%.1fs", remaining );
		const auto [text_w, text_h] = zdraw::measure_text( buf );
		zdraw::text<zdraw::tstyles::outlined>( screen.x - text_w * 0.5f, screen.y - radius - text_h - 2.0f, buf, timer_color );

		if ( bomb.is_being_defused )
		{
			const auto defuse_remaining = bomb.defuse_countdown - current_time;
			if ( defuse_remaining > 0.0f )
			{
				sprintf_s( buf, "DEFUSING: %.1fs", defuse_remaining );
				const auto [defuse_w, defuse_h] = zdraw::measure_text( buf );
				zdraw::text<zdraw::tstyles::outlined>( screen.x - defuse_w * 0.5f, screen.y + radius + 2.0f, buf, zdraw::rgba( 100, 150, 255, 255 ) );
			}
		}
	}

} // namespace features::esp
