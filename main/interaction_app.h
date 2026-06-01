#ifndef _INTERACTION_APP_H_
#define _INTERACTION_APP_H_

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <esp_timer.h>

#include <functional>
#include <mutex>
#include <deque>
#include <string>

#include "device_state.h"

#define MAIN_EVENT_SCHEDULE             (1 << 0)
#define MAIN_EVENT_CLOCK_TICK           (1 << 6)
#define MAIN_EVENT_NETWORK_CONNECTED    (1 << 7)
#define MAIN_EVENT_NETWORK_DISCONNECTED (1 << 8)

/** Touch + WebUI firmware — no voice assistant, no local CNC motion. */
class InteractionApp {
public:
    static InteractionApp& GetInstance() {
        static InteractionApp instance;
        return instance;
    }

    InteractionApp(const InteractionApp&) = delete;
    InteractionApp& operator=(const InteractionApp&) = delete;

    void Initialize();
    void Run();

    DeviceState GetDeviceState() const { return device_state_; }
    bool SetDeviceState(DeviceState state);

    void Schedule(std::function<void()>&& callback);
    void Alert(const char* title, const char* message, const char* icon = nullptr,
               const std::string_view& sound = {});
    void PlaySound(const std::string_view& sound) { (void)sound; }
    void ResetProtocol() {}
    bool IsVoiceDetected() { return false; }
    bool CanEnterSleepMode() { return false; }

private:
    InteractionApp();
    ~InteractionApp();

    void HandleNetworkConnected();
    void HandleNetworkDisconnected();

    EventGroupHandle_t event_group_ = nullptr;
    esp_timer_handle_t clock_timer_handle_ = nullptr;
    DeviceState device_state_ = kDeviceStateUnknown;

    std::mutex mutex_;
    std::deque<std::function<void()>> main_tasks_;
};

#endif
