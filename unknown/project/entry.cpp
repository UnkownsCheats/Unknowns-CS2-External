#include <stdafx.hpp>

int main( )
{
	{
		if ( !g::console.initialize( " :> " ) )
		{
			return 1;
		}

		g::init( );

		if ( !g::input.initialize( ) )
		{
			return 1;
		}

		if ( !g::memory.initialize( L"cs2.exe" ) )
		{
			return 1;
		}
	}

	{
		if ( !g::modules.initialize( ) )
		{
			return 1;
		}

		if ( !g::offsets.initialize( ) )
		{
			return 1;
		}
	}

	{
		std::thread( threads::game ).detach( );
		std::thread( threads::combat ).detach( );

		if ( !g::render.initialize( ) )
		{
			return 1;
		}
	}

	return 0;
}