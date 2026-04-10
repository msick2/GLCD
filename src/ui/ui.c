#include "ui.h"
#include "draw.h"
#include "sim.h"
#include "lcd.h"

#include <stdio.h>
#include <string.h>

/* Layout constants — match the JS mockup. */
#define TW   22                  /* tab area width */
#define TX   23                  /* content start x */
#define CW   233                 /* content width */
#define LC   26                  /* left content text x */
#define RX   253                 /* right edge x for right-aligned text */
#define HDR  13                  /* header height */
#define TH   ((LCD_HEIGHT - HDR) / 7)   /* tab height = 21 */
#define MX   138                 /* middle vertical x for two-column pages */
#define RC   (MX + 4)            /* right column text x */

#define WHITE LCD_COLOR_WHITE
#define LIGHT LCD_COLOR_LIGHT
#define DARK  LCD_COLOR_DARK
#define BLACK LCD_COLOR_BLACK

static uint16_t row_y(uint8_t i)  { return (uint16_t)(HDR + i * TH + (TH - 7) / 2); }
static uint16_t row_sep(uint8_t i){ return (uint16_t)(HDR + (i + 1) * TH); }

static void sep(uint8_t i) { draw_hline(TX, LCD_WIDTH - 1, row_sep(i), LIGHT); }

/* ----- formatting helpers ----- */

static void fmt_v(char *buf, size_t n, float v, int dec) {
    if (dec == 2) snprintf(buf, n, "%.2fV", (double)v);
    else          snprintf(buf, n, "%.3fV", (double)v);
}
static void fmt_a_signed(char *buf, size_t n, float v) {
    snprintf(buf, n, "%s%.3fA", v >= 0 ? "+" : "", (double)v);
}
static void fmt_a_unsigned_plus(char *buf, size_t n, float v) {
    snprintf(buf, n, "+%.3fA", (double)v);
}
static void fmt_pct(char *buf, size_t n, uint8_t v) {
    snprintf(buf, n, "%u%%", v);
}

/* ----- header + tabs ----- */

static void draw_header(const char *title) {
    lcd_fill_rect(0, 0, LCD_WIDTH, HDR, BLACK);
    draw_text(LC, 3, title, WHITE);
}

static const char  TABS[7] = { 'S', 'C', 'D', 'B', 'L', 'K', 'E' };

static void draw_tabs(uint8_t pg) {
    const int tc = TH / 2 - 3;
    lcd_fill_rect(0, HDR, TW, LCD_HEIGHT - HDR, LIGHT);
    for (uint8_t i = 0; i < 7; ++i) {
        const uint16_t ty = (uint16_t)(HDR + i * TH);
        if (i == pg) {
            lcd_fill_rect(0, ty, TW + 1, TH, BLACK);
            draw_char(7, (uint16_t)(ty + tc), TABS[i], WHITE);
        } else {
            lcd_fill_rect(0, ty, TW, TH, LIGHT);
            draw_hline(0, TW - 1, ty, DARK);
            draw_char(7, (uint16_t)(ty + tc), TABS[i], DARK);
        }
    }
    draw_vline(TW, HDR, LCD_HEIGHT - 1, DARK);
}

/* ----- page 0: SUMMARY ----- */

static void page_summary(const sim_state_t *s) {
    char b[24];
    draw_header("SUMMARY");
    draw_vline(MX, HDR, LCD_HEIGHT - 1, LIGHT);

    snprintf(b, sizeof b, "VIN:%.2fV", (double)s->vi); draw_text(LC, row_y(0), b, BLACK);
    snprintf(b, sizeof b, "IIN:%s%.3fA", s->ii >= 0 ? "+" : "", (double)s->ii); draw_text(RC, row_y(0), b, BLACK);
    sep(0);

    snprintf(b, sizeof b, "VB :%.3fV", (double)s->vb); draw_text(LC, row_y(1), b, BLACK);
    snprintf(b, sizeof b, "IB :%s%.3fA", s->ib >= 0 ? "+" : "", (double)s->ib); draw_text(RC, row_y(1), b, BLACK);
    sep(1);

    snprintf(b, sizeof b, "VL :%.3fV", (double)s->vl); draw_text(LC, row_y(2), b, BLACK);
    snprintf(b, sizeof b, "ID :+%.3fA", (double)s->id); draw_text(RC, row_y(2), b, BLACK);
    sep(2);

    snprintf(b, sizeof b, "SOC:%u%%", s->soc); draw_text(LC, row_y(3), b, BLACK);
    snprintf(b, sizeof b, "SOH:%u%%", s->soh); draw_text(RC, row_y(3), b, BLACK);
    sep(3);

    snprintf(b, sizeof b, "ST :%s", s->name); draw_text(LC, row_y(4), b, BLACK);
    snprintf(b, sizeof b, "C:%u/%u", s->cy, sim_cfg.cycl); draw_text(RC, row_y(4), b, BLACK);
    sep(4);

    snprintf(b, sizeof b, "ELA:%s", s->ela); draw_text(LC, row_y(5), b, BLACK);
    snprintf(b, sizeof b, "ETA:%s", s->eta); draw_text(RC, row_y(5), b, BLACK);
    sep(5);
}

