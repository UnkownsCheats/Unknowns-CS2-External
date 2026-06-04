#include <stdafx.hpp>

#include <array>
#include <charconv>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <system_error>
#include <vector>

namespace
{
    // Updated to a2x/cs2-dumper
    constexpr wchar_t repo_api_host[] = L"api.github.com";
    constexpr wchar_t repo_raw_host[] = L"raw.githubusercontent.com";
    constexpr wchar_t repo_api_path[] = L"/repos/a2x/cs2-dumper/git/trees/main?recursive=1";
    constexpr wchar_t repo_raw_path[] = L"/a2x/cs2-dumper/main/output/";

    struct offset_entry
    {
        const char* module;
        const char* key;
        std::uintptr_t offsets::* member;
        std::uintptr_t fallback;
        bool module_relative;
    };

    constexpr std::array offset_entries{
        offset_entry{ "client.dll", "dwCSGOInput", &offsets::csgo_input, 0x237B5B0, true },
        offset_entry{ "client.dll", "dwEntityList", &offsets::entity_list, 0x250C5B0, true },
        offset_entry{ "client.dll", "dwGameEntitySystem", &offsets::game_entity_system, 0x250C5B0, true },
        offset_entry{ "client.dll", "dwGameEntitySystem_highestEntityIndex", &offsets::game_entity_system_highest_entity_index, 0x2090, false },
        offset_entry{ "client.dll", "dwGameRules", &offsets::game_rules, 0x1A21D60, true },
        offset_entry{ "client.dll", "dwGlobalVars", &offsets::global_vars, 0x2085788, true },
        offset_entry{ "client.dll", "dwGlowManager", &offsets::glow_manager, 0x2363580, true },
        offset_entry{ "client.dll", "dwLocalPlayerController", &offsets::local_player_controller, 0x2345D60, true },
        offset_entry{ "client.dll", "dwLocalPlayerPawn", &offsets::local_player_pawn, 0x2090880, true },
        offset_entry{ "client.dll", "dwPlantedC4", &offsets::planted_c4, 0x2374278, true },
        offset_entry{ "client.dll", "dwPrediction", &offsets::prediction, 0x2090790, true },
        offset_entry{ "client.dll", "dwSensitivity", &offsets::sensitivity, 0x2364098, true },
        offset_entry{ "client.dll", "dwSensitivity_sensitivity", &offsets::sensitivity_sensitivity, 0x58, false },
        offset_entry{ "client.dll", "dwViewAngles", &offsets::view_angles, 0x237BC38, true },
        offset_entry{ "client.dll", "dwViewMatrix", &offsets::view_matrix, 0x236C2F0, true },
        offset_entry{ "client.dll", "dwViewRender", &offsets::view_render, 0x236B4F8, true },
        offset_entry{ "client.dll", "dwWeaponC4", &offsets::weapon_c4, 0x22E4518, true },

        offset_entry{ "engine2.dll", "dwBuildNumber", &offsets::build_number, 0x60CC74, true },
        offset_entry{ "engine2.dll", "dwNetworkGameClient", &offsets::network_game_client, 0x90A1A0, true },
        offset_entry{ "engine2.dll", "dwNetworkGameClient_clientTickCount", &offsets::network_game_client_client_tick_count, 0x378, false },
        offset_entry{ "engine2.dll", "dwNetworkGameClient_deltaTick", &offsets::network_game_client_delta_tick, 0x24C, false },
        offset_entry{ "engine2.dll", "dwNetworkGameClient_isBackgroundMap", &offsets::network_game_client_is_background_map, 0x2C141F, false },
        offset_entry{ "engine2.dll", "dwNetworkGameClient_localPlayer", &offsets::network_game_client_local_player, 0xF8, false },
        offset_entry{ "engine2.dll", "dwNetworkGameClient_maxClients", &offsets::network_game_client_max_clients, 0x240, false },
        offset_entry{ "engine2.dll", "dwNetworkGameClient_serverTickCount", &offsets::network_game_client_server_tick_count, 0x24C, false },
        offset_entry{ "engine2.dll", "dwNetworkGameClient_signOnState", &offsets::network_game_client_sign_on_state, 0x230, false },
        offset_entry{ "engine2.dll", "dwWindowHeight", &offsets::window_height, 0x90E5C4, true },
        offset_entry{ "engine2.dll", "dwWindowWidth", &offsets::window_width, 0x90E5C0, true },

        offset_entry{ "inputsystem.dll", "dwInputSystem", &offsets::input_system, 0x42B50, true },
        offset_entry{ "matchmaking.dll", "dwGameTypes", &offsets::game_types, 0x1DF0B0, true },
        offset_entry{ "soundsystem.dll", "dwSoundSystem", &offsets::sound_system, 0x512360, true },
        offset_entry{ "soundsystem.dll", "dwSoundSystem_engineViewData", &offsets::sound_system_engine_view_data, 0x7C, false },
    };

