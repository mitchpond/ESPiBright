#include "Display.h"
#include <WiFi.h>

static auto& D = M5.Display;

// ── Color helpers ─────────────────────────────────────────────────────────────

const char* Display::rgbColorName(uint8_t c) {
    const char* names[] = {"?","BLUE","GREEN","WHITE","RED","ORANGE","PURPLE","PINK","YELLOW","RAINBOW"};
    return (c >= 1 && c <= 9) ? names[c] : names[0];
}

uint16_t Display::rgbPresetTft(uint8_t c) {
    const uint16_t cols[] = {0x4208, 0x001F, 0x07E0, 0xFFFF, 0xF800,
                              0xFCA0, 0x780F, 0xFE19, 0xFFE0, 0x0000};
    return (c >= 1 && c <= 8) ? cols[c] : 0x4208;
}

uint16_t Display::rainbowNow() {
    uint8_t h = (millis() / 24) & 0xFF;
    uint8_t s = h / 43, f = (h % 43) * 6;
    uint8_t r, g, b;
    switch (s) {
        case 0:  r=255; g=f;   b=0;   break;
        case 1:  r=255-f; g=255; b=0; break;
        case 2:  r=0; g=255; b=f;     break;
        case 3:  r=0; g=255-f; b=255; break;
        case 4:  r=f; g=0; b=255;     break;
        default: r=255; g=0; b=255-f; break;
    }
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}

uint16_t Display::rgbColorDisplay(uint8_t c, bool on) {
    if (!on) return UI_DIM;
    return (c == 9) ? rainbowNow() : rgbPresetTft(c);
}

const char* Display::speedLabel(uint8_t cyc) {
    if (cyc == 0x01) return "STATIC";
    if (cyc == 0x02) return "3s";
    if (cyc == 0x04) return "4s";
    if (cyc == 0x08) return "5s";
    return "?";
}

// ── Lifecycle ─────────────────────────────────────────────────────────────────

void Display::begin() {
    D.setRotation(3);
    D.setBrightness(180);
    D.setFont(&fonts::Font0);
    dirty_ = true;
}

void Display::drawBootSplash() {
    D.fillScreen(UI_BG);
    uint16_t wave = 0x041F;
    for (int i = 0; i < 6; i++) {
        int cx = 20 + i * 36, cy = (i % 2 == 0) ? 88 : 100;
        D.fillCircle(cx, cy, 7, wave);
        if (i > 0) D.drawLine(20 + (i-1)*36, ((i-1)%2==0)?88:100, cx, cy, wave);
    }
    D.fillCircle(220, 84, 8, wave);
    D.fillCircle(225, 80, 3, wave);
    D.fillCircle(217, 78, 2, 0xFFFF);
    D.setTextDatum(MC_DATUM);
    D.setTextColor(UI_CYAN, UI_BG);
    D.setTextSize(2);
    D.drawString("ESPiBright", D.width()/2, 40);
    D.setTextSize(1);
    D.setTextColor(UI_DIM, UI_BG);
    D.drawString("for OptiBright", D.width()/2, 62);
    D.setTextColor(0x780F, UI_BG);
    D.drawString("~ Nesso lives ~", D.width()/2, 120);
    delay(2200);
}

void Display::drawConnecting() {
    D.fillScreen(UI_BG);
    D.setTextDatum(MC_DATUM);
    D.setTextColor(UI_DIM, UI_BG);
    D.setTextSize(1);
    D.drawString("Connecting to", D.width()/2, 40);
    D.setTextColor(UI_BRIGHT, UI_BG);
    D.setTextSize(2);
    D.drawString(WIFI_SSID, D.width()/2, 62);
    D.setTextSize(1);
    D.setTextColor(0x780F, UI_BG);
    D.drawString("~ Nesso ~", D.width()/2, 90);
}

// ── Main update loop ──────────────────────────────────────────────────────────