/* ----- page 1: CHARGE ----- */

static void page_charge(const sim_state_t *s) {
    char b[24];
    draw_header("CHARGE");
    draw_vline(MX, HDR, LCD_HEIGHT - 1, LIGHT);

    snprintf(b, sizeof b, "VIN :%.2fV", (double)s->vi); draw_text(LC, row_y(0), b, BLACK);
    snprintf(b, sizeof b, "IIN :%s%.3fA", s->ii >= 0 ? "+" : "", (double)s->ii); draw_text(RC, row_y(0), b, BLACK);
    sep(0);

    snprintf(b, sizeof b, "VBAT:%.3fV", (double)s->vb); draw_text(LC, row_y(1), b, BLACK);
    snprintf(b, sizeof b, "ICHG:%s%.3fA", s->ic >= 0 ? "+" : "", (double)s->ic); draw_text(RC, row_y(1), b, BLACK);
    sep(1);

    snprintf(b, sizeof b, "TGT :%.1fA", (double)(sim_cfg.ich * 0.5f)); draw_text(LC, row_y(2), b, BLACK);
    snprintf(b, sizeof b, "PWM :%u%%", s->pc); draw_text(RC, row_y(2), b, BLACK);
    sep(2);

    snprintf(b, sizeof b, "STAT:%s", s->pc > 0 ? "ACTIVE" : "INACTIVE"); draw_text(LC, row_y(3), b, BLACK);
    sep(3);

    snprintf(b, sizeof b, "VFULL:%.2fV", (double)(sim_cfg.vfl / 100.0f)); draw_text(LC, row_y(4), b, BLACK);
    snprintf(b, sizeof b, "PAK:%.2fV", (double)(sim_cfg.vfl / 100.0f * (sim_cfg.cells + 1))); draw_text(RC, row_y(4), b, BLACK);
    sep(4);

    snprintf(b, sizeof b, "CELLS:%uS", sim_cfg.cells + 1); draw_text(LC, row_y(5), b, BLACK);
    sep(5);
}

/* ----- page 2: DISCHARGE ----- */

static void page_discharge(const sim_state_t *s) {
    char b[24];
    draw_header("DISCHARGE");
    draw_vline(MX, HDR, LCD_HEIGHT - 1, LIGHT);

    snprintf(b, sizeof b, "VBAT :%.3fV", (double)s->vb); draw_text(LC, row_y(0), b, BLACK);
    snprintf(b, sizeof b, "IDCHG:+%.3fA", (double)s->id); draw_text(RC, row_y(0), b, BLACK);
    sep(0);

    snprintf(b, sizeof b, "TGT  :%.1fA", (double)(sim_cfg.idh * 0.5f)); draw_text(LC, row_y(1), b, BLACK);
    snprintf(b, sizeof b, "PWM  :%u%%", s->pd); draw_text(RC, row_y(1), b, BLACK);
    sep(1);

    snprintf(b, sizeof b, "VLOAD:%.3fV", (double)s->vl); draw_text(LC, row_y(2), b, BLACK);
    draw_text(RC, row_y(2), "RLOAD:2.5 OHM", BLACK);
    sep(2);

    snprintf(b, sizeof b, "STAT :%s", s->pd > 0 ? "ACTIVE" : "INACTIVE"); draw_text(LC, row_y(3), b, BLACK);
    sep(3);

    snprintf(b, sizeof b, "VCUT :%.2fV", (double)(sim_cfg.vcu / 100.0f)); draw_text(LC, row_y(4), b, BLACK);
    snprintf(b, sizeof b, "PAK:%.2fV", (double)(sim_cfg.vcu / 100.0f * (sim_cfg.cells + 1))); draw_text(RC, row_y(4), b, BLACK);
    sep(4);

    snprintf(b, sizeof b, "CELLS:%uS", sim_cfg.cells + 1); draw_text(LC, row_y(5), b, BLACK);
    sep(5);
}

