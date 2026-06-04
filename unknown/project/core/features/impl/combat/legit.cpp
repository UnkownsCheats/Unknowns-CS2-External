#include <stdafx.hpp>

namespace
{
	struct hitbox_capsule
	{
		math::vector3 center{};
		math::vector3 start{};
		math::vector3 end{};
		float radius{};
	};

	[[nodiscard]] bool finite( const math::vector3& value )
	{
		return std::isfinite( value.x ) && std::isfinite( value.y ) && std::isfinite( value.z );
	}

	[[nodiscard]] bool ray_hits_sphere( const math::vector3& ray_origin, const math::vector3& ray_dir, const math::vector3& center, float radius, float& ray_dist )
	{
		const auto oc = ray_origin - center;
		const auto a = ray_dir.dot( ray_dir );
		const auto b = 2.0f * oc.dot( ray_dir );
		const auto c = oc.dot( oc ) - radius * radius;
		const auto discriminant = b * b - 4.0f * a * c;

		if ( discriminant < 0.0f )
		{
			return false;
		}

		const auto sqrt_d = std::sqrtf( discriminant );
		auto t = ( -b - sqrt_d ) / ( 2.0f * a );

		if ( t < 0.0f )
		{
			t = ( -b + sqrt_d ) / ( 2.0f * a );
		}

		if ( t < 0.0f )
		{
			return false;
		}

		ray_dist = t;
		return true;
	}

	[[nodiscard]] bool ray_hits_capsule( const math::vector3& ray_origin, const math::vector3& ray_dir, const math::vector3& capsule_start, const math::vector3& capsule_end, float radius, float& ray_dist )
	{
		const auto capsule_vec = capsule_end - capsule_start;
		const auto capsule_len_sq = capsule_vec.length_sqr( );

		if ( capsule_len_sq < 0.001f )
		{
			return ray_hits_sphere( ray_origin, ray_dir, capsule_start, radius, ray_dist );
		}

		const auto w0 = ray_origin - capsule_start;
		const auto a = ray_dir.dot( ray_dir );
		const auto b = ray_dir.dot( capsule_vec );
		const auto c = capsule_vec.dot( capsule_vec );
		const auto d = ray_dir.dot( w0 );
		const auto e = capsule_vec.dot( w0 );
		const auto denom = a * c - b * b;

		float ray_t = 0.0f;
		float seg_t = 0.0f;

		if ( std::fabsf( denom ) < 0.0001f )
		{
			seg_t = std::clamp( e / c, 0.0f, 1.0f );
			ray_t = std::max( 0.0f, ( b * seg_t - d ) / a );
		}
		else
		{
			ray_t = ( b * e - c * d ) / denom;
			seg_t = ( a * e - b * d ) / denom;

			if ( ray_t < 0.0f )
			{
				ray_t = 0.0f;
				seg_t = std::clamp( e / c, 0.0f, 1.0f );
			}
			else
			{
				seg_t = std::clamp( seg_t, 0.0f, 1.0f );
				ray_t = std::max( 0.0f, ( b * seg_t - d ) / a );
			}
		}

		const auto point_on_ray = ray_origin + ray_dir * ray_t;
		const auto point_on_capsule = capsule_start + capsule_vec * seg_t;

		if ( ( point_on_ray - point_on_capsule ).length_sqr( ) > radius * radius )
		{
			return false;
		}

		ray_dist = ray_t;
		return true;
	}