void Display::update() {
    uint32_t now = millis();

    // IMU orientation check every 800ms
    if (now - imuMs_ > 800) {
        imuMs_ = now;
        checkOrientation_();
    }

    handleButtons_();

    // Rainbow needs continuous swatch refresh
    if (page_ == PAGE_LIVE && ch_.rgbOn && ch_.rgbColor == 9) {
        if (now - tickMs_ > 80) { tickMs_ = now; dirty_ = true; }
    }

    if (dirty_) {
        redraw_();
    }

    // Header refresh every second (time, battery, TX dot)
    if (!dirty_ && now - tickMs_ > 1000) {
        tickMs_ = now;
        refreshHeader_();
        if (page_ == PAGE_SYSTEM || page_ == PAGE_SCHEDULE) dirty_ = true;
    }
}

// ── Orientation ───────────────────────────────────────────────────────────────

void Display::checkOrientation_() {
    float ax, ay, az;
    if (!M5.Imu.getAccel(&ax, &ay, &az)) return;
    bool wantLandscape = fabsf(ay) > fabsf(ax);
    // Rotation: landscape ay>0→3, ay<0→1; portrait ax>0→2, ax<0→0
    uint8_t wantRotation = wantLandscape ? (ay > 0 ? 3 : 1) : (ax > 0 ? 2 : 0);
    if (wantRotation != rotation_) {
        rotation_  = wantRotation;
        landscape_ = wantLandscape;
        D.setRotation(wantRotation);
        dirty_ = true;
    }
}

// ── Button handling ───────────────────────────────────────────────────────────

void Display::handleButtons_() {
    // BtnA = green face button → context action for current page
    // BtnB = side button       → cycle pages
    // BtnC = not usable
    if (M5.BtnA.wasClicked()) {
        switch (page_) {
            case PAGE_LIVE:     ch_.send();           dirty_ = true; break;
            case PAGE_SCHEDULE: sched_.send();        dirty_ = true; break;
            case PAGE_SYSTEM:   clock_.send("SYNC");  dirty_ = true; break;
            default: break;
        }
    }
    if (M5.BtnA.wasHold()) {
        if (page_ == PAGE_SYSTEM) ESP.restart();
    }
    if (M5.BtnB.wasClicked()) {
        page_ = (UIPage)((page_ + 1) % PAGE_COUNT);
        dirty_ = true;
    }
}

// ── Redraw dispatcher ─────────────────────────────────────────────────────────

void Display::redraw_() {
    D.startWrite();
    switch (page_) {
        case PAGE_LIVE:     drawPageLive_();     break;
        case PAGE_SCHEDULE: drawPageSchedule_(); break;
        case PAGE_SYSTEM:   drawPageSystem_();   break;
        default: break;
    }
    D.endWrite();
    dirty_ = false;
}

void Display::refreshHeader_() {
    bool txNow = txLit_ && millis() < txUntil_;
    if (!txNow && txLit_) txLit_ = false;
    const char* titles[] = {"ESPiBright", "Schedule", "System"};
    D.startWrite();
    drawHeader_(titles[page_]);
    D.endWrite();
}

// ── Shared chrome ─────────────────────────────────────────────────────────────

void Display::drawHeader_(const char* title) {
    D.fillRect(0, 0, D.width(), HDR_H, UI_HDR);
    D.setTextDatum(ML_DATUM);
    D.setTextColor(UI_CYAN, UI_HDR);
    D.setTextSize(1);
    D.drawString(title, 4, HDR_H/2);

    char tbuf[8];
    snprintf(tbuf, sizeof(tbuf), "%02d:%02d", clock_.hh, clock_.mm);
    D.setTextDatum(MC_DATUM);
    D.setTextColor(UI_BRIGHT, UI_HDR);
    D.drawString(tbuf, D.width()/2, HDR_H/2);

    // TX dot
    uint16_t dotCol = (txLit_ && millis() < txUntil_) ? UI_GREEN : UI_DIM;
    D.fillCircle(D.width() - 6, HDR_H/2, 3, dotCol);

    // Battery
    int8_t bat = M5.Power.getBatteryLevel();
    char bbuf[8];
    snprintf(bbuf, sizeof(bbuf), "%d%%", bat);
    D.setTextDatum(MR_DATUM);
    D.setTextColor(bat > 20 ? UI_DIM : UI_RED, UI_HDR);
    D.drawString(bbuf, D.width() - 14, HDR_H/2);
}

