#pragma once

#include <sdkconfig.h>
#include <string_view>

#if CONFIG_INTERACTION_UI_ONLY
#include "interaction_app.h"
using AppRuntime = InteractionApp;
#else
#include "application.h"
using AppRuntime = Application;
#endif