	[[nodiscard]] bool build_hitbox_capsule( const systems::hitboxes::entry& hb, const systems::bones::data& bones, const math::vector3& velocity, float prediction_time, hitbox_capsule& out )
	{
		if ( hb.index < 0 || hb.bone < 0 || hb.bone >= static_cast<int>( bones.bones.size( ) ) )
		{
			return false;
		}

		const auto& bone = bones.bones[ hb.bone ];
		if ( !finite( bone.position ) || !finite( hb.mins ) || !finite( hb.maxs ) )
		{
			return false;
		}

		const auto center_local = ( hb.mins + hb.maxs ) * 0.5f;
		const auto center_world = bone.position + math::helpers::rotate_by_quat( bone.rotation, center_local ) + velocity * prediction_time;

		const auto half_extent = ( hb.maxs - hb.mins ) * 0.5f;
		const auto longest = std::max( { std::abs( half_extent.x ), std::abs( half_extent.y ), std::abs( half_extent.z ) } );

		math::vector3 axis_local{};
		if ( longest > 0.001f )
		{
			if ( std::abs( half_extent.x ) >= std::abs( half_extent.y ) && std::abs( half_extent.x ) >= std::abs( half_extent.z ) )
			{
				axis_local = { longest, 0.0f, 0.0f };
			}
			else if ( std::abs( half_extent.y ) >= std::abs( half_extent.z ) )
			{
				axis_local = { 0.0f, longest, 0.0f };
			}
			else
			{
				axis_local = { 0.0f, 0.0f, longest };
			}
		}

		const auto axis_world = math::helpers::rotate_by_quat( bone.rotation, axis_local );

		out.center = center_world;
		out.start = center_world - axis_world;
		out.end = center_world + axis_world;
		out.radius = hb.radius > 0.0f ? hb.radius : 4.0f;

		return finite( out.center ) && finite( out.start ) && finite( out.end );
	}
}

namespace features::combat {

	void legit::on_render( )
	{
		const auto eye_pos = systems::g_view.origin( );
		const auto view_angles = systems::g_view.angles( );

		const auto& ctx = g_shared.ctx( );
		if ( !ctx.valid )
		{
			return;
		}

		const auto valid_weapon = cstypes::is_weapon_valid( ctx.weapon_type );
		const auto& cfg = settings::g_combat.get( ctx.weapon_type );

		this->m_fov_alpha.set_target( valid_weapon && cfg.aimbot.draw_fov && cfg.aimbot.enabled ? 1.0f : 0.0f );
		this->m_fov_alpha.update( );

		if ( this->m_fov_alpha.value( ) <= 0.01f )
		{
			return;
		}

		this->draw_fov( eye_pos, view_angles, cfg.aimbot );
	}

	void legit::tick( )
	{
		if ( !this->m_rng_seeded )
		{
			this->m_rng.seed( static_cast< int >( std::chrono::steady_clock::now( ).time_since_epoch( ).count( ) & 0x7fffffff ) );
			this->m_rng_seeded = true;
		}

		if ( this->m_trigger_held )
		{
			const auto& ctx = g_shared.ctx( );
			if ( !ctx.valid || ctx.current_time >= this->m_trigger_release_time )
			{
				g::input.inject_mouse( 0, 0, input::left_up );
				this->m_trigger_held = false;
			}
		}

		const auto& ctx = g_shared.ctx( );
		if ( !ctx.valid )
		{
			return;
		}

		const auto valid_weapon = cstypes::is_weapon_valid( ctx.weapon_type );
		const auto& cfg = settings::g_combat.get( ctx.weapon_type );

		if ( !valid_weapon )
		{
			return;
		}

		const auto eye_pos = systems::g_view.origin( );
		const auto view_angles = systems::g_view.angles( );
		const auto players = systems::g_collector.players( );

		if ( !ctx.is_reloading && ctx.weapon_ready )
		{
			if ( cfg.aimbot.enabled )
			{
				const auto target = this->select_target( eye_pos, view_angles, players, cfg );
				if ( target.player )
				{
					this->aimbot( eye_pos, view_angles, target, cfg.aimbot );
				}
			}

			if ( cfg.triggerbot.enabled )
			{
				this->triggerbot( eye_pos, view_angles, players, cfg.triggerbot );
			}
		}
	}

	legit::target legit::select_target( const math::vector3& eye_pos, const math::vector3& view_angles, const std::vector<systems::collector::player>& players, const settings::combat::group_config& cfg ) const
	{
		target best{};
		best.fov = static_cast< float >( cfg.aimbot.fov );

		for ( const auto& player : players )
		{
			if ( !systems::g_local.is_enemy( player.team ) )
			{
				continue;
			}

			if ( player.invulnerable || player.hitboxes.count <= 0 )
			{
				continue;
			}

			// Check current tick
			const auto bones = systems::g_bones.get( player.bone_cache );
			if ( bones.is_valid( ) )
			{
				auto damage{ 0.0f };
				auto hitbox{ -1 };
				auto penetrated{ false };

				const auto aim_point = this->get_aim_point( eye_pos, player, bones, cfg, damage, hitbox, penetrated );
				if ( hitbox >= 0 )
				{
					const auto fov = this->get_fov( view_angles, eye_pos, aim_point );
					if ( fov < best.fov )
					{
						best.player = &player;
						best.bones = bones;
						best.aim_point = aim_point;
						best.hitbox = hitbox;
						best.damage = damage;
						best.fov = fov;
						best.penetrated = penetrated;
					}
				}
			}

		}

		return best;
	}