/* ----- page 3: BATTERY ----- */

static void page_battery(const sim_state_t *s) {
    char b[24];
    draw_header("BATTERY BQ34Z100");
    draw_vline(MX, HDR, LCD_HEIGHT - 1, LIGHT);

    snprintf(b, sizeof b, "V   :%.3fV", (double)s->vb); draw_text(LC, row_y(0), b, BLACK);
    snprintf(b, sizeof b, "I   :%s%.3fA", s->ib >= 0 ? "+" : "", (double)s->ib); draw_text(RC, row_y(0), b, BLACK);
    sep(0);

    snprintf(b, sizeof b, "SOC :%u%%", s->soc); draw_text(LC, row_y(1), b, BLACK);
    snprintf(b, sizeof b, "SOH :%u%%", s->soh); draw_text(RC, row_y(1), b, BLACK);
    sep(1);

    snprintf(b, sizeof b, "QMAX:%u MAH", s->qm); draw_text(LC, row_y(2), b, BLACK);
    snprintf(b, sizeof b, "CYC :%u/%u", s->cy, sim_cfg.cycl); draw_text(RC, row_y(2), b, BLACK);
    sep(2);

    snprintf(b, sizeof b, "FC:%u  FD:%u", s->fc, s->fd); draw_text(LC, row_y(3), b, BLACK);
    snprintf(b, sizeof b, "IT:%s", s->it ? "Y" : "N"); draw_text(RC, row_y(3), b, BLACK);
    sep(3);

    snprintf(b, sizeof b, "ST  :%s", s->name); draw_text(LC, row_y(4), b, BLACK);
    sep(4);

    snprintf(b, sizeof b, "ELA :%s", s->ela); draw_text(LC, row_y(5), b, BLACK);
    sep(5);
}

/* ----- page 4: LEARN ----- */

static void page_learn(const sim_state_t *s) {
    char b[24];
    draw_header("LEARN");

    /* Row 0: 6 phase boxes (DCC > DCV > RST > CCC > CCV > RST) */
    static const char *phases[6] = { "DCC", "DCV", "RST", "CCC", "CCV", "RST" };
    const uint16_t bh   = 15;
    const uint16_t by   = (uint16_t)(HDR + (TH - bh) / 2);
    const uint16_t ty   = (uint16_t)(by + (bh - 7) / 2);
    const uint16_t sepW = 7;
    const uint16_t boxW = (uint16_t)((RX - LC - sepW * 5) / 6);
    const uint16_t pad  = (uint16_t)((boxW - 18) / 2);

    uint16_t px = LC;
    for (int i = 0; i < 6; ++i) {
        const bool act = (i <= s->ph);
        if (act) {
            lcd_fill_rect(px, by, boxW, bh, BLACK);
            draw_text((uint16_t)(px + pad), ty, phases[i], WHITE);
        } else {
            draw_hline(px, (uint16_t)(px + boxW - 1), by, LIGHT);
            draw_hline(px, (uint16_t)(px + boxW - 1), (uint16_t)(by + bh - 1), LIGHT);
            draw_vline(px, by, (uint16_t)(by + bh - 1), LIGHT);
            draw_vline((uint16_t)(px + boxW - 1), by, (uint16_t)(by + bh - 1), LIGHT);
            draw_text((uint16_t)(px + pad), ty, phases[i], DARK);
        }
        px = (uint16_t)(px + boxW);
        if (i < 5) {
            draw_char((uint16_t)(px + 1), ty, '>', LIGHT);
            px = (uint16_t)(px + sepW);
        }
    }
    sep(0);

    draw_vline(MX, row_sep(0), LCD_HEIGHT - 1, LIGHT);

    snprintf(b, sizeof b, "CYC:%u/%u", s->cy, sim_cfg.cycl); draw_text(LC, row_y(1), b, BLACK);
    snprintf(b, sizeof b, "SOC:%u%%", s->soc); draw_text(RC, row_y(1), b, BLACK);
    sep(1);

    static const char *PN[6] = { "DCC", "DCV", "RST1", "CCC", "CCV", "RST2" };
    const uint8_t ph = s->ph < 6 ? s->ph : 5;
    snprintf(b, sizeof b, "PHASE:%s", PN[ph]); draw_text(LC, row_y(2), b, BLACK);
    sep(2);

    snprintf(b, sizeof b, "ELA:%s", s->ela); draw_text(LC, row_y(3), b, BLACK);
    snprintf(b, sizeof b, "ETA:%s", s->eta); draw_text(RC, row_y(3), b, BLACK);
    sep(3);

    snprintf(b, sizeof b, "REST:%.1fH", (double)(sim_cfg.rest * 0.5f)); draw_text(LC, row_y(4), b, BLACK);
    sep(4);

    snprintf(b, sizeof b, "V:%.3fV", (double)s->vb); draw_text(LC, row_y(5), b, BLACK);
    snprintf(b, sizeof b, "I:%s%.3fA", s->ib >= 0 ? "+" : "", (double)s->ib); draw_text(RC, row_y(5), b, BLACK);
    sep(5);
}

