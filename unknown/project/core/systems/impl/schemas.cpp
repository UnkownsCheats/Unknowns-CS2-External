#include <stdafx.hpp>

#include <charconv>
#include <cctype>
#include <system_error>

namespace
{
	bool read_json_string( const std::string& json, std::size_t quote_pos, std::string& value, std::size_t* next_pos = nullptr )
	{
		if ( quote_pos >= json.size( ) || json[ quote_pos ] != '"' )
			return false;

		value.clear( );
		for ( auto pos = quote_pos + 1; pos < json.size( ); ++pos )
		{
			const auto ch = json[ pos ];
			if ( ch == '"' )
			{
				if ( next_pos )
					*next_pos = pos + 1;
				return true;
			}

			if ( ch == '\\' && pos + 1 < json.size( ) )
			{
				value += json[ ++pos ];
				continue;
			}

			value += ch;
		}

		return false;
	}

	bool find_compound_end( const std::string& json, std::size_t begin, std::size_t limit, char open, char close, std::size_t& end )
	{
		std::size_t depth = 0;
		bool in_string = false;
		bool escaped = false;

		for ( auto pos = begin; pos < limit; ++pos )
		{
			const auto ch = json[ pos ];
			if ( in_string )
			{
				if ( escaped )
				{
					escaped = false;
					continue;
				}

				if ( ch == '\\' )
				{
					escaped = true;
					continue;
				}

				if ( ch == '"' )
					in_string = false;

				continue;
			}

			if ( ch == '"' )
			{
				in_string = true;
				continue;
			}

			if ( ch == open )
			{
				++depth;
				continue;
			}

			if ( ch == close && depth > 0 && --depth == 0 )
			{
				end = pos + 1;
				return true;
			}
		}

		return false;
	}

	bool value_range( const std::string& json, std::size_t begin, std::size_t limit, std::size_t& value_begin, std::size_t& value_end )
	{
		value_begin = begin;
		while ( value_begin < limit && std::isspace( static_cast<unsigned char>( json[ value_begin ] ) ) )
			++value_begin;

		if ( value_begin >= limit )
			return false;

		if ( json[ value_begin ] == '{' )
			return find_compound_end( json, value_begin, limit, '{', '}', value_end );

		if ( json[ value_begin ] == '[' )
			return find_compound_end( json, value_begin, limit, '[', ']', value_end );

		if ( json[ value_begin ] == '"' )
		{
			std::string ignored;
			return read_json_string( json, value_begin, ignored, &value_end );
		}

		value_end = value_begin;
		while ( value_end < limit && json[ value_end ] != ',' && json[ value_end ] != '}' && json[ value_end ] != ']' )
			++value_end;

		return value_begin < value_end;
	}

	bool find_object_member( const std::string& json, std::size_t object_begin, std::size_t object_end, std::string_view key, std::size_t& value_begin, std::size_t& value_end )
	{
		if ( object_begin >= object_end || json[ object_begin ] != '{' )
			return false;

		auto pos = object_begin + 1;
		while ( pos < object_end )
		{
			while ( pos < object_end && ( std::isspace( static_cast<unsigned char>( json[ pos ] ) ) || json[ pos ] == ',' ) )
				++pos;

			if ( pos >= object_end || json[ pos ] == '}' )
				return false;

			std::string member_name;
			std::size_t after_name = 0;
			if ( !read_json_string( json, pos, member_name, &after_name ) )
				return false;

			auto colon = after_name;
			while ( colon < object_end && std::isspace( static_cast<unsigned char>( json[ colon ] ) ) )
				++colon;

			if ( colon >= object_end || json[ colon ] != ':' )
				return false;

			if ( !value_range( json, colon + 1, object_end, value_begin, value_end ) )
				return false;

			if ( member_name == key )
				return true;

			pos = value_end;
		}

		return false;
	}