	math::vector3 legit::get_aim_point( const math::vector3& eye_pos, const systems::collector::player& player, const systems::bones::data& bones, const settings::combat::group_config& cfg, float& out_damage, int& out_hitbox, bool& out_penetrated ) const
	{
		out_hitbox = -1;

		for ( const auto& hb : player.hitboxes )
		{
			if ( hb.index < 0 || hb.bone < 0 )
			{
				continue;
			}

			if ( cfg.aimbot.head_only && hb.index > 1 )
			{
				continue;
			}

			const auto pos = bones.get_position( hb.bone );
			const auto hitgroup = systems::g_hitboxes.hitgroup_from_hitbox( hb.index );

			if ( !cfg.aimbot.visible_only )
			{
				out_damage = combat::g_shared.pen( ).get_max_damage( hitgroup, player.armor, player.has_helmet, player.team );
				out_hitbox = hb.index;
				out_penetrated = false;
				return pos;
			}

			const auto trace = systems::g_bvh.trace_ray( eye_pos, pos );
			const auto visible = !trace.hit || trace.fraction > 0.97f;

			if ( visible )
			{
				out_damage = combat::g_shared.pen( ).get_max_damage( hitgroup, player.armor, player.has_helmet, player.team );
				out_hitbox = hb.index;
				out_penetrated = false;
				return pos;
			}

			if ( cfg.aimbot.autowall )
			{
				shared::penetration::result pen_result{};
				if ( combat::g_shared.pen( ).run( eye_pos, pos, player, bones, pen_result ) )
				{
					if ( pen_result.damage >= cfg.aimbot.min_damage )
					{
						out_damage = pen_result.damage;
						out_hitbox = pen_result.hitbox;
						out_penetrated = pen_result.penetrated;
						return pos;
					}
				}
			}
		}

		return {};
	}

	float legit::get_fov( const math::vector3& view_angles, const math::vector3& eye_pos, const math::vector3& target_pos ) const
	{
		return math::helpers::calculate_fov( view_angles, eye_pos, target_pos );
	}

	float legit::get_fov_radius( const math::vector3& eye_pos, const math::vector3& view_angles, float fov_degrees ) const
	{
		if ( fov_degrees <= 0.0f )
		{
			return 0.0f;
		}

		math::vector3 forward{};
		view_angles.to_directions( &forward, nullptr, nullptr );

		auto offset_angles = view_angles;
		offset_angles.x -= fov_degrees;

		math::vector3 offset_forward{};
		offset_angles.to_directions( &offset_forward, nullptr, nullptr );

		const auto center = systems::g_view.project( eye_pos + forward * 1000.0f );
		const auto edge = systems::g_view.project( eye_pos + offset_forward * 1000.0f );

		if ( !systems::g_view.projection_valid( center ) || !systems::g_view.projection_valid( edge ) )
		{
			return 0.0f;
		}

		const auto dx = edge.x - center.x;
		const auto dy = edge.y - center.y;

		return std::sqrtf( dx * dx + dy * dy );
	}

	void legit::draw_fov( const math::vector3& eye_pos, const math::vector3& view_angles, const settings::combat::aimbot& cfg )
	{
		const auto target_radius = this->get_fov_radius( eye_pos, view_angles, static_cast< float >( cfg.fov ) );
		const auto alpha = this->m_fov_alpha.value( );
		const auto radius = target_radius * alpha;

		if ( radius <= 0.5f )
		{
			return;
		}

		const auto [w, h] = zdraw::get_display_size( );
		const auto color = zdraw::rgba{ cfg.fov_color.r, cfg.fov_color.g, cfg.fov_color.b, static_cast< std::uint8_t >( alpha * 125.0f ) };

		zdraw::circle( w * 0.5f, h * 0.5f, radius, color, 16 );
	}

