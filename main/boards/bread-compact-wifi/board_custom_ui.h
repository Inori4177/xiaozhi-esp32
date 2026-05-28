#pragma once

#include <sdkconfig.h>
#include "boards/common/board_custom_ui.h"

#if CONFIG_MSP3525_LASER_UI
const BoardCustomUiOps *Msp3525GetLaserUiOps(void);
#endif
