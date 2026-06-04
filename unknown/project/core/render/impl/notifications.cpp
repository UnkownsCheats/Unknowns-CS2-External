#include <stdafx.hpp>
#include <core/render/notifications.hpp>

namespace notifications {

	void notification_manager::add( const std::string& message, float duration )
	{
		notification_t n{};
		n.message = message;
		n.start_time = std::chrono::steady_clock::now( );
		n.end_time = n.start_time + std::chrono::milliseconds( static_cast< int >( duration * 1000.0f ) );
		
		this->m_notifications.insert( this->m_notifications.begin( ), n );
	}

	void notification_manager::draw( )
	{
		if ( this->m_notifications.empty( ) )
			return;

		const auto now = std::chrono::steady_clock::now( );
		float current_y = 20.0f;

		for ( auto it = this->m_notifications.begin( ); it != this->m_notifications.end( ); )
		{
			const auto elapsed = std::chrono::duration_cast< std::chrono::milliseconds >( now - it->start_time ).count( ) / 1000.0f;
			const auto total_duration = std::chrono::duration_cast< std::chrono::milliseconds >( it->end_time - it->start_time ).count( ) / 1000.0f;
			const auto remaining = total_duration - elapsed;

			if ( remaining <= 0.0f )
			{
				it = this->m_notifications.erase( it );
				continue;
			}

			// Animations
			float target_alpha = 1.0f;
			if ( elapsed < 0.2f ) // Fade in
				target_alpha = elapsed / 0.2f;
			else if ( remaining < 0.5f ) // Fade out
				target_alpha = remaining / 0.5f;

			it->alpha = std::lerp( it->alpha, target_alpha, 0.1f ); // Smooth alpha
			
			// Drawing
			const auto text_size = zdraw::measure_text( it->message, g::render.fonts( ).mochi_12 );
			const float w = text_size.first + 20.0f;
			const float h = 28.0f;
			const float x = 20.0f; // Bottom left or top left? Let's go top left for now.

			const auto bg_col = zdraw::rgba( 15, 15, 20, static_cast< std::uint8_t >( 200 * it->alpha ) );
			const auto border_col = zdraw::rgba( 45, 45, 55, static_cast< std::uint8_t >( 255 * it->alpha ) );
			const auto accent_col = zui::get_accent_color( );
			const auto text_col = zdraw::rgba( 220, 220, 230, static_cast< std::uint8_t >( 255 * it->alpha ) );

			// Background
			zdraw::rect_filled( x, current_y, w, h, bg_col );
			zdraw::rect( x, current_y, w, h, border_col );
			
			// Accent bar
			zdraw::rect_filled( x, current_y, 3.0f, h, zdraw::rgba( accent_col.r, accent_col.g, accent_col.b, static_cast< std::uint8_t >( 255 * it->alpha ) ) );

			// Text
			zdraw::text( x + 12.0f, current_y + h * 0.5f - text_size.second * 0.5f, it->message, text_col, g::render.fonts( ).mochi_12 );

			current_y += ( h + 8.0f ) * it->alpha;
			++it;
		}
	}

} // namespace notifications