	void legit::aimbot( const math::vector3& eye_pos, const math::vector3& view_angles, const target& tgt, const settings::combat::aimbot& cfg )
	{
		if ( !( GetAsyncKeyState( cfg.key ) & 0x8000 ) )
		{
			this->m_aim_error = {};
			return;
		}

		constexpr auto m_yaw{ 0.022f };
		const auto sensitivity = systems::g_convars.get<float>( CONVAR( "sensitivity"_hash ) );
		const auto fov_adjust = g::memory.read<float>( systems::g_local.pawn( ) + SCHEMA( "C_BasePlayerPawn", "m_flFOVSensitivityAdjust"_hash ) );
		const auto deg_per_pixel = sensitivity * m_yaw * fov_adjust;

		if ( deg_per_pixel <= 0.0f )
		{
			return;
		}

		const auto freshest = systems::g_bones.get( tgt.player->bone_cache );
		if ( !freshest.is_valid( ) )
		{
			return;
		}

		auto aim_point = freshest.get_position( tgt.player->hitboxes.entries[ tgt.hitbox ].bone );

		if ( cfg.predictive )
		{
			const auto velocity = g::memory.read<math::vector3>( tgt.player->pawn + SCHEMA( "C_BaseEntity", "m_vecAbsVelocity"_hash ) );
			const auto prediction_time = g_shared.get_prediction_time( );

			aim_point = aim_point + velocity * prediction_time;
		}

		auto desired = math::helpers::calculate_angle( eye_pos, aim_point );
		auto delta_x = desired.x - view_angles.x;
		auto delta_y = math::helpers::normalize_yaw( desired.y - view_angles.y );

		if ( cfg.smoothing > 1 )
		{
			const auto factor = static_cast< float >( cfg.smoothing );
			delta_x /= factor;
			delta_y /= factor;
		}

		const auto move_x = -delta_y / deg_per_pixel;
		const auto move_y = delta_x / deg_per_pixel;

		this->m_aim_error.x += move_x;
		this->m_aim_error.y += move_y;

		const auto dx = static_cast< int >( this->m_aim_error.x );
		const auto dy = static_cast< int >( this->m_aim_error.y );

		this->m_aim_error.x -= static_cast< float >( dx );
		this->m_aim_error.y -= static_cast< float >( dy );

		if ( dx != 0 || dy != 0 )
		{
			g::input.inject_mouse( dx, dy, input::move );
		}
	}

