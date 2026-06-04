#include <stdafx.hpp>

void menu::draw( )
{
	try
	{
		if ( GetAsyncKeyState( VK_HOME ) & 1 )
		{
			this->m_open = !this->m_open;
		}

		// UI open/close animation
		static float open_anim = 0.0f;
		static float target_anim = 0.0f;
		const auto dt = zdraw::get_delta_time( );
		
		target_anim = this->m_open ? 1.0f : 0.0f;
		open_anim += ( target_anim - open_anim ) * 10.0f * dt;
		
		if ( open_anim < 0.01f && !this->m_open )
		{
			return;
		}

		// Motion blur/smooth dragging animation
		static float target_x{ 200.0f };
		static float target_y{ 150.0f };
		const auto lerp_speed = 10.0f * dt;

		this->m_x += ( target_x - this->m_x ) * std::min( lerp_speed, 1.0f );
		this->m_y += ( target_y - this->m_y ) * std::min( lerp_speed, 1.0f );

		static auto x{ 200.0f };
		static auto y{ 150.0f };
		static auto w{ 680.0f };
		static auto h{ 510.0f };

		zui::begin( );

		// Draw fullscreen background effects (blur + particles) only when menu is visible
		if ( open_anim > 0.01f )
		{
			this->draw_background_effects( open_anim );
		}

		if ( zui::begin_window( "Unknownツ##main", this->m_x, this->m_y, this->m_w, this->m_h, true, 580.0f, 440.0f ) )
		{
			const auto [avail_w, avail_h] = zui::get_content_region_avail( );

			if ( zui::begin_nested_window( "##inner", avail_w, avail_h ) )
			{
				constexpr auto header_h{ 28.0f };
				constexpr auto padding{ 6.0f };

				zui::set_cursor_pos( padding, padding );
				this->draw_header( avail_w - padding * 2.0f, header_h );

				zui::set_cursor_pos( padding, padding + header_h + padding );
				this->draw_content( avail_w - padding * 2.0f, avail_h - header_h - padding * 3.0f );

				if ( const auto win = zui::detail::get_current_window( ) )
				{
					this->draw_accent_lines( win->bounds );
				}

				zui::end_nested_window( );
			}

			zui::end_window( );
		}

		zui::end( );
	}
	catch ( ... )
	{
		// Prevent crashes when changing settings
	}
}

void menu::draw_header( float width, float height )
{
	if ( !zui::begin_nested_window( "##header", width, height ) )
	{
		return;
	}

	const auto current = zui::detail::get_current_window( );
	if ( !current )
	{
		zui::end_nested_window( );
		return;
	}

	const auto& style = zui::get_style( );
	const auto dt = zdraw::get_delta_time( );
	const auto bx = current->bounds.x;
	const auto by = current->bounds.y;
	const auto bw = current->bounds.w;
	const auto bh = current->bounds.h;

	// Pulsing purple animation
	static float pulse_time = 0.0f;
	pulse_time += dt * 2.0f;
	const auto pulse = ( std::sin( pulse_time ) * 0.5f + 0.5f );
	const auto pulse_color = zdraw::rgba(
		static_cast< std::uint8_t >( 15 + pulse * 10 ),
		static_cast< std::uint8_t >( 15 + pulse * 5 ),
		static_cast< std::uint8_t >( 20 + pulse * 15 ),
		255
	);

	zdraw::rect_filled( bx, by, bw, bh, pulse_color );
	
	// Shadow effect
	zdraw::rect_filled( bx + 3.0f, by + 3.0f, bw, bh, zdraw::rgba{ 0, 0, 0, 50 } );
	zdraw::rect( bx, by, bw, bh, zdraw::rgba{ 20, 20, 20, 255 } );

	// Draw PNG icon (placeholder - actual PNG loading would require zdraw image support)
	// For now, draw a stylized U icon
	zdraw::rect_filled( bx + 8.0f, by + 4.0f, 20.0f, 20.0f, zdraw::rgba{ 180, 160, 255, 255 } );
	zdraw::rect( bx + 8.0f, by + 4.0f, 20.0f, 20.0f, zdraw::rgba{ 200, 180, 255, 255 } );
	zdraw::text( bx + 12.0f, by + 7.0f, "U", zdraw::rgba{ 20, 20, 30, 255 } );

	{
		constexpr auto title{ "Unknownツ" };
		auto [tw, th] = zdraw::measure_text( title );
		
		// Pulsing accent color
		const auto accent_pulse = zdraw::rgba(
			static_cast< std::uint8_t >( style.accent.r + pulse * 20 ),
			static_cast< std::uint8_t >( style.accent.g + pulse * 10 ),
			static_cast< std::uint8_t >( style.accent.b ),
			255
		);
		
		zdraw::text( bx + 35.0f, by + ( bh - th ) * 0.5f, title, accent_pulse );
	}

	static constexpr std::pair<const char*, tab> tabs[ ]
	{
		{ "combat",   tab::combat   },
		{ "esp",      tab::esp      },
		{ "movement", tab::movement },
		{ "misc",     tab::misc     },
		{ "configs",  tab::configs  },
	};

	constexpr auto tab_count = static_cast< int >( std::size( tabs ) );
	constexpr auto tab_spacing{ 8.0f };

	struct tab_anim { float v{ 0.0f }; };
	static std::array<tab_anim, tab_count> anims{};

	auto cursor_x = bx + bw - 10.0f;

	for ( int i = tab_count - 1; i >= 0; --i )
	{
		const auto& t = tabs[ i ];
		const auto is_sel = ( this->m_tab == t.second );
		auto [tw, th] = zdraw::measure_text( t.first );

		cursor_x -= tw;

		const auto tab_rect = zui::rect{ cursor_x, by, tw, bh };
		const auto hovered = zui::detail::mouse_hovered( tab_rect ) && !zui::detail::overlay_blocking_input( );

		if ( hovered && zui::detail::mouse_clicked( ) )
		{
			this->m_tab = t.second;
		}

		auto& anim = anims[ i ];
		anim.v += ( ( is_sel ? 1.0f : 0.0f ) - anim.v ) * std::min( 10.0f * dt, 1.0f );

		const auto text_y = by + ( bh - th ) * 0.5f;
		const auto col = is_sel ? zui::lighten( style.accent, 1.0f + 0.1f * anim.v ) : zui::lerp( zdraw::rgba{ 110, 110, 110, 255 }, style.text, hovered ? 1.0f : 0.0f );

		zdraw::text( cursor_x, text_y, t.first, col );

		cursor_x -= tab_spacing;
	}

	zui::end_nested_window( );
}

