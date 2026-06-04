#include <stdafx.hpp>

#include <iterator>
#include <numeric>

namespace
{
	constexpr std::uintptr_t entity_entry_size = 0x70;
	constexpr std::array entity_entry_stride_candidates{ std::uintptr_t{ 0x70 }, std::uintptr_t{ 0x78 }, std::uintptr_t{ 0x80 } };
	constexpr std::array entity_list_entry_offsets{ std::uintptr_t{ 0x10 }, std::uintptr_t{ 0x0 } };

	bool plausible_ptr( std::uintptr_t value )
	{
		return value > 0x10000 && value < 0x0000800000000000;
	}

	bool plausible_entity( std::uintptr_t entity )
	{
		if ( !plausible_ptr( entity ) )
			return false;

		const auto identity = g::memory.read<std::uintptr_t>( entity + 0x10 );
		return plausible_ptr( identity );
	}

	bool plausible_entity_list( std::uintptr_t entity_list )
	{
		if ( !plausible_ptr( entity_list ) )
			return false;

		const auto highest_index = g::memory.read<std::int32_t>( entity_list + g::offsets.game_entity_system_highest_entity_index );
		return highest_index > 0 && highest_index <= 16384;
	}

	std::uintptr_t resolve_entity_list_pointer( )
	{
		const auto indirect = g::memory.read<std::uintptr_t>( g::offsets.entity_list );
		if ( plausible_entity_list( indirect ) )
			return indirect;

		if ( plausible_entity_list( g::offsets.entity_list ) )
			return g::offsets.entity_list;

		return indirect;
	}

	std::uintptr_t resolve_entity_from_index( std::uintptr_t entity_list, std::int32_t index, std::uintptr_t* used_list_entry = nullptr, std::uintptr_t* used_stride = nullptr, std::uintptr_t* used_list_offset = nullptr )
	{
		const auto chunk_index = static_cast<std::uintptr_t>( index >> 9 );
		const auto slot = static_cast<std::uintptr_t>( index & 0x1ff );

		for ( const auto list_offset : entity_list_entry_offsets )
		{
			const auto list_entry = g::memory.read<std::uintptr_t>( entity_list + ( chunk_index * 8 ) + list_offset );
			if ( !plausible_ptr( list_entry ) )
				continue;

			for ( const auto stride : entity_entry_stride_candidates )
			{
				const auto entity = g::memory.read<std::uintptr_t>( list_entry + ( slot * stride ) );
				if ( !plausible_entity( entity ) )
					continue;

				if ( used_list_entry )
					*used_list_entry = list_entry;
				if ( used_stride )
					*used_stride = stride;
				if ( used_list_offset )
					*used_list_offset = list_offset;

				return entity;
			}
		}

		return 0;
	}

	std::string entity_designer_name( std::uintptr_t entity )
	{
		const auto identity = g::memory.read<std::uintptr_t>( entity + 0x10 );
		if ( !plausible_ptr( identity ) )
			return {};

		const auto designer_name_ptr = g::memory.read<std::uintptr_t>( identity + SCHEMA( "CEntityIdentity", "m_designerName"_hash ) );
		if ( !plausible_ptr( designer_name_ptr ) )
			return {};

		const auto name = g::memory.read_string( designer_name_ptr, 128 );
		if ( name.empty( ) )
			return {};

		return name;
	}

