var websocket_port = 0;
var websocket_IP = "";
var ws_source = null;
var websocket_started = false;
var files_currentPath = "/";
var status_timer = null;

function $(id) { return document.getElementById(id); }

function log_append(msg) {
    var el = $("log");
    if (!el) return;
    el.textContent += msg + (msg.endsWith("\n") ? "" : "\n");
    el.scrollTop = el.scrollHeight;
}

function set_conn_status(text, ok) {
    var el = $("conn_status");
    if (!el) return;
    el.textContent = text;
    el.classList.remove("ok", "err");
    if (ok === true) el.classList.add("ok");
    if (ok === false) el.classList.add("err");
}

function format_eta(sec, has_eta) {
    if (!has_eta || sec <= 0) return "—";
    var m = Math.floor(sec / 60);
    var s = sec % 60;
    if (m > 99) return m + " 分";
    return m + ":" + (s < 10 ? "0" : "") + s;
}

function apply_machine_status(st) {
    if (!st) return;
    var stateEl = $("st_state");
    var etaEl = $("st_eta");
    var fileEl = $("st_file");
    var pctEl = $("st_pct");
    var barEl = $("st_bar");
    if (stateEl) stateEl.textContent = st.state || "—";
    if (etaEl) etaEl.textContent = format_eta(st.eta_sec, st.has_eta);
    if (fileEl) {
        var fn = (st.file && st.file !== "-") ? st.file : "—";
        fileEl.textContent = fn;
    }
    var pct = Math.max(0, Math.min(100, st.progress || 0));
    if (pctEl) pctEl.textContent = pct + "%";
    if (barEl) barEl.style.width = pct + "%";

    if (typeof settings_apply_from_status === "function") {
        settings_apply_from_status(st);
    }
    if (typeof pick_on_status === "function") {
        pick_on_status(st);
    }
}

function poll_status() {
    fetch("/status")
        .then(function (r) { return r.json(); })
        .then(apply_machine_status)
        .catch(function () {});
}

function show_tab(name) {
    document.querySelectorAll(".panel").forEach(function (p) {
        p.classList.toggle("active", p.id === "panel_" + name);
    });
    document.querySelectorAll("nav.tabs button").forEach(function (b) {
        b.classList.toggle("active", b.dataset.tab === name);
    });
    if (name === "jog" && typeof pick_refresh_layout === "function") {
        requestAnimationFrame(pick_refresh_layout);
    }
}

function http_command(cmd, onok) {
    var url = "/command?commandText=" + encodeURIComponent(cmd);
    fetch(url).then(function (r) {
        if (!r.ok) throw new Error(r.statusText);
        return r.text();
    }).then(function (t) {
        log_append("<< " + (t || "ok"));
        if (onok) onok(t);
    }).catch(function (e) {
        log_append("!! " + e.message);
    });
}

function SendCustomCommand() {
    var input = $("custom_cmd_txt");
    if (!input) return;
    var cmd = input.value.trim();
    if (!cmd) return;
    if (ws_source && websocket_started) {
        ws_source.send(cmd + "\n");
        log_append(">> " + cmd);
    } else {
        http_command(cmd);
    }
}

function SendJog(axis, sign) {
    var step = parseFloat(($("jog_step_mm") && $("jog_step_mm").value) || "1") || 1;
    var delta = sign < 0 ? -step : step;
    var axisPart = axis + delta;
    var msg = "$J=" + axisPart + " F0";
    if (ws_source && websocket_started) {
        ws_source.send(msg);
        log_append(">> " + msg + " (" + Math.abs(delta) + " mm)");
    } else {
        http_command(msg);
    }
}

function startSocket() {
    if (ws_source) {
        try { ws_source.close(); } catch (e) {}
    }
    var host = websocket_IP || location.hostname;
    var port = websocket_port || location.port || "80";
    var url = "ws://" + host + ":" + port + "/ws";
    ws_source = new WebSocket(url);
    ws_source.onopen = function () {
        websocket_started = true;
        set_conn_status("已连接", true);
        log_append("--- WebSocket open ---");
    };
    ws_source.onclose = function () {
        websocket_started = false;
        set_conn_status("已断开", false);
        setTimeout(startSocket, 3000);
    };
    ws_source.onerror = function () {
        set_conn_status("连接错误", false);
    };
    ws_source.onmessage = function (e) {
        log_append("<< " + e.data);
    };
}

