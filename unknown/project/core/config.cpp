#include <stdafx.hpp>
#include <core/config.hpp>
#include <fstream>
#include <shlobj.h>
#include <shellapi.h>

namespace config {

	std::filesystem::path get_config_path( )
	{
		char path[ MAX_PATH ];
		if ( SUCCEEDED( SHGetFolderPathA( NULL, CSIDL_PERSONAL, NULL, 0, path ) ) )
		{
			std::filesystem::path config_dir = std::filesystem::path( path ) / "unknown" / "configs";
			if ( !std::filesystem::exists( config_dir ) )
				std::filesystem::create_directories( config_dir );
			return config_dir;
		}
		return std::filesystem::current_path( );
	}

	void save( const std::string& name )
	{
		try
		{
			std::filesystem::path path = get_config_path( ) / ( name + ".cat" );
			std::ofstream file( path, std::ios::binary );
			if ( !file.is_open( ) )
				return;

			// Write a simple header with sizes for basic versioning
			const std::uint32_t sz_combat = sizeof( settings::combat );
			const std::uint32_t sz_esp = sizeof( settings::esp );
			const std::uint32_t sz_misc = sizeof( settings::misc );
			
			file.write( reinterpret_cast< const char* >( &sz_combat ), sizeof( sz_combat ) );
			file.write( reinterpret_cast< const char* >( &sz_esp ), sizeof( sz_esp ) );
			file.write( reinterpret_cast< const char* >( &sz_misc ), sizeof( sz_misc ) );

			file.write( reinterpret_cast< const char* >( &settings::g_combat ), sz_combat );
			file.write( reinterpret_cast< const char* >( &settings::g_esp ), sz_esp );
			file.write( reinterpret_cast< const char* >( &settings::g_misc ), sz_misc );
			
			file.close( );
			g_active_config = name;
			refresh( );
		}
		catch ( const std::exception& e )
		{
			// Log error but don't crash
			g::console.warn( "[config] failed to save config '{}': {}", name, e.what( ) );
		}
		catch ( ... )
		{
			g::console.warn( "[config] failed to save config '{}': unknown error", name );
		}
	}

	void load( const std::string& name )
	{
		try
		{
			std::filesystem::path path = get_config_path( ) / ( name + ".cat" );
			std::ifstream file( path, std::ios::binary );
			if ( !file.is_open( ) )
				return;

			std::uint32_t rs_combat = 0;
			std::uint32_t rs_esp = 0;
			std::uint32_t rs_misc = 0;

			// Try read header
			if ( !file.read( reinterpret_cast< char* >( &rs_combat ), sizeof( rs_combat ) ) ||
				 !file.read( reinterpret_cast< char* >( &rs_esp ), sizeof( rs_esp ) ) ||
				 !file.read( reinterpret_cast< char* >( &rs_misc ), sizeof( rs_misc ) ) )
			{
				// Old config format without header, fallback to old read method
				file.clear(); // Clear EOF flags
				file.seekg( 0, std::ios::beg );
				file.read( reinterpret_cast< char* >( &settings::g_combat ), sizeof( settings::combat ) );
				file.read( reinterpret_cast< char* >( &settings::g_esp ), sizeof( settings::esp ) );
				file.read( reinterpret_cast< char* >( &settings::g_misc ), sizeof( settings::misc ) );
			}
			else
			{
				// Read up to current size, bounded by saved size to prevent overflow
				const auto min_combat = std::min( rs_combat, static_cast< std::uint32_t >( sizeof( settings::combat ) ) );
				file.read( reinterpret_cast< char* >( &settings::g_combat ), min_combat );
				if ( rs_combat > min_combat ) file.seekg( rs_combat - min_combat, std::ios::cur );

				const auto min_esp = std::min( rs_esp, static_cast< std::uint32_t >( sizeof( settings::esp ) ) );
				file.read( reinterpret_cast< char* >( &settings::g_esp ), min_esp );
				if ( rs_esp > min_esp ) file.seekg( rs_esp - min_esp, std::ios::cur );

				const auto min_misc = std::min( rs_misc, static_cast< std::uint32_t >( sizeof( settings::misc ) ) );
				file.read( reinterpret_cast< char* >( &settings::g_misc ), min_misc );
				if ( rs_misc > min_misc ) file.seekg( rs_misc - min_misc, std::ios::cur );
			}

			file.close( );
			g_active_config = name;

			notifications::g_notifications.add( std::format( "loaded config: {}", name ) );
		}
		catch ( const std::exception& e )
		{
			g::console.warn( "[config] failed to load config '{}': {}", name, e.what( ) );
		}
		catch ( ... )
		{
			g::console.warn( "[config] failed to load config '{}': unknown error", name );
		}
	}

