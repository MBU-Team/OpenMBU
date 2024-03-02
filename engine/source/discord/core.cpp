#if !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "core.h"

#include <cstring>
#include <memory>

namespace discord {

Result Core::Create(ClientId clientId, std::uint64_t flags, Core** instance)
{
    if (!instance) {
        return Result::InternalError;
    }

    (*instance) = new Core();
    DiscordCreateParams params{};
    DiscordCreateParamsSetDefault(&params);
    params.client_id = clientId;
    params.flags = flags;
    params.events = nullptr;
    params.event_data = *instance;
    params.user_events = &UserManager::events_;
    params.activity_events = &ActivityManager::events_;
    params.relationship_events = &RelationshipManager::events_;
    params.overlay_events = &OverlayManager::events_;
    auto result = DiscordCreate(DISCORD_VERSION, &params, &((*instance)->internal_));
    if (result != DiscordResult_Ok || !(*instance)->internal_) {
        delete (*instance);
        (*instance) = nullptr;
    }

    return static_cast<Result>(result);
}

Core::~Core()
{
    if (internal_) {
        internal_->destroy(internal_);
        internal_ = nullptr;
    }
}

Result Core::RunCallbacks()
{
    auto result = internal_->run_callbacks(internal_);
    return static_cast<Result>(result);
}

void Core::SetLogHook(LogLevel minLevel, std::function<void(LogLevel, char const*)> hook)
{
    setLogHook_.DisconnectAll();
    setLogHook_.Connect(std::move(hook));
    static auto wrapper =
      [](void* callbackData, EDiscordLogLevel level, char const* message) -> void {
        auto cb(reinterpret_cast<decltype(setLogHook_)*>(callbackData));
        if (!cb) {
            return;
        }
        (*cb)(static_cast<LogLevel>(level), static_cast<const char*>(message));
    };

    internal_->set_log_hook(
      internal_, static_cast<EDiscordLogLevel>(minLevel), &setLogHook_, wrapper);
}

discord::UserManager& Core::UserManager()
{
    if (!userManager_.internal_) {
        userManager_.internal_ = internal_->get_user_manager(internal_);
    }

    return userManager_;
}

discord::ActivityManager& Core::ActivityManager()
{
    if (!activityManager_.internal_) {
        activityManager_.internal_ = internal_->get_activity_manager(internal_);
    }

    return activityManager_;
}

discord::RelationshipManager& Core::RelationshipManager()
{
    if (!relationshipManager_.internal_) {
        relationshipManager_.internal_ = internal_->get_relationship_manager(internal_);
    }

    return relationshipManager_;
}

discord::OverlayManager& Core::OverlayManager()
{
    if (!overlayManager_.internal_) {
        overlayManager_.internal_ = internal_->get_overlay_manager(internal_);
    }

    return overlayManager_;
}

} // namespace discord
