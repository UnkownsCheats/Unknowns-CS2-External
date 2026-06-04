#include <stdafx.hpp>

namespace features::misc {

	void crosshair::on_render( )
	{
		const auto& cfg = settings::g_misc.m_crosshair;
		if ( !cfg.enabled )
		{
			return;
		}

		const auto ds = zdraw::get_display_size( );
		const auto x = static_cast< float >( ds.first ) * 0.5f;
		const auto y = static_cast< float >( ds.second ) * 0.5f;


		const auto color = cfg.color;
		const auto outline_color = zdraw::rgba( 0, 0, 0, 200 );
		const auto thickness = cfg.thickness;
		const auto length = cfg.length;
		const auto gap = cfg.gap;

		if ( cfg.outline )
		{
			// Left
			zdraw::rect_filled( x - gap - length - 1.0f, y - thickness * 0.5f - 1.0f, length + 2.0f, thickness + 2.0f, outline_color );
			// Right
			zdraw::rect_filled( x + gap - 1.0f, y - thickness * 0.5f - 1.0f, length + 2.0f, thickness + 2.0f, outline_color );
			// Top
			zdraw::rect_filled( x - thickness * 0.5f - 1.0f, y - gap - length - 1.0f, thickness + 2.0f, length + 2.0f, outline_color );
			// Bottom
			zdraw::rect_filled( x - thickness * 0.5f - 1.0f, y + gap - 1.0f, thickness + 2.0f, length + 2.0f, outline_color );
		}

		// Left
		zdraw::rect_filled( x - gap - length, y - thickness * 0.5f, length, thickness, color );
		// Right
		zdraw::rect_filled( x + gap, y - thickness * 0.5f, length, thickness, color );
		// Top
		zdraw::rect_filled( x - thickness * 0.5f, y - gap - length, thickness, length, color );
		// Bottom
		zdraw::rect_filled( x - thickness * 0.5f, y + gap, thickness, length, color );
	}

} // namespace features::misc