	std::uint32_t designer_name_to_schema_hash( const std::string& name )
	{
		switch ( fnv1a::runtime_hash( name.c_str( ) ) )
		{
		case "cs_player_controller"_hash: return "CCSPlayerController"_hash;
		case "weapon_ak47"_hash: return "C_AK47"_hash;
		case "weapon_m4a1"_hash: return "C_WeaponM4A1"_hash;
		case "weapon_m4a1_silencer"_hash: return "C_WeaponM4A1Silencer"_hash;
		case "weapon_awp"_hash: return "C_WeaponAWP"_hash;
		case "weapon_aug"_hash: return "C_WeaponAug"_hash;
		case "weapon_famas"_hash: return "C_WeaponFamas"_hash;
		case "weapon_galilar"_hash: return "C_WeaponGalilAR"_hash;
		case "weapon_sg556"_hash: return "C_WeaponSG556"_hash;
		case "weapon_g3sg1"_hash: return "C_WeaponG3SG1"_hash;
		case "weapon_scar20"_hash: return "C_WeaponSCAR20"_hash;
		case "weapon_ssg08"_hash: return "C_WeaponSSG08"_hash;
		case "weapon_mac10"_hash: return "C_WeaponMAC10"_hash;
		case "weapon_mp5sd"_hash: return "C_WeaponMP5SD"_hash;
		case "weapon_mp7"_hash: return "C_WeaponMP7"_hash;
		case "weapon_mp9"_hash: return "C_WeaponMP9"_hash;
		case "weapon_bizon"_hash: return "C_WeaponBizon"_hash;
		case "weapon_p90"_hash: return "C_WeaponP90"_hash;
		case "weapon_ump45"_hash: return "C_WeaponUMP45"_hash;
		case "weapon_nova"_hash: return "C_WeaponNOVA"_hash;
		case "weapon_sawedoff"_hash: return "C_WeaponSawedoff"_hash;
		case "weapon_xm1014"_hash: return "C_WeaponXM1014"_hash;
		case "weapon_mag7"_hash: return "C_WeaponMag7"_hash;
		case "weapon_m249"_hash: return "C_WeaponM249"_hash;
		case "weapon_negev"_hash: return "C_WeaponNegev"_hash;
		case "weapon_deagle"_hash: return "C_DEagle"_hash;
		case "weapon_elite"_hash: return "C_WeaponElite"_hash;
		case "weapon_fiveseven"_hash: return "C_WeaponFiveSeven"_hash;
		case "weapon_glock"_hash: return "C_WeaponGlock"_hash;
		case "weapon_hkp2000"_hash: return "C_WeaponHKP2000"_hash;
		case "weapon_usp_silencer"_hash: return "C_WeaponUSPSilencer"_hash;
		case "weapon_p250"_hash: return "C_WeaponP250"_hash;
		case "weapon_cz75a"_hash: return "C_WeaponCZ75a"_hash;
		case "weapon_tec9"_hash: return "C_WeaponTec9"_hash;
		case "weapon_revolver"_hash: return "C_WeaponRevolver"_hash;
		case "weapon_taser"_hash: return "C_WeaponTaser"_hash;
		case "weapon_knife"_hash: return "C_Knife"_hash;
		case "weapon_c4"_hash: return "C_C4"_hash;
		case "weapon_healthshot"_hash: return "C_Item_Healthshot"_hash;
		case "weapon_hegrenade"_hash: return "C_HEGrenade"_hash;
		case "weapon_flashbang"_hash: return "C_Flashbang"_hash;
		case "weapon_smokegrenade"_hash: return "C_SmokeGrenade"_hash;
		case "weapon_molotov"_hash: return "C_MolotovGrenade"_hash;
		case "weapon_incgrenade"_hash: return "C_IncendiaryGrenade"_hash;
		case "weapon_decoy"_hash: return "C_DecoyGrenade"_hash;
		case "hegrenade_projectile"_hash: return "C_HEGrenadeProjectile"_hash;
		case "flashbang_projectile"_hash: return "C_FlashbangProjectile"_hash;
		case "smokegrenade_projectile"_hash: return "C_SmokeGrenadeProjectile"_hash;
		case "molotov_projectile"_hash: return "C_MolotovProjectile"_hash;
		case "inferno"_hash: return "C_Inferno"_hash;
		case "decoy_projectile"_hash: return "C_DecoyProjectile"_hash;
		case "planted_c4"_hash: return "C_PlantedC4"_hash;
		default: return 0;
		}
	}
}