void menu::draw_background_effects( float anim_alpha )
{
	// Draw fullscreen dark overlay/blur
	if ( anim_alpha > 0.1f )
	{
		const auto blur_alpha = static_cast< std::uint8_t >( anim_alpha * 120 );
		zdraw::rect_filled( 0.0f, 0.0f, 1920.0f, 1080.0f, zdraw::rgba{ 0, 0, 0, blur_alpha } );
	}

	// Draw particles at fullscreen coordinates
	this->draw_particles( 1920.0f, 1080.0f, 0.0f, 0.0f );
}

void menu::draw_particles( float width, float height, float offset_x, float offset_y )
{
	// Early-out guard for division-by-zero crash
	if ( width <= 0.0f || height <= 0.0f )
	{
		return;
	}

	constexpr auto particle_count = 80;
	constexpr auto connection_distance = 100.0f;
	constexpr auto mouse_repel_radius = 80.0f;
	constexpr auto mouse_repel_force = 2.0f;

	static struct particle
	{
		float x{ 0.0f };
		float y{ 0.0f };
		float vx{ 0.0f };
		float vy{ 0.0f };
		float size{ 0.0f };
		float life{ 0.0f };
		zdraw::rgba color{ 0, 0, 0, 0 };
	} particles[ particle_count ];

	static bool initialized = false;
	if ( !initialized )
	{
		for ( auto& p : particles )
		{
			// Safety check for random initialization
			const int safe_width = static_cast< int >( std::max( width, 1.0f ) );
			const int safe_height = static_cast< int >( std::max( height, 1.0f ) );
			
			p.x = static_cast< float >( std::rand( ) % safe_width );
			p.y = static_cast< float >( std::rand( ) % safe_height );
			p.vx = ( std::rand( ) % 100 - 50 ) * 0.01f;
			p.vy = ( std::rand( ) % 100 - 50 ) * 0.01f;
			p.size = 2.0f + ( std::rand( ) % 30 ) * 0.1f;
			p.life = 1.0f;
			p.color = zdraw::rgba{
				static_cast< std::uint8_t >( 150 + std::rand( ) % 50 ),
				static_cast< std::uint8_t >( 130 + std::rand( ) % 50 ),
				static_cast< std::uint8_t >( 200 + std::rand( ) % 55 ),
				static_cast< std::uint8_t >( 80 + std::rand( ) % 80 )
			};
		}
		initialized = true;
	}

	const auto dt = zdraw::get_delta_time( );
	
	// Get mouse position for interaction
	const auto mouse_x = zui::detail::mouse_x( );
	const auto mouse_y = zui::detail::mouse_y( );

	for ( auto& p : particles )
	{
		// Mouse interaction - repel particles
		const auto dx = p.x + offset_x - mouse_x;
		const auto dy = p.y + offset_y - mouse_y;
		const auto dist = std::sqrt( dx * dx + dy * dy );
		
		if ( dist < mouse_repel_radius && dist > 0.0f )
		{
			const auto force = ( mouse_repel_radius - dist ) / mouse_repel_radius * mouse_repel_force;
			p.vx += ( dx / dist ) * force * dt;
			p.vy += ( dy / dist ) * force * dt;
		}

		p.x += p.vx;
		p.y += p.vy;
		p.life -= dt * 0.3f;

		// Dampen velocity
		p.vx *= 0.99f;
		p.vy *= 0.99f;

		// Respawn particles
		if ( p.life <= 0.0f || p.x < 0.0f || p.x > width || p.y < 0.0f || p.y > height )
		{
			const int safe_width = static_cast< int >( std::max( width, 1.0f ) );
			const int safe_height = static_cast< int >( std::max( height, 1.0f ) );
			
			p.x = static_cast< float >( std::rand( ) % safe_width );
			p.y = static_cast< float >( std::rand( ) % safe_height );
			p.vx = ( std::rand( ) % 100 - 50 ) * 0.01f;
			p.vy = ( std::rand( ) % 100 - 50 ) * 0.01f;
			p.size = 2.0f + ( std::rand( ) % 30 ) * 0.1f;
			p.life = 1.0f;
		}
	}

	// Draw connecting lines between nearby particles
	for ( size_t i = 0; i < particle_count; ++i )
	{
		for ( size_t j = i + 1; j < particle_count; ++j )
		{
			const auto dx = particles[ i ].x - particles[ j ].x;
			const auto dy = particles[ i ].y - particles[ j ].y;
			const auto dist = std::sqrt( dx * dx + dy * dy );
			
			if ( dist < connection_distance )
			{
				const auto alpha = static_cast< std::uint8_t >( ( 1.0f - dist / connection_distance ) * 50 );
				zdraw::line(
					offset_x + particles[ i ].x, offset_y + particles[ i ].y,
					offset_x + particles[ j ].x, offset_y + particles[ j ].y,
					zdraw::rgba{ 180, 160, 255, alpha }
				);
			}
		}
	}

	// Draw particles as circles
	for ( const auto& p : particles )
	{
		const auto alpha = static_cast< std::uint8_t >( p.color.a * p.life );
		zdraw::circle_filled( offset_x + p.x, offset_y + p.y, p.size, zdraw::rgba{ p.color.r, p.color.g, p.color.b, alpha } );
	}
}

