#include <stdafx.hpp>

namespace features::misc {

	void sound::tick( )
	{
		static auto pan_exponent_ptr = systems::g_convars.find( "snd_headphone_pan_exponent"_hash );
		static auto original_pan_exponent = 1.0f;
		static bool first_run = true;

		if ( first_run && pan_exponent_ptr )
		{
			original_pan_exponent = systems::g_convars.get<float>( pan_exponent_ptr );
			first_run = false;
		}

		if ( !settings::g_misc.m_sound_enhancement.enabled )
		{
			if ( pan_exponent_ptr && !first_run )
			{
				const auto current = systems::g_convars.get<float>( pan_exponent_ptr );
				if ( std::abs( current - original_pan_exponent ) > 0.01f )
				{
					g::memory.write( pan_exponent_ptr + 0x58, original_pan_exponent );
				}
			}
			return;
		}

		if ( pan_exponent_ptr )
		{
			// Higher pan exponent (up to 2.0) makes directional sounds sharper and more prominent
			const auto target = original_pan_exponent * settings::g_misc.m_sound_enhancement.volume_multiplier;
			const auto current = systems::g_convars.get<float>( pan_exponent_ptr );

			if ( std::abs( current - target ) > 0.01f )
			{
				g::memory.write( pan_exponent_ptr + 0x58, target );
			}
		}
	}

} // namespace features::misc