void Display::drawFooter_(const char* a, const char* b, const char* c) {
    int fy = D.height() - FTR_H;
    D.fillRect(0, fy, D.width(), FTR_H, UI_HDR);
    D.setTextSize(1);
    D.setTextColor(UI_DIM, UI_HDR);
    D.setTextDatum(ML_DATUM); D.drawString(a, 4,         fy + FTR_H/2);
    D.setTextDatum(MC_DATUM); D.drawString(b, D.width()/2,   fy + FTR_H/2);
    D.setTextDatum(MR_DATUM); D.drawString(c, D.width() - 4, fy + FTR_H/2);
}

void Display::drawLevelBar_(int x, int y, uint8_t lvl, bool on, uint16_t litCol) {
    const int SEG = 5, GAP = 2, H = 8, N = 10;
    for (int i = 0; i < N; i++) {
        int sx = x + i * (SEG + GAP);
        D.fillRect(sx, y, SEG, H, (on && i < (int)lvl) ? litCol : UI_HDR);
    }
}

void Display::drawRssiBars_(int x, int y, int rssi) {
    int strength = (rssi > -60) ? 4 : (rssi > -70) ? 3 : (rssi > -80) ? 2 : 1;
    for (int i = 0; i < 4; i++) {
        int bh = (i + 1) * 3;
        D.fillRect(x + i*5, y - bh, 3, bh, (i < strength) ? UI_GREEN : UI_DIM);
    }
}

void Display::drawSchedSlot_(int x, int y, int w,
                              bool active, uint8_t hh, uint8_t mm,
                              uint16_t dotCol) {
    if (!active) {
        D.setTextColor(UI_DIM, UI_BG);
        D.setTextDatum(MC_DATUM);
        D.setTextSize(1);
        D.drawString("--:--", x + w/2, y + 8);
        return;
    }
    char tbuf[8];
    snprintf(tbuf, sizeof(tbuf), "%02d:%02d", hh, mm);
    D.fillCircle(x + 8, y + 8, 3, dotCol);
    D.setTextColor(UI_BRIGHT, UI_BG);
    D.setTextDatum(ML_DATUM);
    D.setTextSize(1);
    D.drawString(tbuf, x + 14, y + 8);
}

// ── Page: Live ────────────────────────────────────────────────────────────────

