#include <stdafx.hpp>

namespace systems {

	void local::update( )
	{
		static ULONGLONG next_debug_log = 0;
		const auto now = GetTickCount64( );
		const auto should_log = now >= next_debug_log;
		if ( should_log )
			next_debug_log = now + 1000;

		const auto local_controller = g::memory.read<std::uintptr_t>( g::offsets.local_player_controller );
		if ( !local_controller )
		{
			if ( should_log )
				g::console.warn( "[local] controller is null (offset address=0x{:X})", g::offsets.local_player_controller );

			this->reset( );
			return;
		}

		const auto player_pawn_offset = SCHEMA( "CCSPlayerController", "m_hPlayerPawn"_hash );
		const auto player_pawn_handle = g::memory.read<std::uint32_t>( local_controller + player_pawn_offset );
		if ( !player_pawn_handle )
		{
			if ( should_log )
				g::console.warn( "[local] pawn handle is null (controller=0x{:X}, m_hPlayerPawn=0x{:X})", local_controller, player_pawn_offset );

			this->reset( );
			return;
		}

		auto player_pawn = systems::g_entities.lookup( player_pawn_handle );
		if ( !player_pawn )
		{
			const auto direct_pawn = g::memory.read<std::uintptr_t>( g::offsets.local_player_pawn );
			if ( direct_pawn > 0x10000 )
			{
				player_pawn = direct_pawn;
				if ( should_log )
					g::console.warn( "[local] pawn handle lookup failed (handle=0x{:X}); using dwLocalPlayerPawn=0x{:X}", player_pawn_handle, direct_pawn );
			}
		}

		if ( !player_pawn )
		{
			if ( should_log )
				g::console.warn( "[local] pawn lookup failed (handle=0x{:X}, dwLocalPlayerPawn address=0x{:X})", player_pawn_handle, g::offsets.local_player_pawn );

			this->reset( );
			return;
		}

		this->m_controller.store( local_controller );
		this->m_pawn.store( player_pawn );

		const auto team_num = g::memory.read<std::int32_t>( player_pawn + SCHEMA( "C_BaseEntity", "m_iTeamNum"_hash ) );
		this->m_team.store( team_num );

		const auto health = g::memory.read<std::int32_t>( player_pawn + SCHEMA( "C_BaseEntity", "m_iHealth"_hash ) );
		this->m_alive.store( health > 0 );

		if ( should_log )
		{
			g::console.print(
				"[local] controller=0x{:X} pawn=0x{:X} handle=0x{:X} team={} health={} alive={}",
				local_controller,
				player_pawn,
				player_pawn_handle,
				team_num,
				health,
				this->m_alive.load( )
			);
		}

		if ( this->m_alive.load( ) )
		{
			this->m_view_team.store( team_num );
			this->m_observer_pawn.store( 0 );

			{
				const auto weapon_services = g::memory.read<std::uintptr_t>( player_pawn + SCHEMA( "C_BasePlayerPawn", "m_pWeaponServices"_hash ) );
				if ( !weapon_services )
				{
					this->m_weapon.store( 0 );
					this->m_weapon_vdata.store( 0 );
					this->m_weapon_type.store( 0 );
					return;
				}

				const auto active_weapon_handle = g::memory.read<std::uint32_t>( weapon_services + SCHEMA( "CPlayer_WeaponServices", "m_hActiveWeapon"_hash ) );
				if ( !active_weapon_handle || active_weapon_handle == 0xFFFFFFFF )
				{
					this->m_weapon.store( 0 );
					this->m_weapon_vdata.store( 0 );
					this->m_weapon_type.store( 0 );
					return;
				}

				const auto active_weapon = systems::g_entities.lookup( active_weapon_handle );
				if ( !active_weapon )
				{
					this->m_weapon.store( 0 );
					this->m_weapon_vdata.store( 0 );
					this->m_weapon_type.store( 0 );
					return;
				}

				this->m_weapon.store( active_weapon );

				const auto vdata = g::memory.read<std::uintptr_t>( active_weapon + SCHEMA( "C_BaseEntity", "m_nSubclassID"_hash ) + 0x8 );
				if ( !vdata )
				{
					this->m_weapon_vdata.store( 0 );
					this->m_weapon_type.store( 0 );
					return;
				}

				this->m_weapon_vdata.store( vdata );
				this->m_weapon_type.store( g::memory.read<std::uint32_t>( vdata + SCHEMA( "CCSWeaponBaseVData", "m_WeaponType"_hash ) ) );
			}
		}
		else
		{
			this->m_weapon.store( 0 );
			this->m_weapon_vdata.store( 0 );
			this->m_weapon_type.store( 0 );

			const auto observer_pawn_handle = g::memory.read<std::uint32_t>( local_controller + SCHEMA( "CCSPlayerController", "m_hObserverPawn"_hash ) );
			if ( !observer_pawn_handle )
			{
				return;
			}

			const auto observer_pawn = systems::g_entities.lookup( observer_pawn_handle );
			if ( !observer_pawn )
			{
				return;
			}

			const auto observer_services = g::memory.read<std::uintptr_t>( observer_pawn + SCHEMA( "C_BasePlayerPawn", "m_pObserverServices"_hash ) );
			if ( !observer_services )
			{
				return;
			}

			const auto observer_target_handle = g::memory.read<std::uint32_t>( observer_services + SCHEMA( "CPlayer_ObserverServices", "m_hObserverTarget"_hash ) );
			if ( !observer_target_handle || observer_target_handle == 0xFFFFFFFF )
			{
				return;
			}

			const auto observer_target = systems::g_entities.lookup( observer_target_handle );
			if ( !observer_target )
			{
				return;
			}

			this->m_observer_pawn.store( observer_target );
			this->m_view_team.store( g::memory.read<std::int32_t>( observer_target + SCHEMA( "C_BaseEntity", "m_iTeamNum"_hash ) ) );
		}

		const auto game_type = systems::g_convars.get<std::int32_t>( CONVAR( "game_type"_hash ) );
		const auto game_mode = systems::g_convars.get<std::int32_t>( CONVAR( "game_mode"_hash ) );
		const auto is_ffa = ( game_type == 1 && game_mode == 2 ) || ( game_type == 1 && game_mode == 1 ) || ( game_type == 2 && game_mode == 0 );

		this->m_team_mode.store( !is_ffa );
	}

	void local::reset( )
	{
		this->m_controller.store( 0 );
		this->m_pawn.store( 0 );
		this->m_observer_pawn.store( 0 );
		this->m_team.store( 0 );
		this->m_view_team.store( 0 );
		this->m_alive.store( false );
		this->m_weapon.store( 0 );
		this->m_weapon_vdata.store( 0 );
		this->m_weapon_type.store( 0 );
	}

} // namespace systems
