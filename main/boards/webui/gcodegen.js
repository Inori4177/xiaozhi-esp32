var gcodegen_image = null;
var gcodegen_last_blob = null;
var gcodegen_last_filename = "";
/** 与固件 WEBUI_GCODEGEN_MAX_BYTES 一致 */
var GCODEGEN_MAX_BYTES = 120 * 1024;

function gcodegen_el(name) { return document.getElementById(name); }
function gcodegen_setStatus(message, isError) {
    var el = gcodegen_el("gcodegen_status");
    if (!el) return;
    el.style.color = isError ? "#e88" : "#9ab";
    el.textContent = message;
}
function gcodegen_onModeChange() {
    var mode = gcodegen_el("gcodegen_mode").value;
    gcodegen_el("gcodegen_image_group").classList.toggle("hide_it", mode !== "image");
    gcodegen_el("gcodegen_text_group").classList.toggle("hide_it", mode !== "text");
}
function gcodegen_onImageSelected(event) {
    var file = event && event.target && event.target.files ? event.target.files[0] : null;
    if (!file) return;
    var img = new Image();
    img.onload = function () { gcodegen_image = img; gcodegen_setStatus("Image: " + file.name, false); gcodegen_preview(); };
    img.onerror = function () { gcodegen_setStatus("Failed to decode image.", true); };
    img.src = URL.createObjectURL(file);
}
function gcodegen_renderTextToCanvas() {
    var text = gcodegen_el("gcodegen_text_input").value || "";
    if (!text.trim()) return null;
    var fontSize = parseFloat(gcodegen_el("gcodegen_font_size").value || "72");
    var lineHeight = 1.2;
    var lines = text.split(/\r?\n/);
    var canvas = gcodegen_el("gcodegen_work_canvas"), ctx = canvas.getContext("2d");
    ctx.font = fontSize + "px sans-serif";
    var maxW = 0;
    for (var i = 0; i < lines.length; i++) {
        var mw = ctx.measureText(lines[i]).width;
        if (mw > maxW) maxW = mw;
    }
    canvas.width = Math.max(1, Math.ceil(maxW));
    canvas.height = Math.max(1, Math.ceil(lines.length * fontSize * lineHeight));
    ctx.fillStyle = "#ffffff"; ctx.fillRect(0, 0, canvas.width, canvas.height);
    ctx.fillStyle = "#000000"; ctx.font = fontSize + "px sans-serif"; ctx.textBaseline = "top";
    for (var j = 0; j < lines.length; j++) ctx.fillText(lines[j], 0, j * fontSize * lineHeight);
    return canvas;
}
function gcodegen_prepareSourceCanvas() {
    var mode = gcodegen_el("gcodegen_mode").value;
    var canvas = gcodegen_el("gcodegen_work_canvas"), ctx = canvas.getContext("2d");
    var maxDim = 1000;
    if (mode === "image") {
        if (!gcodegen_image) return null;
        var scale = Math.min(maxDim / gcodegen_image.width, maxDim / gcodegen_image.height, 1);
        canvas.width = Math.max(1, Math.round(gcodegen_image.width * scale));
        canvas.height = Math.max(1, Math.round(gcodegen_image.height * scale));
        ctx.fillStyle = "#ffffff"; ctx.fillRect(0, 0, canvas.width, canvas.height);
        ctx.drawImage(gcodegen_image, 0, 0, canvas.width, canvas.height);
        return canvas;
    }
    return gcodegen_renderTextToCanvas();
}
function gcodegen_binarize(imageData, threshold, invert) {
    var w = imageData.width, h = imageData.height, data = imageData.data;
    var map = new Uint8Array(w * h);
    for (var y = 0; y < h; y++) for (var x = 0; x < w; x++) {
        var i = (y * w + x) * 4;
        var gray = (0.299 * data[i] + 0.587 * data[i + 1] + 0.114 * data[i + 2]) | 0;
        var black = gray < threshold ? 1 : 0;
        if (invert) black = black ? 0 : 1;
        map[y * w + x] = black;
    }
    return { map: map, width: w, height: h };
}
function gcodegen_computePitch(w, h, widthMm) {
    var xPitch = w > 1 ? widthMm / (w - 1) : widthMm;
    return { xPitch: xPitch, yPitch: xPitch };
}
function gcodegen_physicalSize(bin, widthMm) {
    var w = bin.width, h = bin.height;
    var pitch = gcodegen_computePitch(w, h, widthMm);
    return { widthMm: widthMm, heightMm: h > 1 ? (h - 1) * pitch.yPitch : 0, xPitch: pitch.xPitch, yPitch: pitch.yPitch };
}
function gcodegen_drawBinToCanvas(canvas, bin) {
    var w = bin.width, h = bin.height, ctx = canvas.getContext("2d");
    canvas.width = w; canvas.height = h;
    var out = ctx.createImageData(w, h);
    for (var i = 0; i < bin.map.length; i++) {
        var c = bin.map[i] ? 0 : 255;
        out.data[i * 4] = c; out.data[i * 4 + 1] = c; out.data[i * 4 + 2] = c; out.data[i * 4 + 3] = 255;
    }
    ctx.putImageData(out, 0, 0);
    return canvas;
}
function gcodegen_preview() {
    var source = gcodegen_prepareSourceCanvas();
    if (!source) return gcodegen_setStatus("请选择图片或输入文字", true);
    var threshold = parseInt(gcodegen_el("gcodegen_threshold").value || "128", 10);
    var invert = !!gcodegen_el("gcodegen_invert").checked;
    var bin = gcodegen_binarize(source.getContext("2d").getImageData(0, 0, source.width, source.height), threshold, invert);
    var widthMm = parseFloat(gcodegen_el("gcodegen_width_mm").value || "42");
    var phys = gcodegen_physicalSize(bin, widthMm);
    var off = document.createElement("canvas");
    gcodegen_drawBinToCanvas(off, bin);
    var preview = gcodegen_el("gcodegen_preview"), pctx = preview.getContext("2d");
    var scale = Math.min(320 / phys.widthMm, 240 / (phys.heightMm || 1), 1);
    var dispW = Math.max(1, Math.round(phys.widthMm * scale));
    var dispH = Math.max(1, Math.round((phys.heightMm || phys.widthMm) * scale));
    preview.width = dispW; preview.height = dispH;
    pctx.fillStyle = "#fff"; pctx.fillRect(0, 0, dispW, dispH);
    pctx.imageSmoothingEnabled = false;
    pctx.drawImage(off, 0, 0, bin.width, bin.height, 0, 0, dispW, dispH);
    gcodegen_setStatus("Preview " + bin.width + "x" + bin.height + " px", false);
}
function gcodegen_validateParams() {
    var widthMm = parseFloat(gcodegen_el("gcodegen_width_mm").value || "0");
    var power = parseInt(gcodegen_el("gcodegen_power").value || "0", 10);
    var feedWork = parseFloat(gcodegen_el("gcodegen_feed_work").value || "0");
    var feedTravel = parseFloat(gcodegen_el("gcodegen_feed_travel").value || "0");
    if (!(widthMm > 0)) return "Width must be > 0";
    if (!(power > 0)) return "Power must be > 0";
    if (!(feedWork > 0 && feedTravel > 0)) return "Feed rates must be > 0";
    return "";
}
function gcodegen_rasterToGcode(bin, cfg) {
    var w = bin.width, h = bin.height, map = bin.map;
    var pitch = gcodegen_computePitch(w, h, cfg.widthMm);
    var yPitch = pitch.yPitch;
    var heightMm = h > 1 ? (h - 1) * yPitch : 0;
    var lines = ["; Xiaozhi WebUI gcodegen", "G21", "G90", "M5", "G0 X0 Y0 F" + cfg.feedTravel];
    for (var row = 0; row < h; row++) {
        var y = ((h - 1 - row) * yPitch).toFixed(3);
        var leftToRight = row % 2 === 0;
        var xStart = leftToRight ? 0 : w - 1, xEnd = leftToRight ? w : -1, step = leftToRight ? 1 : -1, x = xStart;
        while (x !== xEnd) {
            while (x !== xEnd && map[row * w + x] === 0) x += step;
            if (x === xEnd) break;
            var runStart = x;
            while (x !== xEnd && map[row * w + x] === 1) x += step;
            var runEnd = x - step;
            var x1 = (runStart * pitch.xPitch).toFixed(3), x2 = (runEnd * pitch.xPitch).toFixed(3);
            lines.push("G0 X" + x1 + " Y" + y + " F" + cfg.feedTravel);
            lines.push("M3 S" + cfg.power);
            lines.push("G1 X" + x2 + " Y" + y + " F" + cfg.feedWork);
            lines.push("M5");
        }
    }
    lines.push("G0 X0 Y0 F" + cfg.feedTravel); lines.push("M5");
    return lines.join("\n") + "\n";
}
function gcodegen_generateBlob() {
    var err = gcodegen_validateParams();
    if (err) return gcodegen_setStatus(err, true), null;
    var source = gcodegen_prepareSourceCanvas();
    if (!source) return gcodegen_setStatus("请选择图片或输入文字", true), null;
    var threshold = parseInt(gcodegen_el("gcodegen_threshold").value || "128", 10);
    var invert = !!gcodegen_el("gcodegen_invert").checked;
    var bin = gcodegen_binarize(source.getContext("2d").getImageData(0, 0, source.width, source.height), threshold, invert);
    var gcode = gcodegen_rasterToGcode(bin, {
        widthMm: parseFloat(gcodegen_el("gcodegen_width_mm").value),
        power: parseInt(gcodegen_el("gcodegen_power").value, 10),
        feedWork: parseFloat(gcodegen_el("gcodegen_feed_work").value),
        feedTravel: parseFloat(gcodegen_el("gcodegen_feed_travel").value)
    });
    var filename = (gcodegen_el("gcodegen_filename").value || "image_engrave.gcode").trim();
    if (!/\.g(code|co)?$/i.test(filename)) filename += ".gcode";
    gcodegen_last_blob = new Blob([gcode], { type: "text/plain" });
    gcodegen_last_filename = filename;
    if (gcodegen_last_blob.size > GCODEGEN_MAX_BYTES) {
        gcodegen_setStatus(
            "G-code 过大 (" + gcodegen_last_blob.size + " B)，上限 " + GCODEGEN_MAX_BYTES +
            " B。请减小宽度或降低图片分辨率。",
            true
        );
        gcodegen_last_blob = null;
        return null;
    }
    gcodegen_setStatus("已生成 " + filename + " (" + gcodegen_last_blob.size + " B)", false);
    return { blob: gcodegen_last_blob, filename: gcodegen_last_filename };
}
function gcodegen_generateAndDownload() {
    var res = gcodegen_generateBlob(); if (!res) return;
    var a = document.createElement("a");
    a.href = URL.createObjectURL(res.blob); a.download = res.filename;
    document.body.appendChild(a); a.click();
    setTimeout(function () { URL.revokeObjectURL(a.href); document.body.removeChild(a); }, 500);
}
function gcodegen_uploadFallback(blob, filename) {
    var path = (typeof files_currentPath === "string" && files_currentPath.length) ? files_currentPath : "/";
    var dest = path + filename;
    if (typeof files_upload_blob === "function") {
        return files_upload_blob(blob, dest);
    }
    var rel = dest.replace(/^\/+/, "");
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
async function gcodegen_uploadOnly() {
    try {
        var res = gcodegen_last_blob ? { blob: gcodegen_last_blob, filename: gcodegen_last_filename } : gcodegen_generateBlob();
        if (!res) return;
        gcodegen_setStatus("Uploading...", false);
        await gcodegen_uploadFallback(res.blob, res.filename);
        gcodegen_setStatus("Upload complete", false);
        if (typeof files_refreshFiles === "function") files_refreshFiles(files_currentPath);
    } catch (err) {
        gcodegen_setStatus("Upload failed: " + err.message, true);
    }
}
async function gcodegen_runNow() {
    try {
        await gcodegen_uploadOnly();
        var filename = (gcodegen_el("gcodegen_filename").value || "image_engrave.gcode").trim();
        if (!/\.g(code|co)?$/i.test(filename)) filename += ".gcode";
        var path = files_currentPath || "/";
        if (!path.endsWith("/")) path += "/";
        var cmd = "[ESP700]" + path + filename;
        gcodegen_el("custom_cmd_txt").value = cmd;
        SendCustomCommand();
        gcodegen_setStatus("Run: " + cmd, false);
    } catch (err) {
        gcodegen_setStatus("Run failed: " + err.message, true);
    }
}