	bool parse_number( const std::string& json, std::size_t begin, std::size_t end, std::int32_t& value )
	{
		while ( begin < end && std::isspace( static_cast<unsigned char>( json[ begin ] ) ) )
			++begin;

		if ( begin >= end )
			return false;

		if ( json[ begin ] == '"' )
			++begin;

		auto number_end = begin;
		while ( number_end < end && ( std::isxdigit( static_cast<unsigned char>( json[ number_end ] ) ) || json[ number_end ] == 'x' || json[ number_end ] == 'X' ) )
			++number_end;

		if ( begin == number_end )
			return false;

		std::uint32_t parsed = 0;
		const auto raw = std::string_view{ json.data( ) + begin, number_end - begin };
		if ( raw.size( ) > 2 && raw[ 0 ] == '0' && ( raw[ 1 ] == 'x' || raw[ 1 ] == 'X' ) )
		{
			const auto hex = raw.substr( 2 );
			const auto [ ptr, ec ] = std::from_chars( hex.data( ), hex.data( ) + hex.size( ), parsed, 16 );
			if ( ec != std::errc{ } || ptr != hex.data( ) + hex.size( ) )
				return false;
		}
		else
		{
			const auto [ ptr, ec ] = std::from_chars( raw.data( ), raw.data( ) + raw.size( ), parsed, 10 );
			if ( ec != std::errc{ } || ptr != raw.data( ) + raw.size( ) )
				return false;
		}

		value = static_cast<std::int32_t>( parsed );
		return true;
	}

	bool lookup_client_classes( const std::string& json, std::size_t& classes_begin, std::size_t& classes_end )
	{
		std::size_t module_begin = 0;
		std::size_t module_end = 0;

		return find_object_member( json, 0, json.size( ), "client.dll", module_begin, module_end ) &&
			find_object_member( json, module_begin, module_end, "classes", classes_begin, classes_end );
	}

	bool lookup_fields_offset( const std::string& json, std::size_t fields_begin, std::size_t fields_end, std::uint32_t field_hash, std::int32_t& offset )
	{
		auto pos = fields_begin + 1;
		while ( pos < fields_end )
		{
			while ( pos < fields_end && ( std::isspace( static_cast<unsigned char>( json[ pos ] ) ) || json[ pos ] == ',' ) )
				++pos;

			if ( pos >= fields_end || json[ pos ] == '}' )
				break;

			std::string field_name;
			std::size_t after_name = 0;
			if ( !read_json_string( json, pos, field_name, &after_name ) )
				return false;

			auto colon = after_name;
			while ( colon < fields_end && std::isspace( static_cast<unsigned char>( json[ colon ] ) ) )
				++colon;

			if ( colon >= fields_end || json[ colon ] != ':' )
				return false;

			std::size_t field_begin = 0;
			std::size_t field_end = 0;
			if ( !value_range( json, colon + 1, fields_end, field_begin, field_end ) )
				return false;

			if ( fnv1a::runtime_hash( field_name.c_str( ) ) == field_hash )
			{
				auto val_begin = field_begin;
				while ( val_begin < field_end && std::isspace( static_cast<unsigned char>( json[ val_begin ] ) ) )
					++val_begin;

				if ( val_begin < field_end && json[ val_begin ] == '{' )
				{
					std::size_t offset_begin = 0;
					std::size_t offset_end = 0;
					return find_object_member( json, field_begin, field_end, "offset", offset_begin, offset_end ) && parse_number( json, offset_begin, offset_end, offset );
				}

				return parse_number( json, field_begin, field_end, offset );
			}

			pos = field_end;
		}

		return false;
	}

	bool lookup_dump_schema_offset( const std::string& json, const char* class_name, std::uint32_t field_hash, std::int32_t& offset )
	{
		std::size_t classes_begin = 0;
		std::size_t classes_end = 0;
		std::size_t class_begin = 0;
		std::size_t class_end = 0;
		std::size_t fields_begin = 0;
		std::size_t fields_end = 0;

		if ( !lookup_client_classes( json, classes_begin, classes_end ) ||
			!find_object_member( json, classes_begin, classes_end, class_name, class_begin, class_end ) ||
			!find_object_member( json, class_begin, class_end, "fields", fields_begin, fields_end ) )
		{
			return false;
		}

		return lookup_fields_offset( json, fields_begin, fields_end, field_hash, offset );
	}