namespace systems {

	void entities::refresh( )
	{
		static ULONGLONG next_debug_log = 0;
		const auto now = GetTickCount64( );
		const auto should_log = now >= next_debug_log;
		if ( should_log )
			next_debug_log = now + 1000;

		const auto entity_list = this->get_entity_list( );
		if ( !entity_list )
		{
			if ( should_log )
				g::console.warn( "[entities] entity list pointer is null (offset address=0x{:X})", g::offsets.entity_list );

			std::unique_lock lock( this->m_mutex );
			this->m_entities.clear( );
			return;
		}

		std::vector<cached> fresh{};
		fresh.reserve( 128 );

		const auto reported_highest_index = g::memory.read<std::int32_t>( entity_list + g::offsets.game_entity_system_highest_entity_index );
		auto scan_highest_index = reported_highest_index;
		if ( scan_highest_index <= 0 || scan_highest_index > 16384 )
			scan_highest_index = 2048;
		else
			scan_highest_index = std::max( scan_highest_index, 2048 );

		std::size_t entity_ptrs = 0;
		std::size_t hashed = 0;
		std::size_t players = 0;
		std::size_t items = 0;
		std::size_t projectiles = 0;
		std::size_t bombs = 0;
		std::size_t unknown = 0;
		std::vector<std::string> no_hash_samples;
		std::vector<std::string> unknown_samples;

		for ( std::int32_t i = 0; i <= scan_highest_index; ++i )
		{
			const auto entity = this->get_by_index( entity_list, i );
			if ( !entity )
			{
				continue;
			}
			++entity_ptrs;

			const auto schema_hash = this->get_schema_hash( entity );
			if ( !schema_hash )
			{
				if ( should_log && no_hash_samples.size( ) < 6 )
				{
					try
					{
						auto name = entity_designer_name( entity );
						if ( name.empty( ) )
							name = "<no designer>";

						no_hash_samples.push_back( std::format( "#{}:{}", i, name ) );
					}
					catch ( ... )
					{
						no_hash_samples.push_back( std::format( "#{}:<exception>", i ) );
					}
				}
				continue;
			}
			++hashed;

			const auto entity_type = this->classify( schema_hash );
			if ( entity_type == type::unknown )
			{
				++unknown;
				if ( should_log && unknown_samples.size( ) < 6 )
				{
					try
					{
						auto name = entity_designer_name( entity );
						if ( name.empty( ) )
							name = std::format( "hash=0x{:X}", schema_hash );

						unknown_samples.push_back( std::format( "#{}:{}", i, name ) );
					}
					catch ( ... )
					{
						unknown_samples.push_back( std::format( "#{}:<exception hash=0x{:X}>", i, schema_hash ) );
					}
				}
				continue;
			}

			switch ( entity_type )
			{
			case type::player: ++players; break;
			case type::item: ++items; break;
			case type::projectile: ++projectiles; break;
			case type::bomb: ++bombs; break;
			default: break;
			}

			fresh.push_back( { .ptr = entity, .schema_hash = schema_hash, .index = static_cast< std::int16_t >( i ), .type = entity_type } );
		}

		if ( should_log )
		{
			g::console.print(
				"[entities] list=0x{:X} highest={} ptrs={} hashed={} classified={} players={} items={} projectiles={} bombs={} unknown={}",
				entity_list,
				reported_highest_index,
				entity_ptrs,
				hashed,
				fresh.size( ),
				players,
				items,
				projectiles,
				bombs,
				unknown
			);

			if ( reported_highest_index != scan_highest_index )
				g::console.warn( "[entities] highestEntityIndex reported {}, scanning through {} for safety", reported_highest_index, scan_highest_index );

			if ( !no_hash_samples.empty( ) )
			{
				g::console.warn( "[entities] no-hash samples: {}", std::accumulate( std::next( no_hash_samples.begin( ) ), no_hash_samples.end( ), no_hash_samples.front( ), []( const std::string& a, const std::string& b ) { return a + ", " + b; } ) );
			}

			if ( !unknown_samples.empty( ) )
			{
				g::console.warn( "[entities] unknown samples: {}", std::accumulate( std::next( unknown_samples.begin( ) ), unknown_samples.end( ), unknown_samples.front( ), []( const std::string& a, const std::string& b ) { return a + ", " + b; } ) );
			}
		}

		std::unique_lock lock( this->m_mutex );
		this->m_entities = std::move( fresh );
	}

