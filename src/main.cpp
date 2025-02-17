#include "Hooks.h"
#include "Settings.h"

extern "C" DLLEXPORT bool SKSEAPI
	SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
#ifndef NDEBUG
	auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
	auto path = logger::log_directory();
	if (!path) {
		return false;
	}

	*path /= fmt::format("{}.log"sv, Version::PROJECT);
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

#ifndef NDEBUG
	log->set_level(spdlog::level::trace);
#else
	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::info);
#endif

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("%s(%#): [%^%l%$] %v"s);

	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = Version::PROJECT.data();
	a_info->version = Version::MAJOR;

	if (a_skse->IsEditor()) {
		logger::critical("Loaded in editor, marking as incompatible"sv);
		return false;
	}

	const auto ver = a_skse->RuntimeVersion();
	if (ver <
#ifndef SKYRIMVR
		SKSE::RUNTIME_1_5_39
#else
		SKSE::RUNTIME_VR_1_4_15
#endif
	) {
		logger::critical(FMT_STRING("Unsupported runtime version {}"sv), ver.string());
		return false;
	}

	return true;
}


extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	logger::info("{} v{} loaded"sv, Version::PROJECT, Version::NAME);

	SKSE::Init(a_skse);
	SKSE::AllocTrampoline(8);

	Hooks::Install();

	Settings::GetSingleton()->LoadSettings();

	auto messaging = SKSE::GetMessagingInterface();
	messaging->RegisterListener(
		[](SKSE::MessagingInterface::Message* a_msg)
		{
			switch (a_msg->type) {
			case SKSE::MessagingInterface::kDataLoaded:
			{
				if (Settings::GetSingleton()->FlyingMountCruiseEnabled) {
					auto iniSettingCollection = RE::INISettingCollection::GetSingleton();
					if (iniSettingCollection) {
						auto cruiseSetting = iniSettingCollection->GetSetting(
							"bFlyingMountFastTravelCruiseEnabled:General");

						cruiseSetting->data.b = true;
						logger::info("Enabled flying mount cruise travel"sv);
					}
				}
				break;
			}
			}
		});

	return true;
}