	bool lookup_dump_schema_offset_any_class( const std::string& json, std::uint32_t field_hash, std::int32_t& offset, std::string& owner_class )
	{
		std::size_t classes_begin = 0;
		std::size_t classes_end = 0;
		if ( !lookup_client_classes( json, classes_begin, classes_end ) )
		{
			return false;
		}

		auto pos = classes_begin + 1;
		while ( pos < classes_end )
		{
			while ( pos < classes_end && ( std::isspace( static_cast<unsigned char>( json[ pos ] ) ) || json[ pos ] == ',' ) )
				++pos;

			if ( pos >= classes_end || json[ pos ] == '}' )
				break;

			std::string class_name;
			std::size_t after_name = 0;
			if ( !read_json_string( json, pos, class_name, &after_name ) )
				return false;

			auto colon = after_name;
			while ( colon < classes_end && std::isspace( static_cast<unsigned char>( json[ colon ] ) ) )
				++colon;

			if ( colon >= classes_end || json[ colon ] != ':' )
				return false;

			std::size_t class_begin = 0;
			std::size_t class_end = 0;
			if ( !value_range( json, colon + 1, classes_end, class_begin, class_end ) )
				return false;

			std::size_t fields_begin = 0;
			std::size_t fields_end = 0;
			if ( find_object_member( json, class_begin, class_end, "fields", fields_begin, fields_end ) &&
				lookup_fields_offset( json, fields_begin, fields_end, field_hash, offset ) )
			{
				owner_class = std::move( class_name );
				return true;
			}

			pos = class_end;
		}

		return false;
	}
}

namespace systems {

	std::int32_t schemas::lookup( const char* class_name, std::uint32_t field_hash, const char* field_expr )
	{
		if ( const auto client_dump = g::offsets.repo_file( "client_dll.json" ) )
		{
			std::int32_t dump_offset = 0;
			if ( lookup_dump_schema_offset( *client_dump, class_name, field_hash, dump_offset ) )
			{
				g::console.print( "[schema] {}::{} = 0x{:X} (client_dll.json)", class_name, field_expr ? field_expr : "<hash>", dump_offset );
				return dump_offset;
			}

			std::string owner_class;
			if ( lookup_dump_schema_offset_any_class( *client_dump, field_hash, dump_offset, owner_class ) )
			{
				g::console.print( "[schema] {}::{} = 0x{:X} (client_dll.json via {})", class_name, field_expr ? field_expr : "<hash>", dump_offset, owner_class );
				return dump_offset;
			}

			g::console.warn( "[schema] missing {}::{} in client_dll.json, trying live schema", class_name, field_expr ? field_expr : "<hash>" );
		}
		else
		{
			g::console.warn( "[schema] client_dll.json unavailable while resolving {}::{}", class_name, field_expr ? field_expr : "<hash>" );
		}

		const auto class_info = this->find_class_binding( class_name );
		if ( !class_info )
		{
			g::console.warn( "[schema] live class binding failed for {}::{}", class_name, field_expr ? field_expr : "<hash>" );
			return 0;
		}

		const auto field_count = g::memory.read<std::int16_t>( class_info + 0x1c );
		const auto fields_ptr = g::memory.read<std::uintptr_t>( class_info + 0x28 );

		if ( field_count <= 0 || !fields_ptr )
		{
			g::console.warn( "[schema] live field table invalid for {}::{} (count={}, ptr=0x{:X})", class_name, field_expr ? field_expr : "<hash>", field_count, fields_ptr );
			return 0;
		}

		for ( std::int16_t i = 0; i < field_count; ++i )
		{
			const auto field_addr = fields_ptr + static_cast< std::size_t >( i ) * 0x20;
			const auto name_ptr = g::memory.read<std::uintptr_t>( field_addr );

			if ( !name_ptr )
			{
				continue;
			}

			char name[ 256 ]{};
			g::memory.read( name_ptr, name, sizeof( name ) );

			if ( fnv1a::runtime_hash( name ) == field_hash )
			{
				const auto live_offset = g::memory.read< std::int32_t >( field_addr + 0x10 );
				g::console.print( "[schema] {}::{} = 0x{:X} (live schema)", class_name, field_expr ? field_expr : "<hash>", live_offset );
				return live_offset;
			}
		}

		g::console.warn( "[schema] live field lookup failed for {}::{}", class_name, field_expr ? field_expr : "<hash>" );
		return 0;
	}

