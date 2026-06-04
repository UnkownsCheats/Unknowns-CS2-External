#pragma once
#include <string>
#include <vector>
#include <filesystem>

namespace config {

	void save( const std::string& name );
	void load( const std::string& name );
	void refresh( );

	void rename( const std::string& old_name, const std::string& new_name );
	void remove( const std::string& name );
	void open_folder( );

	void set_default( const std::string& name );
	void load_default( );

	inline std::vector<std::string> g_configs{};
	inline std::string g_active_config{ "" };
	inline std::string g_default_config{ "" };

} // namespace config
