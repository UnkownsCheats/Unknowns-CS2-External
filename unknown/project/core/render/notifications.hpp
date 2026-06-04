#pragma once
#include <string>
#include <vector>
#include <memory>
#include <chrono>

namespace notifications {

	struct notification_t
	{
		std::string message;
		float alpha{ 0.0f };
		float y_offset{ 0.0f };
		std::chrono::steady_clock::time_point start_time;
		std::chrono::steady_clock::time_point end_time;
		bool active{ true };
	};

	class notification_manager
	{
	public:
		void add( const std::string& message, float duration = 4.0f );
		void draw( );

	private:
		std::vector<notification_t> m_notifications{};
	};

	inline notification_manager g_notifications{};

} // namespace notifications