function InitUI() {
    set_conn_status("初始化…", null);
    fetch("/command?commandText=" + encodeURIComponent("[ESP800]"))
        .then(function (r) { return r.text(); })
        .then(function () {
            websocket_IP = location.hostname;
            websocket_port = parseInt(location.port || "80", 10);
            startSocket();
            set_conn_status("就绪", true);
            files_refreshFiles("/");
            poll_status();
            if (status_timer) clearInterval(status_timer);
            status_timer = setInterval(poll_status, 800);
        })
        .catch(function (e) {
            set_conn_status("初始化失败", false);
            log_append("!! " + e.message);
        });
}

function files_refreshFiles(path) {
    files_currentPath = path || "/";
    var url = "/files?action=list&path=" + encodeURIComponent(files_currentPath) + "&filename=all";
    fetch(url).then(function (r) { return r.json(); }).then(files_dispatch).catch(function (e) {
        $("files_status").textContent = e.message;
    });
}

function files_dispatch(json) {
    $("files_status").textContent = "路径: " + json.path;
    var tbody = $("file_list");
    tbody.innerHTML = "";
    if (files_currentPath !== "/") {
        var tr = document.createElement("tr");
        tr.innerHTML = "<td colspan='4'><button type='button' class='ctrl' onclick=\"files_refreshFiles('/')\">/</button></td>";
        tbody.appendChild(tr);
    }
    (json.files || []).forEach(function (f) {
        var tr = document.createElement("tr");
        var safeName = f.name.replace(/'/g, "\\'");
        if (String(f.size) === "-1") {
            tr.innerHTML = "<td>[dir]</td><td>" + f.name + "</td><td></td><td></td>";
        } else {
            var isGcode = /\.g(code|co)?$/i.test(f.name);
            var runBtn = isGcode
                ? "<button type='button' class='ctrl primary' onclick=\"files_run('" + safeName + "')\">运行</button> "
                : "";
            tr.innerHTML = "<td>file</td><td>" + f.name + "</td><td>" + f.size + "</td><td>" +
                runBtn +
                "<button type='button' class='ctrl' onclick=\"files_delete('" + safeName + "')\">删除</button></td>";
        }
        tbody.appendChild(tr);
    });
}

function files_run(name) {
    var path = files_currentPath || "/";
    if (!path.endsWith("/")) path += "/";
    var cmd = "[ESP700]" + path + name;
    var input = $("custom_cmd_txt");
    if (input) input.value = cmd;
    SendCustomCommand();
}

function files_delete(name) {
    if (!confirm("从设备存储永久删除 “" + name + "”？")) return;
    var url = "/files?action=delete&path=" + encodeURIComponent(files_currentPath) +
        "&filename=" + encodeURIComponent(name);
    fetch(url).then(function (r) { return r.json(); }).then(files_dispatch);
}

function files_upload_blob(blob, destPath) {
    var rel = destPath.replace(/^\/+/, "");
    var url = "/files?action=upload&path=" + encodeURIComponent(rel);
    return fetch(url, {
        method: "POST",
        headers: { "Content-Type": "application/octet-stream" },
        body: blob
    }).then(function (r) {
        return r.json().then(function (j) {
            if (!r.ok) throw new Error(j.msg || r.statusText);
            return j;
        });
    });
}

function files_upload_selected() {
    var input = $("files_input_file");
    if (!input || !input.files || !input.files.length) return;
    var path = files_currentPath || "/";
    if (!path.endsWith("/")) path += "/";
    var dest = path + input.files[0].name;
    files_upload_blob(input.files[0], dest)
        .then(files_dispatch)
        .catch(function (e) { $("files_status").textContent = e.message; });
}

function wifi_scan() {
    http_command("[ESP410]", function (t) { $("wifi_scan_result").textContent = t; });
}

function wifi_connect() {
    var ssid = $("wifi_ssid").value.trim();
    var pass = $("wifi_pass").value;
    if (!ssid) return;
    http_command("[ESP420]" + ssid + "|" + pass);
}

/* --- settings / pick (formerly settings.js) --- */
var PICK_WORK_MM = 42;
var PICK_MAP_PAD = 0.08;
var pick_cursor_x = 0;
var pick_cursor_y = 0;
var pick_dragging = false;
var pick_cursor_pending = false;
var pick_last_status = null;
var settings_form_dirty = false;
var settings_status_boot = false;

function pick_el(id) { return document.getElementById(id); }

function pick_mm_to_pct(mm_x, mm_y) {
    var cx = Math.max(0, Math.min(PICK_WORK_MM, mm_x));
    var cy = Math.max(0, Math.min(PICK_WORK_MM, mm_y));
    var span = (1 - 2 * PICK_MAP_PAD) * 100;
    return {
        left: PICK_MAP_PAD * 100 + (cx / PICK_WORK_MM) * span,
        top: PICK_MAP_PAD * 100 + (1 - cy / PICK_WORK_MM) * span
    };
}

function pick_client_to_mm(clientX, clientY) {
    var map = pick_el("pick_map");
    if (!map) return { x: pick_cursor_x, y: pick_cursor_y };
    var r = map.getBoundingClientRect();
    if (r.width <= 0 || r.height <= 0) {
        return { x: pick_cursor_x, y: pick_cursor_y };
    }
    var innerLeft = r.left + r.width * PICK_MAP_PAD;
    var innerTop = r.top + r.height * PICK_MAP_PAD;
    var innerW = r.width * (1 - 2 * PICK_MAP_PAD);
    var innerH = r.height * (1 - 2 * PICK_MAP_PAD);
    var x = (clientX - innerLeft) / innerW * PICK_WORK_MM;
    var y = (innerTop + innerH - clientY) / innerH * PICK_WORK_MM;
    return {
        x: Math.max(0, Math.min(PICK_WORK_MM, x)),
        y: Math.max(0, Math.min(PICK_WORK_MM, y))
    };
}

function pick_place_cursor(mm_x, mm_y) {
    pick_cursor_x = mm_x;
    pick_cursor_y = mm_y;
    var cur = pick_el("pick_cursor");
    if (!cur) return;
    var p = pick_mm_to_pct(mm_x, mm_y);
    cur.style.left = p.left + "%";
    cur.style.top = p.top + "%";
    var cx = pick_el("pick_cx");
    var cy = pick_el("pick_cy");
    if (cx) cx.textContent = mm_x.toFixed(1);
    if (cy) cy.textContent = mm_y.toFixed(1);
}

function pick_place_head(mm_x, mm_y) {
    var head = pick_el("pick_head");
    if (!head) return;
    var p = pick_mm_to_pct(mm_x, mm_y);
    head.style.left = p.left + "%";
    head.style.top = p.top + "%";
    var hx = pick_el("pick_hx");
    var hy = pick_el("pick_hy");
    if (hx) hx.textContent = mm_x.toFixed(1);
    if (hy) hy.textContent = mm_y.toFixed(1);
}

function pick_on_status(st) {
    if (!st) return;
    pick_last_status = st;
    pick_place_head(st.x_mm || 0, st.y_mm || 0);
    if (pick_dragging || pick_cursor_pending) {
        return;
    }
    if (st.pick_valid) {
        pick_place_cursor(st.pick_x, st.pick_y);
    } else {
        pick_place_cursor(st.x_mm || 0, st.y_mm || 0);
    }
}

function pick_refresh_layout() {
    if (pick_last_status) {
        pick_on_status(pick_last_status);
    } else {
        pick_place_cursor(pick_cursor_x, pick_cursor_y);
    }
}

function pick_bind_drag() {
    var map = pick_el("pick_map");
    if (!map) return;

    function on_move(e) {
        if (!pick_dragging) return;
        var pt = e.touches ? e.touches[0] : e;
        var mm = pick_client_to_mm(pt.clientX, pt.clientY);
        pick_place_cursor(mm.x, mm.y);
        e.preventDefault();
    }
    function on_up() {
        if (!pick_dragging) return;
        pick_dragging = false;
        pick_cursor_pending = true;
        window.removeEventListener("mousemove", on_move);
        window.removeEventListener("mouseup", on_up);
    }
    function on_down(e) {
        pick_dragging = true;
        var pt = e.touches ? e.touches[0] : e;
        var mm = pick_client_to_mm(pt.clientX, pt.clientY);
        pick_place_cursor(mm.x, mm.y);
        window.addEventListener("mousemove", on_move);
        window.addEventListener("mouseup", on_up);
        e.preventDefault();
    }

    map.addEventListener("mousedown", on_down);
    map.addEventListener("touchstart", on_down, { passive: false });
    map.addEventListener("touchmove", on_move, { passive: false });
    map.addEventListener("touchend", on_up);
    map.addEventListener("touchcancel", on_up);
}

function settings_fill_select(id, values, suffix) {
    var sel = pick_el(id);
    if (!sel) return;
    sel.innerHTML = "";
    values.forEach(function (v) {
        var o = document.createElement("option");
        o.value = String(v);
        o.textContent = v + suffix;
        sel.appendChild(o);
    });
}

function settings_update_displays() {
    var power = pick_el("set_power_pct");
    var speed = pick_el("set_speed_pct");
    var powerDisp = pick_el("set_power_disp");
    var speedDisp = pick_el("set_speed_disp");
    if (power && powerDisp) powerDisp.textContent = power.value + "%";
    if (speed && speedDisp) speedDisp.textContent = speed.value + "%";
}

function settings_apply_from_status(st) {
    if (!st) return;
    if (!settings_status_boot || !settings_form_dirty) {
        var power = pick_el("set_power_pct");
        var speed = pick_el("set_speed_pct");
        var jog = pick_el("jog_step_mm");
        if (power) power.value = String(st.power_pct);
        if (speed) speed.value = String(st.speed_pct);
        if (jog) {
            var step = Math.round(st.jog_step_mm || 1);
            if (step >= 1 && step <= 5) jog.value = String(step);
        }
        settings_update_displays();
    }
    settings_status_boot = true;
}

function settings_log_applied(j) {
    if (!j) return;
    var parts = [];
    if (j.power_pct != null) parts.push("power " + j.power_pct + "%");
    if (j.speed_pct != null) parts.push("speed " + j.speed_pct + "%");
    if (j.jog_step_mm != null) parts.push("jog " + j.jog_step_mm + "mm");
    if (parts.length) log_append(">> settings " + parts.join(", "));
}

function settings_post_field(body) {
    return fetch("/settings", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(body)
    }).then(function (r) {
        if (!r.ok) throw new Error(r.statusText);
        return r.json();
    }).then(function (j) {
        settings_log_applied(j);
        return j;
    });
}

