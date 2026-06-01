#include "interaction_app.h"

#include "board.h"
#include "display.h"
#include "system_info.h"
#include "assets/lang_config.h"
#include "boards/common/board_custom_ui.h"

#include <esp_log.h>

#define TAG "InteractionApp"

InteractionApp::InteractionApp() {
    event_group_ = xEventGroupCreate();

    esp_timer_create_args_t clock_timer_args = {
        .callback = [](void* arg) {
            auto* app = static_cast<InteractionApp*>(arg);
            xEventGroupSetBits(app->event_group_, MAIN_EVENT_CLOCK_TICK);
        },
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "clock_timer",
        .skip_unhandled_events = true,
    };
    esp_timer_create(&clock_timer_args, &clock_timer_handle_);
}

InteractionApp::~InteractionApp() {
    if (clock_timer_handle_ != nullptr) {
        esp_timer_stop(clock_timer_handle_);
        esp_timer_delete(clock_timer_handle_);
    }
    if (event_group_ != nullptr) {
        vEventGroupDelete(event_group_);
    }
}

bool InteractionApp::SetDeviceState(DeviceState state) {
    device_state_ = state;
    return true;
}

void InteractionApp::Schedule(std::function<void()>&& callback) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        main_tasks_.push_back(std::move(callback));
    }
    xEventGroupSetBits(event_group_, MAIN_EVENT_SCHEDULE);
}

void InteractionApp::Alert(const char* title, const char* message, const char* icon,
                           const std::string_view& sound) {
    (void)icon;
    (void)sound;
    auto display = Board::GetInstance().GetDisplay();
    if (display == nullptr) {
        return;
    }
    std::string text = title;
    if (message != nullptr && message[0] != '\0') {
        text += ": ";
        text += message;
    }
    display->ShowNotification(text.c_str(), 5000);
}

void InteractionApp::Initialize() {
    auto& board = Board::GetInstance();
    SetDeviceState(kDeviceStateStarting);

    auto display = board.GetDisplay();
    if (BoardHasCustomUi()) {
        BoardUiOnDisplayInit(display);
    } else {
        display->SetupUI();
    }

    BoardUiOnSplashStep(BOARD_UI_SPLASH_AUDIO, BOARD_UI_SPLASH_OK);
    BoardUiOnSplashStep(BOARD_UI_SPLASH_MCP, BOARD_UI_SPLASH_OK);
    BoardUiOnSplashStep(BOARD_UI_SPLASH_ASSETS, BOARD_UI_SPLASH_OK);
    BoardUiOnSplashStep(BOARD_UI_SPLASH_OTA, BOARD_UI_SPLASH_OK);
    BoardUiOnSplashStep(BOARD_UI_SPLASH_PROTOCOL, BOARD_UI_SPLASH_OK);

    esp_timer_start_periodic(clock_timer_handle_, 1000000);

    board.SetNetworkEventCallback([this](NetworkEvent event, const std::string& data) {
        auto disp = Board::GetInstance().GetDisplay();
        switch (event) {
        case NetworkEvent::Scanning:
            disp->ShowNotification(Lang::Strings::SCANNING_WIFI, 30000);
            xEventGroupSetBits(event_group_, MAIN_EVENT_NETWORK_DISCONNECTED);
            break;
        case NetworkEvent::Connecting: {
            std::string msg = Lang::Strings::CONNECT_TO;
            msg += data;
            msg += "...";
            disp->ShowNotification(msg.c_str(), 30000);
            break;
        }
        case NetworkEvent::Connected: {
            std::string msg = Lang::Strings::CONNECTED_TO;
            msg += data;
            disp->ShowNotification(msg.c_str(), 30000);
            xEventGroupSetBits(event_group_, MAIN_EVENT_NETWORK_CONNECTED);
            BoardUiOnSplashStep(BOARD_UI_SPLASH_WIFI, BOARD_UI_SPLASH_OK);
            break;
        }
        case NetworkEvent::Disconnected:
            xEventGroupSetBits(event_group_, MAIN_EVENT_NETWORK_DISCONNECTED);
            break;
        default:
            break;
        }
    });

    BoardUiOnSplashStep(BOARD_UI_SPLASH_NETWORK, BOARD_UI_SPLASH_RUNNING);
    board.StartNetwork();
    BoardUiOnSplashStep(BOARD_UI_SPLASH_NETWORK, BOARD_UI_SPLASH_OK);

    display->UpdateStatusBar(true);
}

void InteractionApp::HandleNetworkConnected() {
    ESP_LOGI(TAG, "Network connected");
    auto state = GetDeviceState();
    if (state == kDeviceStateStarting || state == kDeviceStateWifiConfiguring) {
        SetDeviceState(kDeviceStateIdle);
        auto display = Board::GetInstance().GetDisplay();
        BoardUiOnActivationDone(display);
        Board::GetInstance().SetPowerSaveLevel(PowerSaveLevel::LOW_POWER);
        SystemInfo::PrintHeapStats();
    }
    Board::GetInstance().GetDisplay()->UpdateStatusBar(true);
}

void InteractionApp::HandleNetworkDisconnected() {
    Board::GetInstance().GetDisplay()->UpdateStatusBar(true);
}

void InteractionApp::Run() {
    vTaskPrioritySet(nullptr, 10);

    const EventBits_t all_events =
        MAIN_EVENT_SCHEDULE |
        MAIN_EVENT_CLOCK_TICK |
        MAIN_EVENT_NETWORK_CONNECTED |
        MAIN_EVENT_NETWORK_DISCONNECTED;

    while (true) {
        auto bits = xEventGroupWaitBits(event_group_, all_events, pdTRUE, pdFALSE, portMAX_DELAY);

        if (bits & MAIN_EVENT_NETWORK_CONNECTED) {
            HandleNetworkConnected();
        }
        if (bits & MAIN_EVENT_NETWORK_DISCONNECTED) {
            HandleNetworkDisconnected();
        }
        if (bits & MAIN_EVENT_SCHEDULE) {
            std::deque<std::function<void()>> tasks;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                tasks = std::move(main_tasks_);
            }
            for (auto& task : tasks) {
                task();
            }
        }
        if (bits & MAIN_EVENT_CLOCK_TICK) {
            Board::GetInstance().GetDisplay()->UpdateStatusBar();
        }
    }
}
