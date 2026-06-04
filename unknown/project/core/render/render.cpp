#include <stdafx.hpp>

bool render::initialize( )
{
	g::console.print( "render: starting initialization..." );

	if ( !this->register_window_class( ) )
	{
		g::console.print( "render: failed to register window class" );
		return false;
	}

	const auto screen_w = ::GetSystemMetrics( SM_CXSCREEN );
	const auto screen_h = ::GetSystemMetrics( SM_CYSCREEN );

	this->m_hwnd = ::CreateWindowExW( WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_NOACTIVATE, k_class_name, k_class_name, WS_POPUP, 0, 0, screen_w, screen_h, nullptr, nullptr, ::GetModuleHandleW( nullptr ), nullptr );
	if ( !this->m_hwnd )
	{
		g::console.print( "render: failed to create window" );
		return false;
	}

	constexpr MARGINS margins{ -1, -1, -1, -1 };
	::DwmExtendFrameIntoClientArea( this->m_hwnd, &margins );
	::SetLayeredWindowAttributes( this->m_hwnd, 0, 255, LWA_ALPHA );
	::ShowWindow( this->m_hwnd, SW_SHOW );
	::UpdateWindow( this->m_hwnd );

	if ( !this->setup_d3d( ) )
	{
		g::console.print( "render: failed to setup D3D11" );
		return false;
	}

	g::console.print( "render initialized." );

	this->run( );

	return true;
}

bool render::register_window_class( )
{
	WNDCLASSEXW wc{};
	if ( ::GetClassInfoExW( ::GetModuleHandleW( nullptr ), k_class_name, &wc ) )
	{
		return true;
	}

	wc.cbSize = sizeof( WNDCLASSEXW );
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = wnd_proc;
	wc.hInstance = ::GetModuleHandleW( nullptr );
	wc.hbrBackground = static_cast< HBRUSH >( ::GetStockObject( BLACK_BRUSH ) );
	wc.hCursor = ::LoadCursorW( nullptr, IDC_ARROW );
	wc.lpszClassName = k_class_name;

	this->m_atom = ::RegisterClassExW( &wc );
	return this->m_atom != 0;
}

void render::run( )
{
	constexpr float clear[ 4 ]{ 0.0f, 0.0f, 0.0f, 0.0f };
	MSG msg{};

	while ( true )
	{
		while ( ::PeekMessageW( &msg, nullptr, 0, 0, PM_REMOVE ) )
		{
			if ( msg.message == WM_QUIT )
			{
				return;
			}

			::TranslateMessage( &msg );
			::DispatchMessageW( &msg );
		}

		this->update_input_window( );

		{
			static bool last_bypass{ false };
			if ( settings::g_esp.bypass_capture != last_bypass )
			{
				::SetWindowDisplayAffinity( this->m_hwnd, settings::g_esp.bypass_capture ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE );
				last_bypass = settings::g_esp.bypass_capture;
			}
		}

		this->m_context->OMSetRenderTargets( 1, &this->m_rtv, nullptr );

		this->m_context->ClearRenderTargetView( this->m_rtv, clear );

		zdraw::begin_frame( );
		{
			if ( systems::g_local.valid( ) )
			{
				systems::g_view.update( );
				features::esp::g_player.on_render( );
				features::esp::g_item.on_render( );
				features::esp::g_projectile.on_render( );
				features::esp::g_bomb.on_render( );
				features::esp::g_radar.on_render( );
				features::misc::g_grenades.on_render( );

				features::misc::g_crosshair.on_render( );
				features::misc::g_visuals.on_render( );
				features::combat::g_legit.on_render( );

			}

			g::menu.draw( );
		}
		zdraw::end_frame( );

		if ( FAILED( this->m_swap_chain->Present( 0, 0 ) ) )
		{
			break;
		}
	}
}