void menu::draw_content( float width, float height )
{
	zui::push_style_var( zui::style_var::window_padding_x, 10.0f );
	zui::push_style_var( zui::style_var::window_padding_y, 10.0f );

	if ( !zui::begin_nested_window( "##content", width, height ) )
	{
		zui::pop_style_var( 2 );
		return;
	}

	if ( const auto win = zui::detail::get_current_window( ) )
	{
		this->draw_accent_lines( win->bounds );
	}

	switch ( this->m_tab )
	{
	case tab::combat:   this->draw_combat( );   break;
	case tab::esp:      this->draw_esp( );      break;
	case tab::movement: this->draw_movement( ); break;
	case tab::misc:     this->draw_misc( );     break;
	case tab::configs:  this->draw_configs( );  break;
	default: break;
	}

	zui::pop_style_var( 2 );
	zui::end_nested_window( );
}

void menu::draw_accent_lines( const zui::rect& bounds, float fade_ratio )
{
	const auto ix = bounds.x + 1.0f;
	const auto iw = bounds.w - 2.0f;
	const auto top_y = bounds.y + 1.0f;
	const auto bot_y = bounds.y + bounds.h - 2.0f;
	const auto accent = zui::get_accent_color( );
	const auto trans = zdraw::rgba{ accent.r, accent.g, accent.b, 0 };
	const auto fade_w = iw * fade_ratio;
	const auto solid_w = iw - fade_w * 2.0f;

	for ( const auto ly : { top_y, bot_y } )
	{
		zdraw::rect_filled_multi_color( ix, ly, fade_w, 1.0f, trans, accent, accent, trans );
		zdraw::rect_filled( ix + fade_w, ly, solid_w, 1.0f, accent );
		zdraw::rect_filled_multi_color( ix + fade_w + solid_w, ly, fade_w, 1.0f, accent, trans, trans, accent );
	}
}

namespace menu_impl {
	static std::string cfg_name{ "" };
}