    // This list already matches the files inside the output/ folder of a2x/cs2-dumper
    constexpr std::array fallback_repo_files{
        "animationsystem_dll.json",
        "buttons.json",
        "client_dll.json",
        "engine2_dll.json",
        "host_dll.json",
        "info.json",
        "interfaces.json",
        "materialsystem2_dll.json",
        "netvars.json",
        "networksystem_dll.json",
        "offsets.json",
        "panorama_dll.json",
        "particles_dll.json",
        "pulse_system_dll.json",
        "rendersystemdx11_dll.json",
        "resourcesystem_dll.json",
        "scenesystem_dll.json",
        "schemas.json",
        "schemasystem_dll.json",
        "server_dll.json",
        "soundsystem_dll.json",
        "steamaudio_dll.json",
        "vphysics2_dll.json",
        "worldrenderer_dll.json",
    };

    std::wstring widen_ascii(std::string_view value)
    {
        return { value.begin(), value.end() };
    }

    std::string value_key(std::string_view module, std::string_view key)
    {
        std::string result{ module };
        result += "::";
        result += key;
        return result;
    }

    bool ends_with(std::string_view value, std::string_view suffix)
    {
        return value.size() >= suffix.size() && value.substr(value.size() - suffix.size()) == suffix;
    }

    std::string download(const wchar_t* host, const std::wstring& path)
    {
        HINTERNET session = InternetOpenW(L"Unknown?/1.0", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
        if (!session)
            return {};

        DWORD timeout = 10000;
        InternetSetOptionW(session, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
        InternetSetOptionW(session, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
        InternetSetOptionW(session, INTERNET_OPTION_SEND_TIMEOUT, &timeout, sizeof(timeout));

        HINTERNET connection = InternetConnectW(session, host, INTERNET_DEFAULT_HTTPS_PORT, nullptr, nullptr, INTERNET_SERVICE_HTTP, 0, 0);
        if (!connection)
        {
            InternetCloseHandle(session);
            return {};
        }

        HINTERNET request = HttpOpenRequestW(connection, L"GET", path.c_str(), nullptr, nullptr, nullptr, INTERNET_FLAG_SECURE | INTERNET_FLAG_RELOAD, 0);
        if (!request)
        {
            InternetCloseHandle(connection);
            InternetCloseHandle(session);
            return {};
        }

        constexpr wchar_t headers[] =
            L"User-Agent: Unknown?/1.0\r\n"
            L"Accept: application/vnd.github+json\r\n"
            L"Cache-Control: no-cache\r\n";

        if (!HttpSendRequestW(request, headers, static_cast<DWORD>(-1L), nullptr, 0))
        {
            InternetCloseHandle(request);
            InternetCloseHandle(connection);
            InternetCloseHandle(session);
            return {};
        }

        DWORD status_code = 0;
        DWORD status_code_size = sizeof(status_code);
        DWORD index = 0;
        if (!HttpQueryInfoW(request, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &status_code, &status_code_size, &index) || status_code < 200 || status_code >= 300)
        {
            InternetCloseHandle(request);
            InternetCloseHandle(connection);
            InternetCloseHandle(session);
            return {};
        }

        std::string result;
        DWORD available = 0;
        constexpr DWORD buffer_size = 8192;
        std::array<char, buffer_size> buffer{};

        while (InternetReadFile(request, buffer.data(), buffer_size, &available) && available > 0)
        {
            result.append(buffer.data(), available);
        }

        InternetCloseHandle(request);
        InternetCloseHandle(connection);
        InternetCloseHandle(session);

        return result;
    }

    std::filesystem::path cache_dir()
    {
        std::array<wchar_t, MAX_PATH> path{};
        const auto size = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
        if (size != 0)
            return std::filesystem::path{ path.data(), path.data() + size }.parent_path() / "cs2-offsets-cache";

        return std::filesystem::current_path() / "cs2-offsets-cache";
    }

    bool cache_name_is_safe(std::string_view name)
    {
        if (name.empty() || name.find_first_of("\\:*?\"<>|") != std::string_view::npos)
            return false;

        const std::filesystem::path path{ std::string{ name } };
        if (path.is_absolute())
            return false;

        for (const auto& part : path)
        {
            if (part == "." || part == "..")
                return false;
        }

        return true;
    }

    bool load_cached_file(std::string_view name, std::string& content)
    {
        if (!cache_name_is_safe(name))
            return false;

        std::ifstream file{ cache_dir() / std::filesystem::path{ std::string{ name } }, std::ios::binary };
        if (!file)
            return false;

        content.assign(std::istreambuf_iterator<char>{ file }, std::istreambuf_iterator<char>{ });
        return !content.empty();
    }

    void save_cached_file(std::string_view name, const std::string& content)
    {
        if (!cache_name_is_safe(name) || content.empty())
            return;

        const auto dir = cache_dir();
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);
        if (ec)
            return;

        const auto target = dir / std::filesystem::path{ std::string{ name } };
        std::filesystem::create_directories(target.parent_path(), ec);
        if (ec)
            return;

        std::ofstream file{ target, std::ios::binary | std::ios::trunc };
        if (file)
            file.write(content.data(), static_cast<std::streamsize>(content.size()));
    }

    bool read_json_string(const std::string& json, std::size_t quote_pos, std::string& value, std::size_t* next_pos = nullptr)
    {
        if (quote_pos >= json.size() || json[quote_pos] != '"')
            return false;

        value.clear();
        for (auto pos = quote_pos + 1; pos < json.size(); ++pos)
        {
            const auto ch = json[pos];
            if (ch == '"')
            {
                if (next_pos)
                    *next_pos = pos + 1;
                return true;
            }

            if (ch == '\\' && pos + 1 < json.size())
            {
                const auto escaped = json[++pos];
                switch (escaped)
                {
                case '"':
                case '\\':
                case '/':
                    value += escaped;
                    break;
                case 'b':
                    value += '\b';
                    break;
                case 'f':
                    value += '\f';
                    break;
                case 'n':
                    value += '\n';
                    break;
                case 'r':
                    value += '\r';
                    break;
                case 't':
                    value += '\t';
                    break;
                default:
                    value += escaped;
                    break;
                }

                continue;
            }

            value += ch;
        }

        return false;
    }

    std::vector<std::string> extract_repo_file_names(const std::string& listing)
    {
        std::vector<std::string> names;
        std::size_t pos = 0;

        while ((pos = listing.find("\"path\"", pos)) != std::string::npos)
        {
            const auto colon = listing.find(':', pos);
            if (colon == std::string::npos)
                break;

            const auto quote = listing.find('"', colon + 1);
            if (quote == std::string::npos)
                break;

            std::string full_path;
            std::size_t next_pos = quote + 1;
            if (read_json_string(listing, quote, full_path, &next_pos))
            {
                // Only consider files inside the "output/" directory
                const std::string_view prefix = "output/";
                if (full_path.size() > prefix.size() && full_path.compare(0, prefix.size(), prefix) == 0)
                {
                    // Strip the prefix so we can use the raw path directly
                    std::string stripped = full_path.substr(prefix.size());
                    names.push_back(stripped);
                }
            }

            pos = next_pos;
        }

        return names;
    }

    std::vector<std::string> repo_file_names()
    {
        auto listing = download(repo_api_host, repo_api_path);
        auto names = extract_repo_file_names(listing);
        if (!names.empty())
        {
            g::console.print("[offsets] github tree listed {} files", names.size());
            return names;
        }

        g::console.warn("[offsets] github tree listing failed, using fallback file list");
        return { fallback_repo_files.begin(), fallback_repo_files.end() };
    }

    std::unordered_map<std::string, std::string> download_repo_files()
    {
        std::unordered_map<std::string, std::string> files;
        std::size_t downloaded = 0;
        std::size_t cached = 0;
        std::size_t failed = 0;

        for (const auto& name : repo_file_names())
        {
            auto content = download(repo_raw_host, std::wstring{ repo_raw_path } + widen_ascii(name));
            if (!content.empty())
            {
                save_cached_file(name, content);
                files.emplace(name, std::move(content));
                ++downloaded;
                continue;
            }

            if (load_cached_file(name, content))
            {
                files.emplace(name, std::move(content));
                ++cached;
                continue;
            }

            ++failed;
        }

        g::console.print("[offsets] repo files loaded: {} downloaded, {} cached, {} failed", downloaded, cached, failed);
        return files;
    }

    bool find_object_range(const std::string& json, std::string_view key, std::size_t& object_begin, std::size_t& object_end)
    {
        const auto needle = std::string{ "\"" } + std::string{ key } + "\"";
        const auto key_pos = json.find(needle);
        if (key_pos == std::string::npos)
            return false;

        const auto colon = json.find(':', key_pos + needle.size());
        if (colon == std::string::npos)
            return false;

        const auto first_brace = json.find('{', colon + 1);
        if (first_brace == std::string::npos)
            return false;

        std::size_t depth = 0;
        bool in_string = false;
        bool escaped = false;

        for (auto pos = first_brace; pos < json.size(); ++pos)
        {
            const auto ch = json[pos];
            if (in_string)
            {
                if (escaped)
                {
                    escaped = false;
                    continue;
                }

                if (ch == '\\')
                {
                    escaped = true;
                    continue;
                }

                if (ch == '"')
                    in_string = false;

                continue;
            }

            if (ch == '"')
            {
                in_string = true;
                continue;
            }

            if (ch == '{')
            {
                if (depth++ == 0)
                    object_begin = pos;
                continue;
            }

            if (ch == '}' && depth > 0 && --depth == 0)
            {
                object_end = pos + 1;
                return true;
            }
        }

        return false;
    }

    bool extract_string_field(const std::string& json, std::string_view key, std::size_t begin, std::size_t end, std::string& value)
    {
        const auto needle = std::string{ "\"" } + std::string{ key } + "\"";
        const auto key_pos = json.find(needle, begin);
        if (key_pos == std::string::npos || key_pos >= end)
            return false;

        const auto colon = json.find(':', key_pos + needle.size());
        if (colon == std::string::npos || colon >= end)
            return false;

        const auto quote = json.find('"', colon + 1);
        if (quote == std::string::npos || quote >= end)
            return false;

        return read_json_string(json, quote, value);
    }

    bool extract_number_field(const std::string& json, std::string_view key, std::size_t begin, std::size_t end, std::uintptr_t& value)
    {
        const auto needle = std::string{ "\"" } + std::string{ key } + "\"";
        const auto key_pos = json.find(needle, begin);
        if (key_pos == std::string::npos || key_pos >= end)
            return false;

        const auto colon = json.find(':', key_pos + needle.size());
        if (colon == std::string::npos || colon >= end)
            return false;

        auto value_begin = colon + 1;
        while (value_begin < end && std::isspace(static_cast<unsigned char>(json[value_begin])))
            ++value_begin;

        const auto quoted = value_begin < end && json[value_begin] == '"';
        if (quoted)
            ++value_begin;

        auto value_end = value_begin;
        while (value_end < end && (std::isxdigit(static_cast<unsigned char>(json[value_end])) || json[value_end] == 'x' || json[value_end] == 'X'))
            ++value_end;

        if (value_begin == value_end)
            return false;

        std::uint64_t parsed = 0;
        const auto raw_value = std::string_view{ json.data() + value_begin, value_end - value_begin };
        if (raw_value.size() > 2 && raw_value[0] == '0' && (raw_value[1] == 'x' || raw_value[1] == 'X'))
        {
            const auto hex = raw_value.substr(2);
            const auto [ptr, ec] = std::from_chars(hex.data(), hex.data() + hex.size(), parsed, 16);
            if (ec != std::errc{ } || ptr != hex.data() + hex.size())
                return false;
        }
        else
        {
            const auto [ptr, ec] = std::from_chars(raw_value.data(), raw_value.data() + raw_value.size(), parsed, 10);
            if (ec != std::errc{ } || ptr != raw_value.data() + raw_value.size())
                return false;
        }

        value = static_cast<std::uintptr_t>(parsed);
        return true;
    }

    bool extract_offset(const std::string& json, std::string_view module, std::string_view key, std::uintptr_t& value)
    {
        std::size_t module_begin = 0;
        std::size_t module_end = 0;
        return find_object_range(json, module, module_begin, module_end) && extract_number_field(json, key, module_begin, module_end, value);
    }

    std::uintptr_t module_base(std::string_view module)
    {
        if (module == "client.dll")
            return g::modules.client;
        if (module == "engine2.dll")
            return g::modules.engine2;
        if (module == "schemasystem.dll")
            return g::modules.schemasystem;
        if (module == "vphysics2.dll")
            return g::modules.vphysics2;

        return g::memory.get_module(module);
    }

    std::uintptr_t resolve_entry_value(const offset_entry& entry, std::uintptr_t rva)
    {
        if (!entry.module_relative)
            return rva;

        const auto base = module_base(entry.module);
        return base ? base + rva : rva;
    }
}

std::uintptr_t offsets::get(std::string_view module, std::string_view key) const
{
    const auto value = this->m_values.find(value_key(module, key));
    return value == this->m_values.end() ? 0 : value->second;
}

const std::string* offsets::repo_file(std::string_view name) const
{
    const auto file = this->m_repo_files.find(std::string{ name });
    return file == this->m_repo_files.end() ? nullptr : &file->second;
}

bool offsets::initialize()
{
    this->m_values.clear();
    this->m_repo_files = download_repo_files();

    if (this->m_repo_files.empty())
        g::console.warn("failed to download CS2-OFFSETS repo files, using compiled fallback offsets");
    else
        g::console.success("downloaded {} CS2-OFFSETS files from github", this->m_repo_files.size());

    for (const auto name : { "info.json", "offsets.json", "client_dll.json", "schemas.json", "buttons.json" })
    {
        if (const auto file = this->repo_file(name))
            g::console.print("[offsets] {} loaded ({} bytes)", name, file->size());
        else
            g::console.warn("[offsets] {} missing", name);
    }

    if (const auto info = this->repo_file("info.json"))
    {
        std::uintptr_t repo_build = 0;
        std::string timestamp;
        if (extract_number_field(*info, "build_number", 0, info->size(), repo_build) && extract_string_field(*info, "timestamp", 0, info->size(), timestamp))
            g::console.print("CS2-OFFSETS build {} ({})", repo_build, timestamp);
    }

    const auto offsets_json = this->repo_file("offsets.json");
    std::size_t parsed = 0;

    for (const auto& entry : offset_entries)
    {
        auto rva = entry.fallback;
        if (offsets_json && extract_offset(*offsets_json, entry.module, entry.key, rva))
            ++parsed;

        const auto value = resolve_entry_value(entry, rva);
        this->*(entry.member) = value;
        this->m_values[value_key(entry.module, entry.key)] = value;

        g::console.print(
            "[offsets] {}!{} rva=0x{:X} value=0x{:X}{}",
            entry.module,
            entry.key,
            rva,
            value,
            entry.module_relative ? " (module+rva)" : ""
        );
    }

    if (parsed != offset_entries.size())
        g::console.warn("loaded {} of {} global offsets from github, using fallback values for the rest", parsed, offset_entries.size());

    return true;
}