function settings_post_all() {
    var body = {
        power_pct: parseInt(pick_el("set_power_pct").value, 10),
        speed_pct: parseInt(pick_el("set_speed_pct").value, 10),
        jog_step_mm: parseInt(pick_el("jog_step_mm").value, 10),
        apply_cnc: true
    };
    return settings_post_field(body).then(function (j) {
        settings_form_dirty = false;
        if (j.power_pct != null) pick_el("set_power_pct").value = String(j.power_pct);
        if (j.speed_pct != null) pick_el("set_speed_pct").value = String(j.speed_pct);
        settings_update_displays();
        return j;
    });
}

function pick_status_el() {
    return pick_el("pick_status") || pick_el("settings_status");
}

function pick_confirm() {
    var st = pick_status_el();
    if (st) st.textContent = "移动中…";
    return fetch("/pick", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ x_mm: pick_cursor_x, y_mm: pick_cursor_y })
    }).then(function (r) {
        if (!r.ok) throw new Error(r.statusText);
        return r.json();
    }).then(function (j) {
        pick_cursor_pending = false;
        log_append(">> pick move (" + pick_cursor_x.toFixed(1) + ", " + pick_cursor_y.toFixed(1) + ")");
        if (st) st.textContent = "已设定起点并移动";
        return j;
    }).catch(function (e) {
        if (st) st.textContent = "失败: " + e.message;
    });
}

