/** G-code toolpath preview — 42×42 mm work area, matches ui_gcode_preview.cc */
var GCODE_PREVIEW_WORK_MM = 42;

function gcode_preview_stripComments(line) {
    var semi = line.indexOf(";");
    if (semi >= 0) {
        line = line.substring(0, semi);
    }
    line = line.replace(/\([^)]*\)/g, "");
    return line.trim();
}

function gcode_preview_parse(text) {
    var segments = [];
    var curX = 0;
    var curY = 0;
    var absMode = true;
    var laserOn = false;
    var lines = String(text || "").split(/\r?\n/);

    for (var li = 0; li < lines.length; li++) {
        var raw = gcode_preview_stripComments(lines[li]);
        if (!raw) {
            continue;
        }
        var upper = raw.toUpperCase();
        var tokens = upper.split(/\s+/);
        var isG0 = false;
        var isG1 = false;
        var hasX = false;
        var hasY = false;
        var xVal = 0;
        var yVal = 0;

        for (var ti = 0; ti < tokens.length; ti++) {
            var t = tokens[ti];
            if (t === "G90") {
                absMode = true;
            } else if (t === "G91") {
                absMode = false;
            } else if (t === "M3" || t === "M03") {
                laserOn = true;
            } else if (t === "M5" || t === "M05") {
                laserOn = false;
            } else if (t === "G0" || t === "G00") {
                isG0 = true;
            } else if (t === "G1" || t === "G01") {
                isG1 = true;
            } else if (t.charAt(0) === "X") {
                hasX = true;
                xVal = parseFloat(t.substring(1));
            } else if (t.charAt(0) === "Y") {
                hasY = true;
                yVal = parseFloat(t.substring(1));
            }
        }

        if (!isG0 && !isG1) {
            continue;
        }

        var nx = hasX ? (absMode ? xVal : curX + xVal) : curX;
        var ny = hasY ? (absMode ? yVal : curY + yVal) : curY;
        if (Math.abs(nx - curX) < 1e-6 && Math.abs(ny - curY) < 1e-6) {
            continue;
        }

        var kind = (isG1 && !isG0 && laserOn) ? "engrave" : "travel";
        segments.push({ x0: curX, y0: curY, x1: nx, y1: ny, kind: kind });
        curX = nx;
        curY = ny;
    }
    return segments;
}

function gcode_preview_mmToCanvas(mmX, mmY, w, h) {
    return {
        x: (mmX / GCODE_PREVIEW_WORK_MM) * w,
        y: (1 - mmY / GCODE_PREVIEW_WORK_MM) * h
    };
}

function gcode_preview_drawLine(ctx, x0, y0, x1, y1, color, width) {
    ctx.strokeStyle = color;
    ctx.lineWidth = width;
    ctx.lineCap = "round";
    ctx.beginPath();
    ctx.moveTo(x0, y0);
    ctx.lineTo(x1, y1);
    ctx.stroke();
}

/**
 * Render parsed segments onto canvas.
 * @returns {{ ok: boolean, segments: number, error?: string }}
 */
function gcode_preview_render(canvas, text) {
    if (!canvas || !canvas.getContext) {
        return { ok: false, segments: 0, error: "canvas unavailable" };
    }
    var ctx = canvas.getContext("2d");
    var w = canvas.width;
    var h = canvas.height;
    var segments = gcode_preview_parse(text);
    if (!segments.length) {
        ctx.fillStyle = "#E8F4F2";
        ctx.fillRect(0, 0, w, h);
        return { ok: false, segments: 0, error: "未找到可绘制的 G0/G1 刀路" };
    }

    ctx.fillStyle = "#E8F4F2";
    ctx.fillRect(0, 0, w, h);
    ctx.strokeStyle = "#B0BEC5";
    ctx.lineWidth = 1;
    ctx.strokeRect(0.5, 0.5, w - 1, h - 1);

    for (var i = 0; i < segments.length; i++) {
        var s = segments[i];
        var p0 = gcode_preview_mmToCanvas(s.x0, s.y0, w, h);
        var p1 = gcode_preview_mmToCanvas(s.x1, s.y1, w, h);
        var color = s.kind === "engrave" ? "#1A3A4A" : "#B0BEC5";
        var lw = s.kind === "engrave" ? 2 : 1;
        gcode_preview_drawLine(ctx, p0.x, p0.y, p1.x, p1.y, color, lw);
    }
    return { ok: true, segments: segments.length };
}