	legit::trigger_result legit::trace_crosshair( const math::vector3& eye_pos, const math::vector3& view_angles, const std::vector<systems::collector::player>& players, const settings::combat::triggerbot& cfg ) const
	{
		trigger_result result{};

		math::vector3 forward{};
		view_angles.to_directions( &forward, nullptr, nullptr );

		constexpr auto max_range{ 8192.0f };
		auto best_dist_sq = max_range * max_range;
		const auto prediction_time = cfg.predictive ? g_shared.get_prediction_time( ) + static_cast< float >( cfg.delay ) * 0.001f : 0.0f;

		for ( const auto& player : players )
		{
			if ( !systems::g_local.is_enemy( player.team ) )
			{
				continue;
			}

			if ( player.invulnerable )
			{
				continue;
			}

			const auto bones = systems::g_bones.get( player.bone_cache );
			if ( !bones.is_valid( ) )
			{
				continue;
			}

			math::vector3 velocity{};
			if ( cfg.predictive )
			{
				velocity = g::memory.read<math::vector3>( player.pawn + SCHEMA( "C_BaseEntity", "m_vecAbsVelocity"_hash ) );
			}

			const auto consider_hit = [ & ]( int hitbox, int hitgroup, const math::vector3& capsule_start, const math::vector3& capsule_end, float radius, bool can_penetrate )
				{
					float ray_dist = 0.0f;
					if ( !ray_hits_capsule( eye_pos, forward, capsule_start, capsule_end, radius, ray_dist ) )
					{
						return;
					}

					if ( ray_dist < 0.0f || ray_dist > max_range )
					{
						return;
					}

					const auto hit_pos = eye_pos + forward * ray_dist;
					const auto dist_sq = ( hit_pos - eye_pos ).length_sqr( );

					if ( dist_sq >= best_dist_sq )
					{
						return;
					}

					const auto vis_trace = systems::g_bvh.trace_ray( eye_pos, hit_pos );
					const auto visible = !vis_trace.hit || vis_trace.fraction > 0.97f;

					if ( visible )
					{
						const auto damage = combat::g_shared.pen( ).get_max_damage( hitgroup, player.armor, player.has_helmet, player.team );

						best_dist_sq = dist_sq;
						result.player = &player;
						result.bones = bones;
						result.hitbox = hitbox;
						result.hitgroup = hitgroup;
						result.damage = damage;
						result.penetrated = false;
						return;
					}

					if ( !cfg.autowall || !can_penetrate )
					{
						return;
					}

					shared::penetration::result pen_result{};
					if ( combat::g_shared.pen( ).run( eye_pos, hit_pos, player, bones, pen_result ) )
					{
						if ( pen_result.damage >= cfg.min_damage )
						{
							best_dist_sq = dist_sq;
							result.player = &player;
							result.bones = bones;
							result.hitbox = pen_result.hitbox;
							result.hitgroup = systems::g_hitboxes.hitgroup_from_hitbox( pen_result.hitbox );
							result.damage = pen_result.damage;
							result.penetrated = pen_result.penetrated;
						}
					}
				};

			if ( player.hitboxes.count > 0 )
			{
				for ( const auto& hb : player.hitboxes )
				{
					hitbox_capsule capsule{};
					if ( build_hitbox_capsule( hb, bones, velocity, prediction_time, capsule ) )
					{
						consider_hit( hb.index, systems::g_hitboxes.hitgroup_from_hitbox( hb.index ), capsule.start, capsule.end, capsule.radius, true );
					}
				}

				continue;
			}

			const auto head = bones.get_position( 6 ) + velocity * prediction_time;
			const auto chest = bones.get_position( 3 ) + velocity * prediction_time;
			const auto pelvis = bones.get_position( 0 ) + velocity * prediction_time;

			if ( finite( head ) )
			{
				consider_hit( 0, 1, head, head, 6.0f, false );
			}

			if ( finite( chest ) )
			{
				consider_hit( 4, 2, chest, chest, 8.0f, false );
			}

			if ( finite( pelvis ) )
			{
				consider_hit( 2, 3, pelvis, pelvis, 9.0f, false );
			}
		}

		return result;
	}

	void legit::triggerbot( const math::vector3& eye_pos, const math::vector3& view_angles, const std::vector<systems::collector::player>& players, const settings::combat::triggerbot& cfg )
	{
		if ( this->m_trigger_held )
		{
			return;
		}

		this->release_autostop( );

		if ( !( GetAsyncKeyState( cfg.key ) & 0x8000 ) )
		{
			this->m_trigger_waiting = false;
			return;
		}

		const auto& ctx = g_shared.ctx( );
		if ( !ctx.weapon_ready )
		{
			this->m_trigger_waiting = false;
			return;
		}

		const auto pawn = systems::g_local.pawn( );
		const auto velocity = pawn ? g::memory.read<math::vector3>( pawn + SCHEMA( "C_BaseEntity", "m_vecAbsVelocity"_hash ) ) : math::vector3{};
		const auto speed = velocity.length_2d( );
		const auto flags = pawn ? g::memory.read<std::uint32_t>( pawn + SCHEMA( "C_BaseEntity", "m_fFlags"_hash ) ) : 0u;
		const auto on_ground = ( flags & 1 ) != 0;
		const auto is_moving = speed > 5.0f;

		auto trace_pos = eye_pos;
		auto found_at_extrapolated{ false };

		if ( cfg.autostop && cfg.early_autostop && on_ground && is_moving )
		{
			constexpr auto lookahead_ticks{ 4 };
			constexpr auto tick_interval{ 0.015625f };
			const auto lookahead_pos = eye_pos + math::vector3{ velocity.x * tick_interval * lookahead_ticks, velocity.y * tick_interval * lookahead_ticks, 0.0f };

			trace_pos = g_shared.extrapolate_stop( lookahead_pos );

			const auto extrap_result = this->trace_crosshair( trace_pos, view_angles, players, cfg );
			if ( extrap_result.player )
			{
				found_at_extrapolated = true;
			}
		}

		const auto result = found_at_extrapolated ? this->trace_crosshair( trace_pos, view_angles, players, cfg ) : this->trace_crosshair( eye_pos, view_angles, players, cfg );
		if ( !result.player )
		{
			this->m_trigger_waiting = false;
			return;
		}

		if ( result.penetrated && result.damage < cfg.min_damage )
		{
			this->m_trigger_waiting = false;
			return;
		}

		if ( cfg.autostop && on_ground && is_moving )
		{
			this->apply_autostop( );

			if ( speed > 30.0f )
			{
				return;
			}
		}

		if ( !g_shared.is_sniper_accurate( ) )
		{
			return;
		}

		if ( cfg.hitchance > 0.0f && result.player->hitboxes.count > 0 )
		{
			const auto required = cfg.hitchance / 100.0f;
			const auto hc = g_shared.calculate_hitchance( eye_pos, view_angles, *result.player, result.bones );

			if ( hc < required )
			{
				this->m_trigger_waiting = false;
				return;
			}
		}

		const auto now = ctx.current_time;

		if ( !this->m_trigger_waiting )
		{
			this->m_trigger_waiting = true;
			this->m_trigger_delay_end = now + static_cast< float >( cfg.delay ) * 0.001f;
			return;
		}

		if ( now < this->m_trigger_delay_end )
		{
			return;
		}

		this->m_trigger_waiting = false;

		const auto hold_ms = this->m_rng.random_float( 50.0f, 120.0f );

		g::input.inject_mouse( 0, 0, input::left_down );
		this->m_trigger_held = true;
		this->m_trigger_release_time = now + hold_ms * 0.001f;
	}