var cnc_transport_busy = false;

function cnc_post_action(path, label) {
    if (cnc_transport_busy) return Promise.resolve();
    cnc_transport_busy = true;
    return fetch(path, { method: "POST" })
        .then(function (r) {
            if (!r.ok) throw new Error(r.statusText);
            return r.json();
        })
        .then(function (j) {
            log_append(">> " + label + (j.action ? " (" + j.action + ")" : ""));
            poll_status();
            return j;
        })
        .catch(function (e) {
            log_append("!! " + label + ": " + e.message);
        })
        .finally(function () {
            setTimeout(function () { cnc_transport_busy = false; }, 450);
        });
}

function cnc_run() {
    return cnc_post_action("/run", "run");
}

function cnc_pause() {
    return cnc_post_action("/pause", "pause");
}

function pick_reset() {
    var st = pick_status_el();
    if (st) st.textContent = "复位中…";
    return fetch("/pick", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ reset: true })
    }).then(function () {
        pick_cursor_pending = false;
        log_append(">> pick reset (home)");
        if (st) st.textContent = "已复位";
    }).catch(function (e) {
        if (st) st.textContent = "复位失败: " + e.message;
    });
}

function settings_init() {
    var powerVals = [];
    for (var p = 10; p <= 100; p += 10) powerVals.push(p);
    var speedVals = [];
    for (var s = 50; s <= 300; s += 50) speedVals.push(s);
    settings_fill_select("set_power_pct", powerVals, "%");
    settings_fill_select("set_speed_pct", speedVals, "%");
    settings_update_displays();

    pick_bind_drag();
    pick_place_cursor(0, 0);
    pick_place_head(0, 0);

    var applyBtn = pick_el("settings_apply_btn");
    if (applyBtn) {
        applyBtn.addEventListener("click", function () {
            var statusEl = pick_el("settings_status");
            if (statusEl) statusEl.textContent = "应用中…";
            settings_post_all()
                .then(function (j) {
                    if (statusEl) {
                        statusEl.textContent = "已应用并下发 CNC — 功率 " + j.power_pct + "% 速度 " + j.speed_pct + "%";
                    }
                })
                .catch(function (e) {
                    if (statusEl) statusEl.textContent = "失败: " + e.message;
                });
        });
    }
    var confirmBtn = pick_el("pick_confirm_btn");
    if (confirmBtn) confirmBtn.addEventListener("click", pick_confirm);
    var resetBtn = pick_el("pick_reset_btn");
    if (resetBtn) resetBtn.addEventListener("click", pick_reset);

    function bind_setting_select(id, field) {
        var sel = pick_el(id);
        if (!sel) return;
        sel.addEventListener("change", function () {
            settings_form_dirty = true;
            settings_update_displays();
            var body = {};
            body[field] = parseInt(sel.value, 10);
            settings_post_field(body)
                .then(function (j) {
                    settings_form_dirty = false;
                    if (j.power_pct != null) pick_el("set_power_pct").value = String(j.power_pct);
                    if (j.speed_pct != null) pick_el("set_speed_pct").value = String(j.speed_pct);
                    settings_update_displays();
                })
                .catch(function () {});
        });
    }
    bind_setting_select("set_power_pct", "power_pct");
    bind_setting_select("set_speed_pct", "speed_pct");

    var jogSel = pick_el("jog_step_mm");
    if (jogSel) {
        jogSel.addEventListener("change", function () {
            fetch("/settings", {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify({ jog_step_mm: parseInt(jogSel.value, 10) })
            }).catch(function () {});
        });
    }
}

document.addEventListener("DOMContentLoaded", function () {
    document.querySelectorAll("nav.tabs button").forEach(function (btn) {
        btn.addEventListener("click", function () { show_tab(btn.dataset.tab); });
    });
    $("send_btn").addEventListener("click", SendCustomCommand);
    $("custom_cmd_txt").addEventListener("keyup", function (e) {
        if (e.key === "Enter") SendCustomCommand();
    });
    var runBtn = $("cnc_run_btn");
    var pauseBtn = $("cnc_pause_btn");
    if (runBtn) runBtn.addEventListener("click", cnc_run);
    if (pauseBtn) pauseBtn.addEventListener("click", cnc_pause);
    document.querySelectorAll("[data-jog]").forEach(function (btn) {
        btn.addEventListener("click", function () {
            var code = btn.getAttribute("data-jog");
            var axis = code.charAt(0);
            var sign = code.indexOf("-") >= 0 ? -1 : 1;
            SendJog(axis, sign);
        });
    });
    var homeBtn = $("jog_home");
    if (homeBtn) {
        homeBtn.addEventListener("click", function () {
            if (typeof pick_reset === "function") {
                pick_reset();
            } else {
                http_command("$H");
            }
        });
    }
    settings_init();
    InitUI();
});