	std::uintptr_t schemas::find_class_binding( const char* class_name )
	{
		if ( !this->m_client_scope )
		{
			const auto schema_system = g::memory.find_vtable_instance( g::modules.schemasystem, "CSchemaSystem" );
			if ( !schema_system )
			{
				return 0;
			}

			const auto scope_count = g::memory.read<std::int32_t>( schema_system + 0x190 );
			const auto scope_data = g::memory.read<std::uintptr_t>( schema_system + 0x198 );

			if ( !scope_count || scope_count > 64 || !scope_data )
			{
				return 0;
			}

			for ( std::int32_t i = 0; i < scope_count; ++i )
			{
				const auto scope_ptr = g::memory.read<std::uintptr_t>( scope_data + i * sizeof( std::uintptr_t ) );
				if ( !scope_ptr )
				{
					continue;
				}

				char scope_name[ 32 ]{};
				g::memory.read( scope_ptr + 0x8, scope_name, sizeof( scope_name ) );

				if ( std::strcmp( scope_name, "client.dll" ) == 0 )
				{
					this->m_client_scope = scope_ptr;
					break;
				}
			}

			if ( !this->m_client_scope )
			{
				return 0;
			}
		}

		const auto name_hash = this->murmur2( class_name );
		const auto idx = this->bucket_index( name_hash );

		const auto bucket_addr = this->m_client_scope + 0x5c0 + static_cast< std::size_t >( idx ) * 24;
		const auto first = g::memory.read<std::uintptr_t>( bucket_addr );
		auto element = first;

		while ( element )
		{
			const auto key = g::memory.read<std::uint32_t>( element );
			if ( key == name_hash )
			{
				const auto data = g::memory.read<std::uintptr_t>( element + 16 );
				if ( data )
				{
					return data;
				}
			}

			element = g::memory.read<std::uintptr_t>( element + 8 );
		}

		const auto first_uncommitted = g::memory.read<std::uintptr_t>( bucket_addr + 16 );
		element = first_uncommitted;

		while ( element && element != first )
		{
			const auto key = g::memory.read<std::uint32_t>( element );
			if ( key == name_hash )
			{
				const auto data = g::memory.read<std::uintptr_t>( element + 16 );
				if ( data )
				{
					return data;
				}
			}

			element = g::memory.read<std::uintptr_t>( element + 8 );
		}

		return 0;
	}

	std::uint32_t schemas::murmur2( const char* str )
	{
		std::uint32_t len{ 0 };
		const auto data = reinterpret_cast< const std::uint8_t* >( str );

		while ( data[ len ] )
		{
			++len;
		}

		auto h = len ^ 0xBAADFEED;
		auto remaining = len;
		const auto* p = data;

		while ( remaining >= 4 )
		{
			auto k = *reinterpret_cast< const std::uint32_t* >( p );
			k *= 0x5BD1E995;
			k ^= k >> 24;
			k *= 0x5BD1E995;

			h *= 0x5BD1E995;
			h ^= k;

			p += 4;
			remaining -= 4;
		}

		switch ( remaining )
		{
		case 3: h ^= p[ 2 ] << 16; [[fallthrough]];
		case 2: h ^= p[ 1 ] << 8; [[fallthrough]];
		case 1: h ^= p[ 0 ];
			h *= 0x5BD1E995;
			break;
		}

		h ^= h >> 13;
		h *= 0x5BD1E995;
		h ^= h >> 15;

		return h;
	}

	std::uint8_t schemas::bucket_index( std::uint32_t hash )
	{
		auto v4 = ( 4097 * hash + 2127912214 ) ^ ( ( 4097 * hash + 2127912214 ) >> 19 ) ^ 0xC761C23C;
		auto v5 = ( 33 * v4 + 374761393 ) << 9;

		auto a = 9 * ( v5 ^ ( 33 * v4 - 369570787 ) ) - 42973499;
		auto b = ( a >> 16 ) ^ a ^ 0xB55A4F09;

		auto byte0 = static_cast< std::uint8_t >( ( a >> 16 ) ^ ( 9 * ( v5 ^ ( 33 * v4 + 29 ) ) - 59 ) );
		auto byte1 = static_cast< std::uint8_t >( b >> 16 );
		auto byte2 = static_cast< std::uint8_t >( ( static_cast< std::uint16_t >( ( a >> 16 ) ^ ( 9 * ( v5 ^ ( 33 * v4 - 13283 ) ) + 18117 ) ^ 0x4F09 ^ ( b >> 16 ) ) ) >> 8 );

		return byte0 ^ 9 ^ byte1 ^ byte2;
	}

} // namespace systems