void Display::drawPageLive_() {
    int cw = D.width() / 3;
    int cy = HDR_H;
    int ch = D.height() - HDR_H - FTR_H;

    D.fillRect(0, cy, D.width(), ch, UI_BG);
    D.drawFastVLine(cw,     cy, ch, UI_HDR);
    D.drawFastVLine(cw * 2, cy, ch, UI_HDR);

    // White
    {
        int cx = 0;
        D.fillCircle(cx + 8, cy + 16, 4, ch_.whiteOn ? UI_W_COL : UI_DIM);
        D.setTextDatum(ML_DATUM);
        D.setTextColor(ch_.whiteOn ? UI_BRIGHT : UI_DIM, UI_BG);
        D.setTextSize(1);
        D.drawString("WHITE", cx + 16, cy + 16);
        drawLevelBar_(cx + 5, cy + 30, ch_.whiteLevel, ch_.whiteOn, UI_W_COL);
        char sbuf[12];
        snprintf(sbuf, sizeof(sbuf), "%s Lv.%d", ch_.whiteOn ? "ON" : "OFF", ch_.whiteLevel);
        D.setTextDatum(MC_DATUM);
        D.setTextColor(ch_.whiteOn ? UI_BRIGHT : UI_DIM, UI_BG);
        D.drawString(sbuf, cx + cw/2, cy + 46);
    }

    // Blue
    {
        int cx = cw;
        D.fillCircle(cx + 8, cy + 16, 4, ch_.blueOn ? UI_B_COL : UI_DIM);
        D.setTextDatum(ML_DATUM);
        D.setTextColor(ch_.blueOn ? UI_BRIGHT : UI_DIM, UI_BG);
        D.setTextSize(1);
        D.drawString("BLUE", cx + 16, cy + 16);
        drawLevelBar_(cx + 5, cy + 30, ch_.blueLevel, ch_.blueOn, UI_B_COL);
        char sbuf[12];
        snprintf(sbuf, sizeof(sbuf), "%s Lv.%d", ch_.blueOn ? "ON" : "OFF", ch_.blueLevel);
        D.setTextDatum(MC_DATUM);
        D.setTextColor(ch_.blueOn ? UI_BRIGHT : UI_DIM, UI_BG);
        D.drawString(sbuf, cx + cw/2, cy + 46);
    }

    // RGB
    {
        int cx = cw * 2;
        uint16_t swatch = rgbColorDisplay(ch_.rgbColor, ch_.rgbOn);
        D.fillCircle(cx + 8, cy + 12, 4, swatch);
        D.setTextDatum(ML_DATUM);
        D.setTextColor(ch_.rgbOn ? UI_BRIGHT : UI_DIM, UI_BG);
        D.setTextSize(1);
        D.drawString("RGB", cx + 16, cy + 12);
        D.fillRoundRect(cx + 6, cy + 24, cw - 12, 14, 3, swatch);
        if (!ch_.rgbOn) D.drawRoundRect(cx + 6, cy + 24, cw - 12, 14, 3, UI_DIM);
        D.setTextDatum(MC_DATUM);
        D.setTextColor((swatch < 0x4000) ? UI_BRIGHT : UI_BG, swatch);
        D.drawString(rgbColorName(ch_.rgbColor), cx + cw/2, cy + 31);
        drawLevelBar_(cx + 5, cy + 42, ch_.rgbLevel, ch_.rgbOn, swatch);
        D.setTextColor(ch_.rgbOn ? UI_BRIGHT : UI_DIM, UI_BG);
        D.setTextDatum(MC_DATUM);
        char sbuf[12];
        snprintf(sbuf, sizeof(sbuf), "%s Lv.%d", ch_.rgbOn ? "ON" : "OFF", ch_.rgbLevel);
        D.drawString(sbuf, cx + cw/2, cy + 56);
        if (ch_.rgbOn && ch_.rgbCycle != 0x01) {
            char sp[12];
            snprintf(sp, sizeof(sp), "%s cyc", speedLabel(ch_.rgbCycle));
            D.setTextColor(UI_DIM, UI_BG);
            D.drawString(sp, cx + cw/2, cy + 68);
        }
    }

    drawHeader_("ESPiBright");
    drawFooter_("[A] send", "[B] page", "");
}

// ── Page: Schedule ────────────────────────────────────────────────────────────

