#pragma once

class Display;
class Board;

/**
 * Optional board-specific UI integration (splash, alternate shell, chat log hook).
 * Return non-null from Board::GetCustomUiOps() in your board class.
 *
 * Splash step/state IDs mirror laser_ui_splash.h so Application can stay board-agnostic.
 */
enum BoardUiSplashStep {
    BOARD_UI_SPLASH_DISPLAY = 0,
    BOARD_UI_SPLASH_AUDIO,
    BOARD_UI_SPLASH_MCP,
    BOARD_UI_SPLASH_NETWORK,
    BOARD_UI_SPLASH_WIFI,
    BOARD_UI_SPLASH_ASSETS,
    BOARD_UI_SPLASH_OTA,
    BOARD_UI_SPLASH_PROTOCOL,
    BOARD_UI_SPLASH_READY,
};

enum BoardUiSplashState {
    BOARD_UI_SPLASH_WAIT = 0,
    BOARD_UI_SPLASH_RUNNING,
    BOARD_UI_SPLASH_OK,
    BOARD_UI_SPLASH_FAIL,
};

struct BoardCustomUiOps {
    /** Replace default SetupUI + system chat banner (laser splash, etc.). */
    void (*on_display_init)(Display *display);
    void (*on_splash_step)(BoardUiSplashStep step, BoardUiSplashState state);
    void (*on_activation_done)(Display *display);
    void (*on_chat_message)(const char *role, const char *content);
    void (*on_chrome_visible)(Display *display, bool top_bottom_visible, bool center_visible);
};

/** Helpers used by Application / LcdDisplay — no board-specific includes. */
const BoardCustomUiOps *BoardGetCustomUiOps(void);
bool BoardHasCustomUi(void);
void BoardUiOnDisplayInit(Display *display);
void BoardUiOnSplashStep(BoardUiSplashStep step, BoardUiSplashState state);
void BoardUiOnActivationDone(Display *display);
void BoardUiOnChatMessage(const char *role, const char *content);
void BoardUiSetChromeVisible(Display *display, bool top_bottom_visible, bool center_visible);