/* ----- page 5: CONTROL ----- */

static const char *OPS[4] = { "STOP", "CHG", "DCHG", "LEARN" };
static const char *OD[4]  = {
    "STOP  : ALL STOP",
    "CHG   : CC/CV CHARGE",
    "DCHG  : BUCK DISCH 2.5OHM",
    "LEARN : AUTO CYCLE",
};

static void page_control(uint8_t cur, uint8_t op, const sim_state_t *s) {
    char b[24];
    draw_header("CONTROL");

    draw_text(LC, row_y(0), "SELECT OPERATION:", DARK);
    sep(0);

    const uint16_t bw   = 105;
    const uint16_t bh   = 26;
    const uint16_t gap  = 6;
    const uint16_t by0  = (uint16_t)(row_sep(0) + 1 + (row_sep(3) - row_sep(0) - 1 - bh * 2 - gap) / 2);

    for (int i = 0; i < 4; ++i) {
        const uint16_t bx  = (uint16_t)(LC + (i % 2) * (bw + gap));
        const uint16_t by  = (uint16_t)(by0 + (i / 2) * (bh + gap));
        const uint16_t mid = (uint16_t)(by + bh / 2);

        if (i == op) {
            lcd_fill_rect(bx, by, bw, bh, DARK);
            draw_hline(bx, (uint16_t)(bx + bw - 1), by, BLACK);
            draw_hline(bx, (uint16_t)(bx + bw - 1), (uint16_t)(by + bh - 1), BLACK);
            draw_vline(bx, by, (uint16_t)(by + bh - 1), BLACK);
            draw_vline((uint16_t)(bx + bw - 1), by, (uint16_t)(by + bh - 1), BLACK);
            draw_text((uint16_t)(bx + 18), (uint16_t)(mid - 3), OPS[i], WHITE);
        } else {
            draw_hline(bx, (uint16_t)(bx + bw - 1), by, LIGHT);
            draw_hline(bx, (uint16_t)(bx + bw - 1), (uint16_t)(by + bh - 1), LIGHT);
            draw_vline(bx, by, (uint16_t)(by + bh - 1), LIGHT);
            draw_vline((uint16_t)(bx + bw - 1), by, (uint16_t)(by + bh - 1), LIGHT);
            draw_text((uint16_t)(bx + 18), (uint16_t)(mid - 3), OPS[i], BLACK);
        }
        if (i == cur) {
            const uint8_t clv = (i == op) ? WHITE : DARK;
            draw_hline((uint16_t)(bx + 4), (uint16_t)(bx + 14), (uint16_t)(mid - 1), clv);
            draw_hline((uint16_t)(bx + 4), (uint16_t)(bx + 14),  mid,                clv);
            draw_hline((uint16_t)(bx + 4), (uint16_t)(bx + 14), (uint16_t)(mid + 1), clv);
        }
    }
    sep(3);

    draw_text(LC, row_y(4), OD[cur], DARK);
    sep(4);

    snprintf(b, sizeof b, "NOW: %s", op == 0 ? "STOPPED" : OPS[op]);
    draw_text(LC, row_y(5), b, op > 0 ? BLACK : DARK);
    if (op > 0) {
        snprintf(b, sizeof b, "V:%.3fV", (double)s->vb);
        draw_text_right(RX, row_y(5), b, BLACK);
    }
    sep(5);
}