	std::uintptr_t entities::lookup( std::uint32_t handle ) const
	{
		static ULONGLONG next_lookup_debug_log = 0;

		if ( !handle || handle == 0xffffffff )
		{
			return 0;
		}

		const auto entity_list = this->get_entity_list( );
		if ( !entity_list )
		{
			return 0;
		}

		const auto index = static_cast<std::int32_t>( handle & 0x7fff );
		std::uintptr_t list_entry = 0;
		std::uintptr_t stride = 0;
		std::uintptr_t list_offset = 0;
		const auto entity = resolve_entity_from_index( entity_list, index, &list_entry, &stride, &list_offset );

		if ( entity )
		{
			const auto now = GetTickCount64( );
			if ( stride != entity_entry_size && now >= next_lookup_debug_log )
			{
				next_lookup_debug_log = now + 1000;
				g::console.warn( "[entities::lookup] handle=0x{:X} resolved with fallback stride=0x{:X} list_offset=0x{:X}", handle, stride, list_offset );
			}

			return entity;
		}

		const auto now = GetTickCount64( );
		if ( now >= next_lookup_debug_log )
		{
			next_lookup_debug_log = now + 1000;

			const auto chunk = static_cast<std::uintptr_t>( index >> 9 );
			const auto slot = static_cast<std::uintptr_t>( index & 0x1ff );
			const auto entry_10 = g::memory.read<std::uintptr_t>( entity_list + ( chunk * 8 ) + 0x10 );
			const auto entry_0 = g::memory.read<std::uintptr_t>( entity_list + ( chunk * 8 ) );
			const auto probe_78 = plausible_ptr( entry_10 ) ? g::memory.read<std::uintptr_t>( entry_10 + ( slot * 0x78 ) ) : 0;
			const auto probe_70 = plausible_ptr( entry_10 ) ? g::memory.read<std::uintptr_t>( entry_10 + ( slot * 0x70 ) ) : 0;

			g::console.warn(
				"[entities::lookup] failed handle=0x{:X} index={} chunk={} slot={} list=0x{:X} entry+10=0x{:X} entry+0=0x{:X} probe78=0x{:X} probe70=0x{:X}",
				handle,
				index,
				chunk,
				slot,
				entity_list,
				entry_10,
				entry_0,
				probe_78,
				probe_70
			);
		}

		return 0;
	}

	std::vector<entities::cached> entities::by_type( type filter ) const
	{
		std::shared_lock lock( this->m_mutex );

		std::vector<cached> result{};
		result.reserve( this->m_entities.size( ) );

		for ( const auto& entry : this->m_entities )
		{
			if ( entry.type == filter )
			{
				result.push_back( entry );
			}
		}

		return result;
	}

	std::vector<entities::cached> entities::all( ) const
	{
		std::shared_lock lock( this->m_mutex );
		return this->m_entities;
	}

	std::uintptr_t entities::get_entity_list( ) const
	{
		return resolve_entity_list_pointer( );
	}

	std::uintptr_t entities::get_by_index( std::uintptr_t entity_list, std::int32_t index ) const
	{
		return resolve_entity_from_index( entity_list, index );
	}