	void refresh( )
	{
		try
		{
			g_configs.clear( );
			std::filesystem::path path = get_config_path( );
			for ( const auto& entry : std::filesystem::directory_iterator( path ) )
			{
				if ( entry.path( ).extension( ) == ".cat" )
				{
					g_configs.push_back( entry.path( ).stem( ).string( ) );
				}
			}

			std::filesystem::path default_path = path / "default.cfg";
			if ( std::filesystem::exists( default_path ) )
			{
				std::ifstream file( default_path );
				if ( file.is_open( ) )
				{
					file >> g_default_config;
				}
			}
			else
			{
				g_default_config = "";
			}
		}
		catch ( const std::exception& e )
		{
			g::console.warn( "[config] failed to refresh configs: {}", e.what( ) );
		}
		catch ( ... )
		{
			g::console.warn( "[config] failed to refresh configs: unknown error" );
		}
	}

	void rename( const std::string& old_name, const std::string& new_name )
	{
		if ( old_name.empty( ) || new_name.empty( ) )
			return;

		try
		{
			std::filesystem::path old_path = get_config_path( ) / ( old_name + ".cat" );
			std::filesystem::path new_path = get_config_path( ) / ( new_name + ".cat" );

			if ( std::filesystem::exists( old_path ) )
			{
				std::filesystem::rename( old_path, new_path );
				if ( g_active_config == old_name )
					g_active_config = new_name;
				
				if ( g_default_config == old_name )
					set_default( new_name );

				refresh( );
			}
		}
		catch ( const std::exception& e )
		{
			g::console.warn( "[config] failed to rename config: {}", e.what( ) );
		}
		catch ( ... )
		{
			g::console.warn( "[config] failed to rename config: unknown error" );
		}
	}

	void remove( const std::string& name )
	{
		try
		{
			std::filesystem::path path = get_config_path( ) / ( name + ".cat" );
			if ( std::filesystem::exists( path ) )
			{
				std::filesystem::remove( path );
				if ( g_active_config == name )
					g_active_config = "";
				
				if ( g_default_config == name )
				{
					std::filesystem::remove( get_config_path( ) / "default.cfg" );
					g_default_config = "";
				}

				refresh( );
			}
		}
		catch ( const std::exception& e )
		{
			g::console.warn( "[config] failed to remove config: {}", e.what( ) );
		}
		catch ( ... )
		{
			g::console.warn( "[config] failed to remove config: unknown error" );
		}
	}

	void open_folder( )
	{
		ShellExecuteA( NULL, "open", get_config_path( ).string( ).c_str( ), NULL, NULL, SW_SHOWNORMAL );
	}

	void set_default( const std::string& name )
	{
		try
		{
			std::filesystem::path path = get_config_path( ) / "default.cfg";
			std::ofstream file( path );
			if ( !file.is_open( ) )
				return;

			file << name;
			file.close( );
			g_default_config = name;
		}
		catch ( const std::exception& e )
		{
			g::console.warn( "[config] failed to set default config: {}", e.what( ) );
		}
		catch ( ... )
		{
			g::console.warn( "[config] failed to set default config: unknown error" );
		}
	}

	void load_default( )
	{
		std::filesystem::path path = get_config_path( ) / "default.cfg";
		std::ifstream file( path );
		if ( !file.is_open( ) )
			return;

		std::string name;
		if ( file >> name )
		{
			load( name );
		}
		file.close( );
	}

} // namespace config