void menu::draw_combat( )
{
	const auto [avail_w, avail_h] = zui::get_content_region_avail( );
	const auto col_w = ( avail_w - 8.0f ) * 0.5f;
	const auto& style = zui::get_style( );

	{
		const auto win = zui::detail::get_current_window( );
		if ( win )
		{
			constexpr auto group_spacing{ 12.0f };
			constexpr auto bar_h{ 22.0f };
			auto gx = win->bounds.x + style.window_padding_x;
			const auto gy = win->bounds.y + win->cursor_y;

			for ( int i = 0; i < 6; ++i )
			{
				auto [tw, th] = zdraw::measure_text( k_weapon_groups[ i ] );
				const auto gr = zui::rect{ gx, gy, tw, bar_h };
				const auto hov = zui::detail::mouse_hovered( gr ) && !zui::detail::overlay_blocking_input( );

				if ( hov && zui::detail::mouse_clicked( ) )
				{
					this->m_weapon_group = i;
				}

				const auto sel = ( this->m_weapon_group == i );
				const auto col = sel ? zui::get_accent_color( ) : zui::lerp( zdraw::rgba{ 100, 100, 100, 255 }, style.text, hov ? 1.0f : 0.0f );

				zdraw::text( gx, gy + ( bar_h - th ) * 0.5f, k_weapon_groups[ i ], col );

				if ( sel )
				{
					const auto accent = zui::get_accent_color( );
					const auto trans = zdraw::rgba{ accent.r, accent.g, accent.b, 0 };
					const auto fade = tw * 0.3f;
					zdraw::rect_filled_multi_color( gx, gy + bar_h - 2.0f, fade, 1.0f, trans, accent, accent, trans );
					zdraw::rect_filled( gx + fade, gy + bar_h - 2.0f, tw - fade * 2.0f, 1.0f, accent );
					zdraw::rect_filled_multi_color( gx + tw - fade, gy + bar_h - 2.0f, fade, 1.0f, accent, trans, trans, accent );
				}

				gx += tw + group_spacing;
			}

			win->cursor_y += bar_h + style.item_spacing_y;
			win->line_height = 0.0f;
		}
	}

	auto& cfg = settings::g_combat.groups[ this->m_weapon_group ];

	if ( zui::begin_group_box( "aimbot", col_w ) )
	{
		zui::checkbox( "enabled##ab", cfg.aimbot.enabled );
		zui::keybind( "key##ab", cfg.aimbot.key );
		zui::slider_int( "fov##ab", cfg.aimbot.fov, 1, 45 );
		zui::slider_int( "smoothing##ab", cfg.aimbot.smoothing, 0, 50 );
		zui::checkbox( "head only##ab", cfg.aimbot.head_only );
		zui::checkbox( "visible only##ab", cfg.aimbot.visible_only );

		if ( cfg.aimbot.visible_only )
		{
			zui::checkbox( "autowall##ab", cfg.aimbot.autowall );

			if ( cfg.aimbot.autowall )
			{
				zui::slider_float( "min damage##ab", cfg.aimbot.min_damage, 1.0f, 100.0f, "%.0f" );
			}
		}

		zui::checkbox( "predictive##ab", cfg.aimbot.predictive );
		zui::separator( );
		zui::checkbox( "draw fov##ab", cfg.aimbot.draw_fov );

		if ( cfg.aimbot.draw_fov )
		{
			zui::color_picker( "fov color##ab", cfg.aimbot.fov_color );
		}

		zui::end_group_box( );
	}

	zui::same_line( );

	if ( zui::begin_group_box( "triggerbot", col_w ) )
	{
		zui::checkbox( "enabled##tb", cfg.triggerbot.enabled );
		zui::keybind( "key##tb", cfg.triggerbot.key );
		zui::slider_float( "hitchance##tb", cfg.triggerbot.hitchance, 0.0f, 100.0f, "%.0f%%" );
		zui::slider_int( "delay (ms)##tb", cfg.triggerbot.delay, 0, 500 );
		zui::checkbox( "autowall##tb", cfg.triggerbot.autowall );

		if ( cfg.triggerbot.autowall )
		{
			zui::slider_float( "min damage##tb", cfg.triggerbot.min_damage, 1.0f, 100.0f, "%.0f" );
		}

		zui::checkbox( "autostop##tb", cfg.triggerbot.autostop );

		if ( cfg.triggerbot.autostop )
		{
			zui::checkbox( "early autostop##tb", cfg.triggerbot.early_autostop );
		}

		zui::checkbox( "predictive##tb", cfg.triggerbot.predictive );
		zui::end_group_box( );
	}

	if ( zui::begin_group_box( "recoil control system", col_w ) )
	{
		auto& rcs = settings::g_combat.m_rcs;
		zui::checkbox( "recoil compensation##rcs", rcs.enabled );
		if ( rcs.enabled )
		{
			zui::slider_float( "factor##rcsf", rcs.factor, 0.0f, 1.0f, "%.2f" );
		}

		zui::checkbox( "visual no recoil##rcs", rcs.visual_no_recoil );
		zui::text( "rcs works with mouse injection for reliability" );

		zui::end_group_box( );
	}
}

