#include "board_custom_ui.h"
#include "board.h"

const BoardCustomUiOps *BoardGetCustomUiOps(void)
{
    return Board::GetInstance().GetCustomUiOps();
}

bool BoardHasCustomUi(void)
{
    return BoardGetCustomUiOps() != nullptr;
}

void BoardUiOnDisplayInit(Display *display)
{
    const BoardCustomUiOps *ops = BoardGetCustomUiOps();
    if (ops != nullptr && ops->on_display_init != nullptr) {
        ops->on_display_init(display);
    }
}

void BoardUiOnSplashStep(BoardUiSplashStep step, BoardUiSplashState state)
{
    const BoardCustomUiOps *ops = BoardGetCustomUiOps();
    if (ops != nullptr && ops->on_splash_step != nullptr) {
        ops->on_splash_step(step, state);
    }
}

void BoardUiOnActivationDone(Display *display)
{
    const BoardCustomUiOps *ops = BoardGetCustomUiOps();
    if (ops != nullptr && ops->on_activation_done != nullptr) {
        ops->on_activation_done(display);
    }
}

void BoardUiOnChatMessage(const char *role, const char *content)
{
    const BoardCustomUiOps *ops = BoardGetCustomUiOps();
    if (ops != nullptr && ops->on_chat_message != nullptr) {
        ops->on_chat_message(role, content);
    }
}

void BoardUiSetChromeVisible(Display *display, bool top_bottom_visible, bool center_visible)
{
    const BoardCustomUiOps *ops = BoardGetCustomUiOps();
    if (ops != nullptr && ops->on_chrome_visible != nullptr) {
        ops->on_chrome_visible(display, top_bottom_visible, center_visible);
    }
}