	void legit::apply_autostop( )
	{
		const auto pawn = systems::g_local.pawn( );
		if ( !pawn )
		{
			return;
		}

		const auto flags = g::memory.read<std::uint32_t>( pawn + SCHEMA( "C_BaseEntity", "m_fFlags"_hash ) );
		if ( !( flags & ( 1 << 0 ) ) )
		{
			return;
		}

		const auto velocity = g::memory.read<math::vector3>( pawn + SCHEMA( "C_BaseEntity", "m_vecAbsVelocity"_hash ) );
		if ( velocity.length_2d( ) <= 15.0f )
		{
			return;
		}

		if ( this->m_autostop_active )
		{
			return;
		}

		const auto forward_pressed = ( GetAsyncKeyState( 'W' ) & 0x8000 ) != 0;
		const auto back_pressed = ( GetAsyncKeyState( 'S' ) & 0x8000 ) != 0;
		const auto left_pressed = ( GetAsyncKeyState( 'A' ) & 0x8000 ) != 0;
		const auto right_pressed = ( GetAsyncKeyState( 'D' ) & 0x8000 ) != 0;

		this->m_autostop_keys.clear( );

		if ( forward_pressed && !back_pressed )
		{
			this->m_autostop_keys.push_back( 'S' );
		}
		else if ( back_pressed && !forward_pressed )
		{
			this->m_autostop_keys.push_back( 'W' );
		}

		if ( left_pressed && !right_pressed )
		{
			this->m_autostop_keys.push_back( 'D' );
		}
		else if ( right_pressed && !left_pressed )
		{
			this->m_autostop_keys.push_back( 'A' );
		}

		if ( this->m_autostop_keys.empty( ) )
		{
			return;
		}

		for ( const auto key : this->m_autostop_keys )
		{
			g::input.inject_keyboard( key, true );
		}

		this->m_autostop_active = true;
		this->m_autostop_start = std::chrono::steady_clock::now( );
	}

	void legit::release_autostop( )
	{
		if ( !this->m_autostop_active )
		{
			return;
		}

		const auto pawn = systems::g_local.pawn( );
		const auto velocity = pawn ? g::memory.read<math::vector3>( pawn + SCHEMA( "C_BaseEntity", "m_vecAbsVelocity"_hash ) ) : math::vector3{};
		const auto elapsed = std::chrono::duration<float>( std::chrono::steady_clock::now( ) - this->m_autostop_start ).count( );

		if ( velocity.length_2d( ) > 15.0f && elapsed < 0.15f )
		{
			return;
		}

		for ( const auto key : this->m_autostop_keys )
		{
			g::input.inject_keyboard( key, false );
		}

		this->m_autostop_keys.clear( );
		this->m_autostop_active = false;
	}

} // namespace features::combat