/* ----- page 6: SETTING ----- */

typedef struct {
    const char *label;
    void      (*format_value)(char *buf, size_t n);
    void      (*format_pak)(char *buf, size_t n);   /* NULL if no PAK column */
} setting_item_t;

static void s_ich_v(char *b, size_t n) { snprintf(b, n, "%.1f A", (double)(sim_cfg.ich * 0.5f)); }
static void s_idh_v(char *b, size_t n) { snprintf(b, n, "%.1f A", (double)(sim_cfg.idh * 0.5f)); }
static void s_vfl_v(char *b, size_t n) { snprintf(b, n, "%.2f V", (double)(sim_cfg.vfl / 100.0f)); }
static void s_vfl_p(char *b, size_t n) { snprintf(b, n, "%.2fV", (double)(sim_cfg.vfl / 100.0f * (sim_cfg.cells + 1))); }
static void s_vcu_v(char *b, size_t n) { snprintf(b, n, "%.2f V", (double)(sim_cfg.vcu / 100.0f)); }
static void s_vcu_p(char *b, size_t n) { snprintf(b, n, "%.2fV", (double)(sim_cfg.vcu / 100.0f * (sim_cfg.cells + 1))); }
static void s_cycl_v(char *b, size_t n) { snprintf(b, n, "%u CYC", sim_cfg.cycl); }
static void s_rest_v(char *b, size_t n) { snprintf(b, n, "%.1f H", (double)(sim_cfg.rest * 0.5f)); }
static void s_cells_v(char *b, size_t n) { snprintf(b, n, "%u S", sim_cfg.cells + 1); }

static const setting_item_t SI[7] = {
    { "ICHG  :", s_ich_v,   NULL    },
    { "IDCHG :", s_idh_v,   NULL    },
    { "VFULL :", s_vfl_v,   s_vfl_p },
    { "VCUT  :", s_vcu_v,   s_vcu_p },
    { "CYCL  :", s_cycl_v,  NULL    },
    { "REST  :", s_rest_v,  NULL    },
    { "CELLS :", s_cells_v, NULL    },
};

static void page_setting(uint8_t cur) {
    char b[16];
    draw_header("SETTING");

    for (uint8_t i = 0; i < 7; ++i) {
        const setting_item_t *item = &SI[i];
        const uint16_t st  = (uint16_t)(HDR + i * TH);
        const bool sel = (i == cur);

        uint8_t label_color, value_color;
        if (sel) {
            lcd_fill_rect(TX, (uint16_t)(st + 1), CW, (uint16_t)(TH - 2), DARK);
            label_color = LIGHT;
            value_color = WHITE;
        } else {
            label_color = DARK;
            value_color = BLACK;
        }

        const uint16_t e = draw_text(LC, row_y(i), item->label, label_color);
        item->format_value(b, sizeof b);
        draw_text(e, row_y(i), b, value_color);

        if (item->format_pak) {
            draw_vline(MX, (uint16_t)(st + 2), (uint16_t)(st + TH - 3), LIGHT);
            item->format_pak(b, sizeof b);
            draw_text(RC, row_y(i), b, value_color);
        }
        sep(i);
    }
}

/* ----- entry point ----- */

void ui_render(uint8_t page, uint8_t sim_idx, uint8_t cur, uint8_t op) {
    lcd_clear();
    draw_tabs(page);

    if (sim_idx >= SIM_STATE_COUNT) sim_idx = 0;
    const sim_state_t *s = &sim_states[sim_idx];

    switch (page) {
        case 0: page_summary(s); break;
        case 1: page_charge(s); break;
        case 2: page_discharge(s); break;
        case 3: page_battery(s); break;
        case 4: page_learn(s); break;
        case 5: page_control(cur, op, s); break;
        case 6: page_setting(cur); break;
        default: break;
    }
}