void menu::draw_esp( )
{
	const auto [avail_w, avail_h] = zui::get_content_region_avail( );
	const auto col_w = ( avail_w - 8.0f ) * 0.5f;

	if ( zui::begin_group_box( "global", col_w ) )
	{
		zui::checkbox( "bypass capture", settings::g_esp.bypass_capture );
		zui::end_group_box( );
	}

	if ( zui::begin_group_box( "radar", col_w ) )
	{
		auto& rd = settings::g_esp.m_radar;
		zui::checkbox( "enabled##rd", rd.enabled );
		zui::slider_float( "pos x##rd", rd.x, 0.0f, 2000.0f, "%.0f" );
		zui::slider_float( "pos y##rd", rd.y, 0.0f, 2000.0f, "%.0f" );
		zui::slider_float( "size##rd", rd.size, 50.0f, 400.0f, "%.0f" );
		zui::slider_float( "scale##rd", rd.scale, 0.1f, 5.0f, "%.1fx" );
		zui::checkbox( "show teammates##rd", rd.show_teammates );
		zui::end_group_box( );
	}

	if ( zui::begin_group_box( "spectator list", col_w ) )
	{
		auto& sl = settings::g_esp.m_spectator_list;
		zui::checkbox( "enabled##sl", sl.enabled );
		zui::slider_float( "pos x##sl", sl.x, 0.0f, 2000.0f, "%.0f" );
		zui::slider_float( "pos y##sl", sl.y, 0.0f, 2000.0f, "%.0f" );
		zui::end_group_box( );
	}

	if ( zui::begin_group_box( "bullet tracers", col_w ) )
	{
		auto& bt = settings::g_esp.m_bullet_tracers;
		zui::checkbox( "enabled##bt", bt.enabled );
		zui::checkbox( "tracers##bt", bt.tracers );
		zui::checkbox( "impacts##bt", bt.impacts );
		zui::color_picker( "tracer color##bt", bt.tracer_color );
		zui::color_picker( "impact color##bt", bt.impact_color );
		zui::slider_float( "duration##bt", bt.tracer_duration, 0.1f, 2.0f, "%.1fs" );
		zui::slider_float( "impact size##bt", bt.impact_size, 1.0f, 20.0f, "%.0f" );
		zui::end_group_box( );
	}

	if ( zui::begin_group_box( "glow", col_w ) )
	{
		auto& gl = settings::g_esp.m_glow;
		zui::checkbox( "enabled##gl", gl.enabled );
		zui::checkbox( "enemies##gl", gl.enemies );
		zui::checkbox( "teammates##gl", gl.teammates );
		zui::color_picker( "enemy color##gl", gl.enemy_color );
		zui::color_picker( "teammate color##gl", gl.teammate_color );
		zui::end_group_box( );
	}

	if ( zui::begin_group_box( "chams", col_w ) )
	{
		auto& ch = settings::g_esp.m_chams;
		zui::checkbox( "enabled##ch", ch.enabled );
		zui::checkbox( "enemies##ch", ch.enemies );
		zui::checkbox( "teammates##ch", ch.teammates );
		zui::checkbox( "through walls##ch", ch.through_walls );
		zui::color_picker( "enemy visible##ch", ch.enemy_visible );
		zui::color_picker( "enemy occluded##ch", ch.enemy_occluded );
		zui::end_group_box( );
	}

	if ( zui::begin_group_box( "hit marker", col_w ) )
	{
		auto& hm = settings::g_esp.m_hit_marker;
		zui::checkbox( "enabled##hm", hm.enabled );
		zui::checkbox( "show damage##hm", hm.show_damage );
		zui::color_picker( "color##hm", hm.color );
		zui::end_group_box( );
	}

	if ( zui::begin_group_box( "skin changer", col_w ) )
	{
		auto& sc = settings::g_esp.m_skin_changer;
		zui::checkbox( "enabled##sc", sc.enabled );
		zui::slider_int( "ak47##sc", sc.ak47, 1, 1000 );
		zui::slider_int( "m4a4##sc", sc.m4a4, 1, 1000 );
		zui::slider_int( "m4a1s##sc", sc.m4a1s, 1, 1000 );
		zui::slider_int( "awp##sc", sc.awp, 1, 1000 );
		zui::slider_int( "deagle##sc", sc.deagle, 1, 1000 );
		zui::slider_int( "glock##sc", sc.glock, 1, 1000 );
		zui::slider_int( "knife##sc", sc.knife, 1, 1000 );
		zui::slider_int( "usp##sc", sc.usp, 1, 1000 );
		zui::end_group_box( );
	}

	auto& p = settings::g_esp.m_player;


	if ( zui::begin_group_box( "box", col_w ) )
	{
		zui::checkbox( "enabled##bx", p.m_box.enabled );

		constexpr const char* box_styles[ ]{ "full", "cornered" };
		auto bs = static_cast< int >( p.m_box.style );

		if ( zui::combo( "style##bx", bs, box_styles, 2 ) )
		{
			p.m_box.style = static_cast< settings::esp::player::box::style0 >( bs );
		}

		zui::checkbox( "fill##bx", p.m_box.fill );
		zui::checkbox( "outline##bx", p.m_box.outline );

		if ( p.m_box.style == settings::esp::player::box::style0::cornered )
		{
			zui::slider_float( "corner len##bx", p.m_box.corner_length, 4.0f, 30.0f, "%.0f" );
		}

		zui::color_picker( "visible##bx", p.m_box.visible_color );
		zui::color_picker( "occluded##bx", p.m_box.occluded_color );
		zui::end_group_box( );
	}

	if ( zui::begin_group_box( "skeleton", col_w ) )
	{
		zui::checkbox( "enabled##sk", p.m_skeleton.enabled );

		constexpr const char* sk_styles[ ]{ "solid", "dotted" };
		auto sks = static_cast< int >( p.m_skeleton.occluded_style );

		if ( zui::combo( "occluded style##sk", sks, sk_styles, 2 ) )
		{
			p.m_skeleton.occluded_style = static_cast< settings::esp::player::skeleton::occluded_style0 >( sks );
		}

		zui::slider_float( "thickness##sk", p.m_skeleton.thickness, 0.5f, 4.0f, "%.1f" );

		zui::color_picker( "visible##sk", p.m_skeleton.visible_color );
		zui::color_picker( "occluded##sk", p.m_skeleton.occluded_color );
		zui::end_group_box( );
	}

	if ( zui::begin_group_box( "health bar", col_w ) )
	{
		zui::checkbox( "enabled##hb", p.m_health_bar.enabled );
		zui::checkbox( "outline##hb", p.m_health_bar.outline );
		zui::checkbox( "gradient##hb", p.m_health_bar.gradient );
		zui::checkbox( "show value##hb", p.m_health_bar.show_value );
		zui::color_picker( "full##hb", p.m_health_bar.full_color );
		zui::color_picker( "low##hb", p.m_health_bar.low_color );
		zui::end_group_box( );
	}

	if ( zui::begin_group_box( "items", col_w ) )
	{
		auto& it = settings::g_esp.m_item;
		zui::checkbox( "enabled##it", it.enabled );
		zui::slider_float( "max dist##it", it.max_distance, 5.0f, 150.0f, "%.0fm" );
		zui::checkbox( "icon##it", it.m_icon.enabled );
		zui::color_picker( "icon color##it", it.m_icon.color );
		zui::checkbox( "name##it", it.m_name.enabled );
		zui::color_picker( "name color##it", it.m_name.color );
		zui::checkbox( "ammo##it", it.m_ammo.enabled );
		zui::color_picker( "ammo color##it", it.m_ammo.color );
		zui::color_picker( "empty color##it", it.m_ammo.empty_color );
		zui::end_group_box( );
	}

	if ( zui::begin_group_box( "name / weapon", col_w ) )
	{
		zui::checkbox( "name##nm", p.m_name.enabled );
		zui::color_picker( "name color##nm", p.m_name.color );
		zui::separator( );
		zui::checkbox( "weapon##wp", p.m_weapon.enabled );

		constexpr const char* disp_types[ ]{ "text", "icon", "text + icon" };
		auto dt = static_cast< int >( p.m_weapon.display );

		if ( zui::combo( "display##wp", dt, disp_types, 3 ) )
		{
			p.m_weapon.display = static_cast< settings::esp::player::weapon::display_type >( dt );
		}

		zui::color_picker( "text color##wp", p.m_weapon.text_color );
		zui::color_picker( "icon color##wp", p.m_weapon.icon_color );
		zui::end_group_box( );
	}


	if ( zui::begin_group_box( "offscreen arrows", col_w ) )
	{
		zui::checkbox( "enabled##osa", p.m_offscreen_arrows.enabled );
		zui::slider_float( "size##osa", p.m_offscreen_arrows.size, 4.0f, 20.0f, "%.0f" );
		zui::slider_float( "radius##osa", p.m_offscreen_arrows.radius, 20.0f, 300.0f, "%.0f" );
		zui::color_picker( "color##osa", p.m_offscreen_arrows.color );
		zui::end_group_box( );
	}

	zui::same_line( );

	if ( zui::begin_group_box( "rank & chams", col_w ) )
	{
		zui::checkbox( "rank esp##rk", p.m_ranking.enabled );
		zui::color_picker( "rank color##rk", p.m_ranking.color );
		zui::separator( );
		zui::checkbox( "chams (overlay)##ch", p.m_chams.enabled );
		zui::color_picker( "chams color##ch", p.m_chams.color );
		zui::checkbox( "wireframe##ch", p.m_chams.wireframe );
		zui::separator( );
		zui::checkbox( "headshot dot##hs", p.m_headshot_dot.enabled );
		zui::color_picker( "headshot color##hs", p.m_headshot_dot.color );
		zui::separator( );
		zui::checkbox( "head direction##hd", p.m_head_direction.enabled );
		if ( p.m_head_direction.enabled )
		{
			zui::same_line( );
			zui::color_picker( "##hd_col", p.m_head_direction.color );
			zui::slider_float( "line length##hd", p.m_head_direction.length, 10.0f, 100.0f, "%.0f" );
		}
		zui::end_group_box( );
	}

	if ( zui::begin_group_box( "bomb", col_w ) )
	{
		auto& b = settings::g_esp.m_bomb;
		zui::checkbox( "enabled##bm", b.enabled );
		zui::color_picker( "icon color##bm", b.color );
		zui::color_picker( "timer color##bm", b.timer_color );
		zui::end_group_box( );
	}

	if ( zui::begin_group_box( "footstep esp", col_w ) )
	{
		auto& fs = settings::g_esp.m_player.m_footstep_esp;
		zui::checkbox( "enabled##fs", fs.enabled );
		zui::same_line( );
		zui::color_picker( "##fs_col", fs.color );

		constexpr const char* fs_styles[ ]{ "ripples", "shoe prints" };
		int fss = static_cast< int >( fs.style );

		if ( zui::combo( "style##fs", fss, fs_styles, 2 ) )
		{
			fs.style = static_cast< settings::esp::player::footstep_esp::style0 >( fss );
		}

		zui::slider_float( "duration##fs", fs.duration, 0.5f, 5.0f, "%.1fs" );
		zui::slider_float( "max radius##fs", fs.max_radius, 5.0f, 50.0f, "%.0f" );
		zui::end_group_box( );
	}

	if ( zui::begin_group_box( "sound esp", col_w ) )
	{
		auto& se = settings::g_esp.m_player.m_sound_esp;
		zui::checkbox( "enabled##se", se.enabled );

		zui::checkbox( "gunshots##se", se.gunshots );
		zui::same_line( );
		zui::color_picker( "##se_shot_col", se.shot_color );

		zui::checkbox( "landfall##se", se.landfall );
		zui::same_line( );
		zui::color_picker( "##se_land_col", se.land_color );

		zui::slider_float( "duration##se", se.duration, 0.5f, 5.0f, "%.1fs" );
		zui::slider_float( "max radius##se", se.max_radius, 5.0f, 100.0f, "%.0f" );
		zui::end_group_box( );
	}



	if ( zui::begin_group_box( "ammo bar", col_w ) )
	{
		zui::checkbox( "enabled##amb", p.m_ammo_bar.enabled );
		zui::checkbox( "outline##amb", p.m_ammo_bar.outline );
		zui::checkbox( "gradient##amb", p.m_ammo_bar.gradient );
		zui::checkbox( "show value##amb", p.m_ammo_bar.show_value );
		zui::color_picker( "full##amb", p.m_ammo_bar.full_color );
		zui::color_picker( "low##amb", p.m_ammo_bar.low_color );
		zui::end_group_box( );
	}

	if ( zui::begin_group_box( "projectiles", col_w ) )
	{
		auto& pr = settings::g_esp.m_projectile;
		zui::checkbox( "enabled##pr", pr.enabled );
		zui::checkbox( "icon##pr", pr.show_icon );
		zui::checkbox( "name##pr", pr.show_name );
		zui::checkbox( "timer bar##pr", pr.show_timer_bar );
		zui::checkbox( "inferno bounds##pr", pr.show_inferno_bounds );
		zui::separator( );
		zui::color_picker( "he##pr", pr.color_he );
		zui::color_picker( "flash##pr", pr.color_flash );
		zui::color_picker( "smoke##pr", pr.color_smoke );
		zui::color_picker( "molotov##pr", pr.color_molotov );
		zui::color_picker( "decoy##pr", pr.color_decoy );
		zui::end_group_box( );
	}

	if ( zui::begin_group_box( "item filters", col_w ) )
	{
		auto& f = settings::g_esp.m_item.m_filters;
		zui::checkbox( "rifles##f", f.rifles );
		zui::checkbox( "smgs##f", f.smgs );
		zui::checkbox( "shotguns##f", f.shotguns );
		zui::checkbox( "snipers##f", f.snipers );
		zui::checkbox( "pistols##f", f.pistols );
		zui::checkbox( "heavy##f", f.heavy );
		zui::checkbox( "grenades##f", f.grenades );
		zui::checkbox( "utility##f", f.utility );
		zui::end_group_box( );
	}

	if ( zui::begin_group_box( "china hat", col_w ) )
	{
		auto& chat = settings::g_esp.m_player.m_visuals.m_china_hat;
		zui::checkbox( "enabled##chat", chat.enabled );
		if ( chat.enabled )
		{
			zui::same_line( );
			zui::color_picker( "##chat_col", chat.color );
			zui::slider_float( "radius##chat", chat.radius, 5.0f, 30.0f, "%.1f" );
		}
		zui::end_group_box( );
	}

	if ( zui::begin_group_box( "visual utilities", col_w ) )
	{
		auto& v = settings::g_esp.m_player.m_visuals;
		
		zui::checkbox( "override flash alpha##vu", v.no_flash );
		if ( v.no_flash )
		{
			zui::slider_float( "max alpha##vuf", v.flash_alpha, 0.0f, 255.0f, "%.0f" );
		}

		zui::checkbox( "remove smoke##vu", v.no_smoke );
		
		zui::text( "simulates transparency by replacing smoke" );
		zui::end_group_box( );
	}
}

