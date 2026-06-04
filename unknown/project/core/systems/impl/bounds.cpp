#include <stdafx.hpp>

namespace
{
	[[nodiscard]] bool finite( const math::vector2& value )
	{
		return std::isfinite( value.x ) && std::isfinite( value.y );
	}

	[[nodiscard]] bool finite( const math::vector3& value )
	{
		return std::isfinite( value.x ) && std::isfinite( value.y ) && std::isfinite( value.z );
	}

	[[nodiscard]] bool usable_world_point( const math::vector3& value )
	{
		return finite( value ) && !( value.x == 0.0f && value.y == 0.0f && value.z == 0.0f );
	}

	[[nodiscard]] bool usable_screen_point( const math::vector2& value )
	{
		if ( !systems::g_view.projection_valid( value ) || !finite( value ) )
		{
			return false;
		}

		const auto [ width, height ] = zdraw::get_display_size( );
		const auto margin_x = std::max( width * 0.75f, 300.0f );
		const auto margin_y = std::max( height * 0.75f, 300.0f );

		return value.x > -margin_x && value.x < width + margin_x && value.y > -margin_y && value.y < height + margin_y;
	}

	[[nodiscard]] systems::bounds::data invalid_bounds( )
	{
		return { { static_cast<float>( 0xdead ), static_cast<float>( 0xdead ) }, { } };
	}

	[[nodiscard]] bool sane_bounds( const systems::bounds::data& data )
	{
		if ( !finite( data.min ) || !finite( data.max ) )
		{
			return false;
		}

		const auto width = data.width( );
		const auto height = data.height( );
		if ( width < 2.0f || height < 4.0f )
		{
			return false;
		}

		const auto [ display_width, display_height ] = zdraw::get_display_size( );
		if ( width > display_width * 1.5f || height > display_height * 2.0f )
		{
			return false;
		}

		const auto aspect = width / height;
		return aspect >= 0.12f && aspect <= 1.25f;
	}
}

namespace systems {

	bool bounds::data::is_valid( ) const
	{
		return this->min.x != static_cast<float>( 0xdead ) && sane_bounds( *this );
	}

	bounds::data bounds::get( const bones::data& bone_data ) const
	{
		const auto head = bone_data.get_position( 6 );
		if ( usable_world_point( head ) )
		{
			auto lowest = head;
			for ( const auto& bone : bone_data.bones )
			{
				if ( usable_world_point( bone.position ) && bone.position.z < lowest.z )
				{
					lowest = bone.position;
				}
			}

			const auto top = g_view.project( head + math::vector3{ 0.0f, 0.0f, 8.0f } );
			const auto bottom = g_view.project( lowest - math::vector3{ 0.0f, 0.0f, 4.0f } );

			if ( usable_screen_point( top ) && usable_screen_point( bottom ) )
			{
				const auto height = std::fabsf( bottom.y - top.y );
				if ( height >= 4.0f )
				{
					const auto [ display_width, _ ] = zdraw::get_display_size( );
					const auto width = std::clamp( height * 0.42f, 3.0f, display_width * 0.45f );
					const auto center_x = ( top.x + bottom.x ) * 0.5f;

					const auto result = data
					{
						{ center_x - width * 0.5f, std::min( top.y, bottom.y ) },
						{ center_x + width * 0.5f, std::max( top.y, bottom.y ) }
					};

					if ( sane_bounds( result ) )
					{
						return result;
					}
				}
			}
		}

		auto screen_min = math::vector2{ std::numeric_limits<float>::max( ), std::numeric_limits<float>::max( ) };
		auto screen_max = math::vector2{ std::numeric_limits<float>::lowest( ), std::numeric_limits<float>::lowest( ) };
		auto valid_points{ 0 };

		for ( const auto& bone : bone_data.bones )
		{
			if ( !usable_world_point( bone.position ) )
			{
				continue;
			}

			const auto projected = g_view.project( bone.position );
			if ( !usable_screen_point( projected ) )
			{
				continue;
			}

			++valid_points;
			screen_min.x = std::min( screen_min.x, projected.x );
			screen_min.y = std::min( screen_min.y, projected.y );
			screen_max.x = std::max( screen_max.x, projected.x );
			screen_max.y = std::max( screen_max.y, projected.y );
		}

		if ( valid_points < 2 )
		{
			return invalid_bounds( );
		}

		const auto height = screen_max.y - screen_min.y;
		const auto width = std::max( screen_max.x - screen_min.x, height * 0.35f );
		const auto center_x = ( screen_min.x + screen_max.x ) * 0.5f;
		const auto pad = std::clamp( height * 0.05f, 1.0f, 8.0f );
		const auto result = data
		{
			{ center_x - width * 0.5f - pad, screen_min.y - pad },
			{ center_x + width * 0.5f + pad, screen_max.y + pad }
		};

		return sane_bounds( result ) ? result : invalid_bounds( );
	}

} // namespace systems
