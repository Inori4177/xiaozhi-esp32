#include "ui_kanjivg_mcp_bridge.h"

#include "../../laser_ui_state.h"
#include "mcp_server.h"

#include <cJSON.h>
#include <cmath>

void ui_kanjivg_mcp_bridge_register(void)
{
    auto &mcp = McpServer::GetInstance();
    mcp.AddTool(
        "self.cnc.get_pick_origin",
        "读取激光 UI 选定页设置的原点 (mm)。valid=false 表示尚未在选定页确认。\n"
        "雕刻前可先调用此工具，将 x/y 用于 self.engraving.engrave_text。",
        PropertyList(),
        [](const PropertyList &properties) -> ReturnValue {
            (void)properties;
            float x_mm = 0.0f;
            float y_mm = 0.0f;
            const bool valid = laser_ui_state_get_pick_origin(&x_mm, &y_mm);

            cJSON *root = cJSON_CreateObject();
            cJSON_AddBoolToObject(root, "valid", valid);
            if (valid) {
                cJSON_AddNumberToObject(root, "x", static_cast<int>(std::lround(x_mm)));
                cJSON_AddNumberToObject(root, "y", static_cast<int>(std::lround(y_mm)));
            }
            return root;
        });
}