void menu::draw_misc( )
{
	const auto [avail_w, avail_h] = zui::get_content_region_avail( );
	const auto col_w = ( avail_w - 8.0f ) * 0.5f;
	auto& gr = settings::g_misc.m_grenades;
	auto& v = settings::g_esp.m_player.m_visuals;

	if ( zui::begin_group_box( "bomb visuals", col_w ) )
	{
		zui::checkbox( "override flash alpha##vu", v.no_flash );
		if ( v.no_flash )
		{
			zui::slider_float( "max alpha##vuf", v.flash_alpha, 0.0f, 255.0f, "%.0f" );
		}

		zui::checkbox( "remove smoke##vu", v.no_smoke );
		
		zui::text( "simulates transparency by replacing smoke" );
		zui::end_group_box( );
	}

	if ( zui::begin_group_box( "grenade prediction", col_w ) )
	{
		zui::checkbox( "enabled##gr", gr.enabled );
		zui::checkbox( "local only##gr", gr.local_only );
		zui::slider_float( "line thickness##gr", gr.line_thickness, 0.5f, 5.0f, "%.1f" );
		zui::checkbox( "gradient line##gr", gr.line_gradient );
		zui::color_picker( "line color##gr", gr.line_color );
		zui::separator( );
		zui::checkbox( "show bounces##gr", gr.show_bounces );
		zui::color_picker( "bounce color##gr", gr.bounce_color );
		zui::slider_float( "bounce size##gr", gr.bounce_size, 1.0f, 8.0f, "%.1f" );
		zui::separator( );
		zui::color_picker( "detonate color##gr", gr.detonate_color );
		zui::slider_float( "detonate size##gr", gr.detonate_size, 1.0f, 10.0f, "%.1f" );
		zui::slider_float( "fade duration##gr", gr.fade_duration, 0.0f, 2.0f, "%.2f" );
		zui::end_group_box( );
	}

	zui::same_line( );

	if ( zui::begin_group_box( "per type colors", col_w ) )
	{
		zui::checkbox( "enabled##ptc", gr.per_type_colors );
		if ( gr.per_type_colors )
		{
			zui::color_picker( "he##ptc", gr.color_he );
			zui::color_picker( "flash##ptc", gr.color_flash );
			zui::color_picker( "smoke##ptc", gr.color_smoke );
			zui::color_picker( "molotov##ptc", gr.color_molotov );
			zui::color_picker( "decoy##ptc", gr.color_decoy );
		}

		zui::end_group_box( );
	}

	if ( zui::begin_group_box( "smoke visualizer", col_w ) )
	{
		zui::checkbox( "enabled##smk_viz", v.smoke_visualizer );
		if ( v.smoke_visualizer )
		{
			zui::color_picker( "color##smk_col", v.smoke_color );
			
			constexpr const char* styles[ ]{ "wireframe", "solid", "gradient", "full" };
			int s = static_cast< int >( v.style );
			if ( zui::combo( "style##smk", s, styles, 4 ) )
			{
				v.style = static_cast< settings::esp::player::visual_utility::smoke_style >( s );
			}

			zui::slider_float( "radius##smk", v.smoke_radius, 50.0f, 250.0f, "%.0f" );
		}
		zui::end_group_box( );
	}

	if ( zui::begin_group_box( "crosshair", col_w ) )
	{
		auto& ch = settings::g_misc.m_crosshair;
		zui::checkbox( "enabled##ch", ch.enabled );
		zui::slider_float( "length##ch", ch.length, 1.0f, 20.0f, "%.0f" );
		zui::slider_float( "gap##ch", ch.gap, 0.0f, 20.0f, "%.0f" );
		zui::slider_float( "thickness##ch", ch.thickness, 1.0f, 5.0f, "%.0f" );
		zui::checkbox( "outline##ch", ch.outline );
		zui::color_picker( "color##ch", ch.color );
		zui::end_group_box( );
	}

	zui::same_line( );

	if ( zui::begin_group_box( "camera", col_w ) )
	{
		auto& cam = settings::g_misc.m_camera;
		zui::checkbox( "custom fov##cam", cam.custom_fov );
		if ( cam.custom_fov )
		{
			zui::slider_float( "fov##cam", cam.fov, 30.0f, 150.0f, "%.0f" );
		}
		zui::end_group_box( );
	}


	if ( zui::begin_group_box( "sound enhancement", col_w ) )
	{
		auto& se = settings::g_misc.m_sound_enhancement;
		zui::checkbox( "louder footsteps##se", se.enabled );
		zui::slider_float( "volume multiplier##se", se.volume_multiplier, 1.0f, 2.0f, "%.2fx" );
		zui::end_group_box( );
	}
}