void render::update_input_window( )
{
	static bool last_menu_state{ false };
	const auto current_menu_state = g::menu.is_open( );

	if ( current_menu_state == last_menu_state )
		return;

	last_menu_state = current_menu_state;
	const auto style = ::GetWindowLongW( this->m_hwnd, GWL_EXSTYLE );

	if ( current_menu_state )
	{
		::SetWindowLongW( this->m_hwnd, GWL_EXSTYLE, style & ~( WS_EX_TRANSPARENT | WS_EX_NOACTIVATE ) );
	}
	else
	{
		::SetWindowLongW( this->m_hwnd, GWL_EXSTYLE, style | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE );
	}

	::SetWindowPos( this->m_hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED );
}

bool render::setup_d3d( )
{
	g::console.print( "render: setting up D3D11..." );

	DXGI_SWAP_CHAIN_DESC desc{};
	desc.BufferCount = 2;
	desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.OutputWindow = this->m_hwnd;
	desc.SampleDesc.Count = 1;
	desc.Windowed = TRUE;
	desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	D3D_FEATURE_LEVEL levels[ ]{ D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0 };
	D3D_FEATURE_LEVEL selected{};

	if ( FAILED( D3D11CreateDeviceAndSwapChain( nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_PREVENT_INTERNAL_THREADING_OPTIMIZATIONS, levels, 2, D3D11_SDK_VERSION, &desc, &this->m_swap_chain, &this->m_device, &selected, &this->m_context ) ) )
	{
		g::console.print( "render: failed to create D3D11 device and swap chain" );
		return false;
	}

	ID3D11Texture2D* back_buffer{};
	if ( FAILED( this->m_swap_chain->GetBuffer( 0, IID_PPV_ARGS( &back_buffer ) ) ) )
	{
		g::console.print( "render: failed to get swap chain back buffer" );
		return false;
	}

	const auto hr = this->m_device->CreateRenderTargetView( back_buffer, nullptr, &this->m_rtv );

	D3D11_TEXTURE2D_DESC bb_desc{};
	back_buffer->GetDesc( &bb_desc );
	back_buffer->Release( );

	if ( FAILED( hr ) )
	{
		g::console.print( "render: failed to create render target view" );
		return false;
	}

	D3D11_VIEWPORT vp{};
	vp.Width = static_cast< float >( bb_desc.Width );
	vp.Height = static_cast< float >( bb_desc.Height );
	vp.MaxDepth = 1.0f;
	this->m_context->RSSetViewports( 1, &vp );

	if ( !zdraw::initialize( this->m_device, this->m_context ) )
	{
		g::console.print( "render: failed to initialize zdraw" );
		return false;
	}

	g::console.print( "render: zdraw initialized, initializing zui..." );
	zui::initialize( this->m_hwnd );
	g::console.print( "render: zui initialized" );

	{
		this->m_fonts.mochi_12 = zdraw::add_font_from_memory( std::span( reinterpret_cast< const std::byte* >( resources::fonts::mochi ), sizeof( resources::fonts::mochi ) ), 12.0f, 512, 512 );
		this->m_fonts.mochi_24 = zdraw::add_font_from_memory( std::span( reinterpret_cast< const std::byte* >( resources::fonts::mochi ), sizeof( resources::fonts::mochi ) ), 24.0f, 512, 512 );
		this->m_fonts.pretzel_12 = zdraw::add_font_from_memory( std::span( reinterpret_cast< const std::byte* >( resources::fonts::pretzel ), sizeof( resources::fonts::pretzel ) ), 12.0f, 512, 512 );
		this->m_fonts.pixel7_10 = zdraw::add_font_from_memory( std::span( reinterpret_cast< const std::byte* >( resources::fonts::pixel7 ), sizeof( resources::fonts::pixel7 ) ), 10.0f, 512, 512 );
		this->m_fonts.weapons_15 = zdraw::add_font_from_memory( std::span( reinterpret_cast< const std::byte* >( resources::fonts::weapons ), sizeof( resources::fonts::weapons ) ), 16.0f, 512, 512 );
	}

	return true;
}

LRESULT CALLBACK render::wnd_proc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
	zui::process_wndproc_message( msg, wp, lp );

	if ( msg == WM_DESTROY )
	{
		::PostQuitMessage( 0 );
		return 0;
	}

	return ::DefWindowProcW( hwnd, msg, wp, lp );
}