void Display::drawPageSchedule_() {
    int cy = HDR_H;
    D.fillRect(0, cy, D.width(), D.height() - HDR_H - FTR_H, UI_BG);

    const int ON_X = 52, OFF_X = 142, ROW_H = 18;

    D.setTextSize(1);
    D.setTextDatum(MC_DATUM);
    D.setTextColor(UI_GREEN,  UI_BG); D.drawString("ON",  ON_X  + 45, cy + 8);
    D.setTextColor(UI_ORANGE, UI_BG); D.drawString("OFF", OFF_X + 49, cy + 8);
    D.drawFastHLine(0, cy + 16, D.width(), UI_HDR);

    const int R0 = cy + 20, R1 = cy + 38, R2 = cy + 56;

    // White row
    D.fillCircle(8, R0 + 8, 4, UI_W_COL);
    D.setTextDatum(ML_DATUM); D.setTextColor(UI_BRIGHT, UI_BG);
    D.drawString("White", 16, R0 + 8);
    drawSchedSlot_(ON_X,  R0, 90, sched_.whiteOn.active,  sched_.whiteOn.hh,  sched_.whiteOn.mm,  UI_GREEN);
    drawSchedSlot_(OFF_X, R0, 98, sched_.whiteOff.active, sched_.whiteOff.hh, sched_.whiteOff.mm, UI_ORANGE);

    // Blue row
    D.fillCircle(8, R1 + 8, 4, UI_B_COL);
    D.setTextColor(UI_BRIGHT, UI_BG);
    D.drawString("Blue", 16, R1 + 8);
    drawSchedSlot_(ON_X,  R1, 90, sched_.blueOn.active,  sched_.blueOn.hh,  sched_.blueOn.mm,  UI_GREEN);
    drawSchedSlot_(OFF_X, R1, 98, sched_.blueOff.active, sched_.blueOff.hh, sched_.blueOff.mm, UI_ORANGE);

    // RGB row
    uint16_t rgbDot = ch_.rgbOn ? rgbColorDisplay(ch_.rgbColor, true) : UI_DIM;
    D.fillCircle(8, R2 + 8, 4, rgbDot);
    D.setTextColor(UI_BRIGHT, UI_BG);
    D.drawString("RGB", 16, R2 + 8);
    uint16_t rgbOnCol  = sched_.rgbOn.active  ? rgbPresetTft(sched_.rgbOn.state  & 0x0F) : UI_DIM;
    uint16_t rgbOffCol = sched_.rgbOff.active ? rgbPresetTft(sched_.rgbOff.state & 0x0F) : UI_DIM;
    drawSchedSlot_(ON_X,  R2, 90, sched_.rgbOn.active,  sched_.rgbOn.hh,  sched_.rgbOn.mm,  rgbOnCol);
    drawSchedSlot_(OFF_X, R2, 98, sched_.rgbOff.active, sched_.rgbOff.hh, sched_.rgbOff.mm, rgbOffCol);
    if (sched_.rgbOn.active)
        D.fillRect(ON_X  + 66, R2 + 3, 16, 10, rgbPresetTft(sched_.rgbOn.state  & 0x0F));
    if (sched_.rgbOff.active)
        D.fillRect(OFF_X + 66, R2 + 3, 16, 10, rgbPresetTft(sched_.rgbOff.state & 0x0F));

    D.drawFastHLine(0, R2 + ROW_H, D.width(), UI_HDR);

    // Next event countdown
    const char* nextLbl = nullptr;
    int minsUntil = sched_.minutesUntilNext(clock_.hh, clock_.mm, &nextLbl);
    if (nextLbl && minsUntil >= 0) {
        char nbuf[32];
        int h = minsUntil / 60, m = minsUntil % 60;
        if (h > 0) snprintf(nbuf, sizeof(nbuf), "Next: %s in %dh %dm", nextLbl, h, m);
        else        snprintf(nbuf, sizeof(nbuf), "Next: %s in %dm",     nextLbl, m);
        D.setTextDatum(MC_DATUM);
        D.setTextColor(UI_CYAN, UI_BG);
        D.setTextSize(1);
        D.drawString(nbuf, D.width()/2, R2 + ROW_H + 8);
    }

    drawHeader_("Schedule");
    drawFooter_("[A] send", "[B] page", "");
}

// ── Page: System ──────────────────────────────────────────────────────────────