void menu::draw_movement( )
{
	const auto [avail_w, avail_h] = zui::get_content_region_avail( );
	const auto col_w = ( avail_w - 8.0f ) * 0.5f;

	if ( zui::begin_group_box( "movement", col_w ) )
	{
		auto& mv = settings::g_misc.m_movement;
		zui::checkbox( "bunnyhop##mv", mv.bhop );
		zui::checkbox( "auto strafe##mv", mv.auto_strafe );
		zui::checkbox( "fast stop##mv", mv.fast_stop );
		zui::slider_float( "speed multiplier##mv", mv.speed_multiplier, 1.0f, 3.0f, "%.2fx" );
		
		zui::separator( );
		zui::checkbox( "third person##mv", mv.m_third_person.enabled );
		zui::keybind( "third person key##mv", mv.m_third_person.key );

		zui::separator( );
		zui::checkbox( "spinbot##mv", mv.spinbot );
		if ( mv.spinbot )
		{
			zui::slider_float( "spin speed##mv", mv.spinbot_speed, 1.0f, 100.0f, "%.0f" );
		}

		zui::end_group_box( );
	}

}

void menu::draw_configs( )
{
	const auto [avail_w, avail_h] = zui::get_content_region_avail( );
	const auto col_w = ( avail_w - 8.0f ) * 0.5f;

	if ( zui::begin_group_box( "manage", col_w ) )
	{
		zui::text_input( "name##cfg", menu_impl::cfg_name, 32, "config name..." );

		if ( zui::button( "save##cfg", zui::calc_item_width( ), 24.0f ) )
		{
			if ( !menu_impl::cfg_name.empty( ) )
				config::save( menu_impl::cfg_name );
		}

		const auto half_w = ( zui::calc_item_width( ) - zui::get_style( ).item_spacing_x ) * 0.5f;

		if ( zui::button( "refresh##cfg", half_w, 24.0f ) )
		{
			config::refresh( );
		}

		zui::same_line( );

		if ( zui::button( "open folder##cfg", half_w, 24.0f ) )
		{
			config::open_folder( );
		}

		zui::end_group_box( );
	}

	zui::same_line( );

	if ( zui::begin_group_box( "list", col_w ) )
	{
		if ( config::g_configs.empty( ) )
		{
			zui::text( "no configs found" );
		}
		else
		{
			for ( const auto& name : config::g_configs )
			{
				const auto full_w = zui::calc_item_width( );
				const auto small_btn_w = 24.0f;
				const auto set_def_w = 55.0f;
				const auto spacing = zui::get_style( ).item_spacing_x;
				const auto load_w = full_w - set_def_w - ( small_btn_w * 2 ) - ( spacing * 3 );

				const bool is_selected = ( config::g_active_config == name );
				if ( is_selected )
					zui::push_style_color( zui::style_color::text, zui::get_accent_color( ) );

				if ( zui::button( name.c_str( ), load_w, 22.0f ) )
				{
					config::load( name );
				}

				if ( is_selected )
					zui::pop_style_color( );

				zui::same_line( );

				const bool is_default = ( config::g_default_config == name );
				if ( is_default )
					zui::push_style_color( zui::style_color::button_bg, zui::alpha( zui::get_accent_color( ), 100ui8 ) );

				if ( zui::button( std::format( "def##{}", name ).c_str( ), set_def_w, 22.0f ) )
				{
					config::set_default( name );
				}

				if ( is_default )
					zui::pop_style_color( );

				zui::same_line( );

				if ( zui::button( std::format( "rn##{}", name ).c_str( ), small_btn_w, 22.0f ) )
				{
					if ( !menu_impl::cfg_name.empty( ) )
						config::rename( name, menu_impl::cfg_name );
				}

				zui::same_line( );

				if ( zui::button( std::format( "del##{}", name ).c_str( ), small_btn_w, 22.0f ) )
				{
					config::remove( name );
				}
			}
		}

		zui::end_group_box( );
	}
}