	std::uint32_t entities::get_schema_hash( std::uintptr_t entity ) const
	{
		const auto entity_identity = g::memory.read<std::uintptr_t>( entity + 0x10 );
		if ( !entity_identity )
		{
			return 0;
		}

		const auto designer_fallback = [ & ]( ) -> std::uint32_t
		{
			const auto designer_name_ptr = g::memory.read<std::uintptr_t>( entity_identity + SCHEMA( "CEntityIdentity", "m_designerName"_hash ) );
			if ( !designer_name_ptr )
				return 0;

			return designer_name_to_schema_hash( g::memory.read_string( designer_name_ptr, 128 ) );
		};

		const auto entity_class_info = g::memory.read<std::uintptr_t>( entity_identity + 0x8 );
		if ( !entity_class_info )
		{
			return designer_fallback( );
		}

		const auto schema_name_ptr = g::memory.read<std::uintptr_t>( entity_class_info + 0x8 );
		if ( !schema_name_ptr )
		{
			return designer_fallback( );
		}

		const auto schema_name = g::memory.read<std::uintptr_t>( schema_name_ptr );
		if ( !schema_name )
		{
			return designer_fallback( );
		}

		char class_name[ 64 ]{};
		g::memory.read( schema_name, class_name, sizeof( class_name ) );

		if ( !class_name[ 0 ] )
		{
			return designer_fallback( );
		}

		return fnv1a::runtime_hash( class_name );
	}

	entities::type entities::classify( std::uint32_t schema_hash ) const
	{
		switch ( schema_hash )
		{
		case "CCSPlayerController"_hash:
			return type::player;

		case "C_AK47"_hash:
		case "C_WeaponM4A1"_hash:
		case "C_WeaponM4A1Silencer"_hash:
		case "C_WeaponAWP"_hash:
		case "C_WeaponAug"_hash:
		case "C_WeaponFamas"_hash:
		case "C_WeaponGalilAR"_hash:
		case "C_WeaponSG556"_hash:
		case "C_WeaponG3SG1"_hash:
		case "C_WeaponSCAR20"_hash:
		case "C_WeaponSSG08"_hash:
		case "C_WeaponMAC10"_hash:
		case "C_WeaponMP5SD"_hash:
		case "C_WeaponMP7"_hash:
		case "C_WeaponMP9"_hash:
		case "C_WeaponBizon"_hash:
		case "C_WeaponP90"_hash:
		case "C_WeaponUMP45"_hash:
		case "C_WeaponNOVA"_hash:
		case "C_WeaponSawedoff"_hash:
		case "C_WeaponXM1014"_hash:
		case "C_WeaponMag7"_hash:
		case "C_WeaponM249"_hash:
		case "C_WeaponNegev"_hash:
		case "C_DEagle"_hash:
		case "C_WeaponElite"_hash:
		case "C_WeaponFiveSeven"_hash:
		case "C_WeaponGlock"_hash:
		case "C_WeaponHKP2000"_hash:
		case "C_WeaponUSPSilencer"_hash:
		case "C_WeaponP250"_hash:
		case "C_WeaponCZ75a"_hash:
		case "C_WeaponTec9"_hash:
		case "C_WeaponRevolver"_hash:
		case "C_WeaponTaser"_hash:
		case "C_Knife"_hash:
		case "C_C4"_hash:
		case "C_Item_Healthshot"_hash:
		case "C_HEGrenade"_hash:
		case "C_Flashbang"_hash:
		case "C_SmokeGrenade"_hash:
		case "C_MolotovGrenade"_hash:
		case "C_IncendiaryGrenade"_hash:
		case "C_DecoyGrenade"_hash:
			return type::item;

		case "C_HEGrenadeProjectile"_hash:
		case "C_FlashbangProjectile"_hash:
		case "C_SmokeGrenadeProjectile"_hash:
		case "C_MolotovProjectile"_hash:
		case "C_Inferno"_hash:
		case "C_DecoyProjectile"_hash:
			return type::projectile;
		case "C_PlantedC4"_hash:
			return type::bomb;


		default:
			return type::unknown;
		}
	}

} // namespace systems