void Display::drawPageSystem_() {
    int cy = HDR_H;
    D.fillRect(0, cy, D.width(), D.height() - HDR_H - FTR_H, UI_BG);

    D.setTextSize(1);
    const int LH = 14;
    int y = cy + 6;

    // Hostname
    D.setTextDatum(ML_DATUM);
    D.setTextColor(UI_DIM, UI_BG);    D.drawString("Host", 4, y);
    D.setTextColor(UI_CYAN, UI_BG);   D.drawString(hostname.c_str(), 40, y);

    // IP address
    y += LH;
    D.setTextColor(UI_DIM, UI_BG);    D.drawString("IP",   4, y);
    D.setTextColor(UI_CYAN, UI_BG);   D.drawString(ipAddress.c_str(), 40, y);

    // WiFi + RSSI
    y += LH;
    int rssi = WiFi.RSSI();
    D.setTextColor(UI_DIM, UI_BG);    D.drawString("WiFi", 4, y);
    D.setTextColor(UI_BRIGHT, UI_BG); D.drawString(WIFI_SSID, 40, y);
    drawRssiBars_(160, y + 6, rssi);
    char rsbuf[12]; snprintf(rsbuf, sizeof(rsbuf), "%ddBm", rssi);
    D.setTextDatum(MR_DATUM); D.setTextColor(UI_DIM, UI_BG);
    D.drawString(rsbuf, D.width() - 2, y);

    // Battery
    y += LH;
    int8_t bat = M5.Power.getBatteryLevel();
    D.setTextDatum(ML_DATUM);
    D.setTextColor(UI_DIM, UI_BG); D.drawString("Bat", 4, y);
    char batbuf[12]; snprintf(batbuf, sizeof(batbuf), "%d%%", bat);
    D.setTextColor(bat > 20 ? UI_GREEN : UI_RED, UI_BG);
    D.drawString(batbuf, 40, y);
    for (int i = 0; i < 8; i++) {
        uint16_t bc = (i < (bat * 8 / 100)) ? (bat > 20 ? UI_GREEN : UI_RED) : UI_HDR;
        D.fillRect(68 + i*9, y - 4, 7, 9, bc);
    }

    // Uptime
    y += LH;
    uint32_t up = millis() / 1000;
    uint32_t ud = up / 86400, uh = (up % 86400) / 3600, um = (up % 3600) / 60;
    char ubuf[24];
    if (ud > 0) snprintf(ubuf, sizeof(ubuf), "%dd %dh %dm", ud, uh, um);
    else        snprintf(ubuf, sizeof(ubuf), "%dh %dm %ds",  uh, um, (int)(up % 60));
    D.setTextColor(UI_DIM, UI_BG);    D.drawString("Up",   4, y);
    D.setTextColor(UI_BRIGHT, UI_BG); D.drawString(ubuf,  40, y);

    // Heap
    y += LH;
    char hbuf[20]; snprintf(hbuf, sizeof(hbuf), "%dKB free", esp_get_free_heap_size() / 1024);
    D.setTextColor(UI_DIM, UI_BG);    D.drawString("Heap", 4, y);
    D.setTextColor(UI_BRIGHT, UI_BG); D.drawString(hbuf,  40, y);

    // Last TX
    y += LH;
    D.setTextColor(UI_DIM, UI_BG); D.drawString("TX", 4, y);
    char txbuf[32];
    if (rf_.lastMs > 0) {
        uint32_t ago = (millis() - rf_.lastMs) / 1000;
        snprintf(txbuf, sizeof(txbuf), "%-12s %ds ago", rf_.lastLabel, ago);
    } else {
        snprintf(txbuf, sizeof(txbuf), "none");
    }
    D.setTextColor(UI_GREEN, UI_BG); D.drawString(txbuf, 40, y);

    // Clock
    y += LH;
    char clkbuf[12]; snprintf(clkbuf, sizeof(clkbuf), "%02d:%02d:%02d", clock_.hh, clock_.mm, clock_.ss);
    D.setTextColor(UI_DIM, UI_BG);    D.drawString("Clock", 4, y);
    D.setTextColor(UI_BRIGHT, UI_BG); D.drawString(clkbuf, 40, y);

    drawHeader_("System");
    drawFooter_("[A] sync time", "[B] page", "hold=reboot");
}
