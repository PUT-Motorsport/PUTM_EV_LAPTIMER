#include "lvgl_ui.h"
#include <stdio.h>

#define OUTLINE 5
#define UI_LIST_SIZE 5

static lv_obj_t *sd_status_label;
static lv_obj_t *wifi_status_label;
static lv_obj_t *wifi_ip_label;
static lv_obj_t *gates_status_label;
static lv_obj_t *run_stop_btn;
static lv_obj_t *run_stop_label;

static lv_obj_t *current_lap_label;
static lv_obj_t *penalty_label;
static lv_obj_t *driver_label;
static lv_obj_t *oc_label;
static lv_obj_t *doo_label;

static lv_obj_t *top_laps_labels[UI_LIST_SIZE];
static lv_obj_t *last_laps_labels[UI_LIST_SIZE];

static lv_obj_t *status_bar_add_separator(lv_obj_t *parent)
{
    lv_obj_t *sep = lv_obj_create(parent);
    lv_obj_set_size(sep, 2, lv_pct(60));
    lv_obj_set_style_bg_color(
        sep,
        lv_palette_darken(LV_PALETTE_GREY, 3),
        LV_PART_MAIN);
    lv_obj_set_style_bg_opa(sep, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(sep, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(sep, 1, LV_PART_MAIN);
    lv_obj_remove_flag(sep, LV_OBJ_FLAG_SCROLLABLE);
    return sep;
}

void ui_init(void)
{
    lv_obj_set_style_bg_color(
        lv_screen_active(),
        lv_color_black(),
        LV_PART_MAIN);

    lv_obj_set_style_bg_opa(
        lv_screen_active(),
        LV_OPA_COVER,
        LV_PART_MAIN);

    lv_obj_t *main_div = lv_obj_create(lv_screen_active());
    lv_obj_set_size(main_div, lv_pct(100), lv_pct(100));
    lv_obj_set_layout(main_div, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(main_div, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(main_div, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(main_div, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(main_div, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(main_div, 0, LV_PART_MAIN);
    lv_obj_remove_flag(main_div, LV_OBJ_FLAG_SCROLLABLE);

    static lv_style_t main_style;
    lv_style_init(&main_style);
    lv_style_set_bg_opa(&main_style, LV_OPA_TRANSP);
    lv_style_set_radius(&main_style, 0);
    lv_style_set_border_width(&main_style, 0);
    lv_style_set_pad_all(&main_style, 5);

    static lv_style_t status_text_style;
    lv_style_init(&status_text_style);
    lv_style_set_text_font(&status_text_style, &lv_font_montserrat_14);

    static lv_style_t current_lap_style;
    lv_style_init(&current_lap_style);
    lv_style_set_radius(&current_lap_style, 10);
    lv_style_set_border_width(&current_lap_style, 0);
    lv_style_set_bg_color(&current_lap_style, lv_palette_darken(LV_PALETTE_GREY, 4));
    lv_style_set_pad_hor(&current_lap_style, 10);

    /// STATUS BAR
    lv_obj_t *status_bar_div = lv_obj_create(main_div);
    lv_obj_set_size(status_bar_div, lv_pct(100), lv_pct(10));
    lv_obj_add_style(status_bar_div, &main_style, LV_PART_MAIN);
    lv_obj_add_style(status_bar_div, &status_text_style, LV_PART_MAIN);
    lv_obj_remove_flag(status_bar_div, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_layout(status_bar_div, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(status_bar_div, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status_bar_div, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    sd_status_label = lv_label_create(status_bar_div);
    lv_obj_set_width(sd_status_label, lv_pct(10));
    lv_label_set_long_mode(sd_status_label, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(sd_status_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(sd_status_label, "SD");
    lv_obj_set_style_text_color(sd_status_label, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);
    status_bar_add_separator(status_bar_div);

    wifi_status_label = lv_label_create(status_bar_div);
    lv_obj_set_width(wifi_status_label, lv_pct(20));
    lv_label_set_long_mode(wifi_status_label, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(wifi_status_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(wifi_status_label, "OFF");
    lv_obj_set_style_text_color(wifi_status_label, lv_palette_main(LV_PALETTE_GREEN), LV_PART_MAIN);

    wifi_ip_label = lv_label_create(status_bar_div);
    lv_obj_set_width(wifi_ip_label, lv_pct(35));
    lv_label_set_long_mode(wifi_ip_label, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(wifi_ip_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(wifi_ip_label, "000.000.000.000");
    lv_obj_set_style_text_color(wifi_ip_label, lv_color_white(), LV_PART_MAIN);
    status_bar_add_separator(status_bar_div);

    gates_status_label = lv_label_create(status_bar_div);
    lv_obj_set_width(gates_status_label, lv_pct(5));
    lv_label_set_long_mode(gates_status_label, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(gates_status_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(gates_status_label, "1G");
    lv_obj_set_style_text_color(gates_status_label, lv_color_white(), LV_PART_MAIN);

    run_stop_btn = lv_button_create(status_bar_div);
    lv_obj_add_flag(run_stop_btn, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_height(run_stop_btn, LV_SIZE_CONTENT);
    lv_obj_set_width(run_stop_btn, lv_pct(15));
    lv_obj_set_style_pad_all(run_stop_btn, 1, LV_PART_MAIN);
    lv_obj_set_style_bg_color(run_stop_btn, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);
    lv_obj_set_style_text_color(run_stop_btn, lv_color_white(), LV_PART_MAIN);

    run_stop_label = lv_label_create(run_stop_btn);
    lv_label_set_text(run_stop_label, "STOP");
    lv_obj_center(run_stop_label);

    /// CURRENT LAP
    lv_obj_t *current_lap_div = lv_obj_create(main_div);
    lv_obj_set_size(current_lap_div, lv_pct(100), lv_pct(40));
    lv_obj_add_style(current_lap_div, &main_style, LV_PART_MAIN);
    lv_obj_remove_flag(current_lap_div, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(current_lap_div, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(current_lap_div, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_flex_main_place(current_lap_div, LV_FLEX_ALIGN_SPACE_EVENLY, LV_PART_MAIN);

    /// LAPTIME
    lv_obj_t *laptime_div = lv_obj_create(current_lap_div);
    lv_obj_set_size(laptime_div, lv_pct(100), lv_pct(65));
    lv_obj_add_style(laptime_div, &current_lap_style, LV_PART_MAIN);
    // lv_obj_set_style_pad_all(laptime_div, 5, LV_PART_MAIN);

    current_lap_label = lv_label_create(laptime_div);
    lv_label_set_text(current_lap_label, "--, --:--.--");
    lv_label_set_long_mode(current_lap_label, LV_LABEL_LONG_CLIP);
    lv_obj_align(current_lap_label, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_text_font(current_lap_label, &lv_font_montserrat_36, LV_PART_MAIN);
    lv_obj_set_style_text_color(current_lap_label, lv_color_white(), LV_PART_MAIN);

    penalty_label = lv_label_create(laptime_div);
    lv_label_set_text(penalty_label, "+00:00");
    lv_label_set_long_mode(penalty_label, LV_LABEL_LONG_CLIP);
    lv_obj_align(penalty_label, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_text_font(penalty_label, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_set_style_text_color(penalty_label, lv_palette_main(LV_PALETTE_YELLOW), LV_PART_MAIN);

    /// DRIVER
    lv_obj_t *driver_div = lv_obj_create(current_lap_div);
    lv_obj_set_size(driver_div, lv_pct(45), lv_pct(30));
    lv_obj_add_style(driver_div, &current_lap_style, LV_PART_MAIN);

    driver_label = lv_label_create(driver_div);
    lv_label_set_text(driver_label, "DRIVER: ---");
    lv_obj_align(driver_label, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_text_font(driver_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(driver_label, lv_color_white(), LV_PART_MAIN);

    /// OC COUNT
    lv_obj_t *oc_div = lv_obj_create(current_lap_div);
    lv_obj_set_size(oc_div, lv_pct(25), lv_pct(30));
    lv_obj_add_style(oc_div, &current_lap_style, LV_PART_MAIN);

    oc_label = lv_label_create(oc_div);
    lv_label_set_text(oc_label, "OC: 0");
    lv_obj_align(oc_label, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_text_font(oc_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(oc_label, lv_palette_main(LV_PALETTE_YELLOW), LV_PART_MAIN);

    /// DOO COUNT
    lv_obj_t *doo_div = lv_obj_create(current_lap_div);
    lv_obj_set_size(doo_div, lv_pct(25), lv_pct(30));
    lv_obj_add_style(doo_div, &current_lap_style, LV_PART_MAIN);

    doo_label = lv_label_create(doo_div);
    lv_label_set_text(doo_label, "DOO: 0");
    lv_obj_align(doo_label, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_text_font(doo_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(doo_label, lv_palette_main(LV_PALETTE_YELLOW), LV_PART_MAIN);

    /// LISTS
    lv_obj_t *lists_div = lv_obj_create(main_div);
    lv_obj_set_size(lists_div, lv_pct(100), lv_pct(50));
    lv_obj_add_style(lists_div, &main_style, LV_PART_MAIN);
    lv_obj_set_style_pad_all(lists_div, 0, LV_PART_MAIN);
    lv_obj_remove_flag(lists_div, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(lists_div, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(lists_div, LV_FLEX_FLOW_ROW);

    lv_obj_set_style_flex_main_place(lists_div, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_PART_MAIN);
    lv_obj_set_style_flex_cross_place(lists_div, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_flex_track_place(lists_div, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);

    lv_obj_t *top_div = lv_obj_create(lists_div);
    lv_obj_set_size(top_div, lv_pct(50), lv_pct(100));
    lv_obj_add_style(top_div, &main_style, LV_PART_MAIN);
    lv_obj_remove_flag(top_div, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_text_font(top_div, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_set_style_border_side(top_div, LV_BORDER_SIDE_RIGHT, LV_PART_MAIN);
    lv_obj_set_style_border_width(top_div, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(top_div, lv_palette_darken(LV_PALETTE_GREY, 4), LV_PART_MAIN);
    lv_obj_set_layout(top_div, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(top_div, LV_FLEX_FLOW_COLUMN);

    lv_obj_set_style_flex_main_place(top_div, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_PART_MAIN);
    lv_obj_set_style_flex_cross_place(top_div, LV_FLEX_ALIGN_START, LV_PART_MAIN);
    lv_obj_set_style_flex_track_place(top_div, LV_FLEX_ALIGN_START, LV_PART_MAIN);

    lv_obj_t *top_text = lv_label_create(top_div);
    lv_label_set_text(top_text, "TOP LAPS");
    lv_obj_set_style_text_font(top_text, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(top_text, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_border_side(top_text, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
    lv_obj_set_style_border_width(top_text, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(top_text, lv_palette_darken(LV_PALETTE_GREY, 4), LV_PART_MAIN);

    for (int i = 0; i < UI_LIST_SIZE; i++)
    {
        top_laps_labels[i] = lv_label_create(top_div);
        lv_label_set_text(top_laps_labels[i], "--, --:--.--");
        lv_obj_set_style_text_color(top_laps_labels[i], lv_color_white(), LV_PART_MAIN);
    }

    lv_obj_t *last_div = lv_obj_create(lists_div);
    lv_obj_set_size(last_div, lv_pct(50), lv_pct(100));
    lv_obj_add_style(last_div, &main_style, LV_PART_MAIN);
    lv_obj_remove_flag(last_div, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_text_font(last_div, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_set_style_border_side(last_div, LV_BORDER_SIDE_LEFT, LV_PART_MAIN);
    lv_obj_set_style_border_width(last_div, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(last_div, lv_palette_darken(LV_PALETTE_GREY, 4), LV_PART_MAIN);
    lv_obj_set_layout(last_div, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(last_div, LV_FLEX_FLOW_COLUMN);

    lv_obj_set_style_flex_main_place(last_div, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_PART_MAIN);
    lv_obj_set_style_flex_cross_place(last_div, LV_FLEX_ALIGN_START, LV_PART_MAIN);
    lv_obj_set_style_flex_track_place(last_div, LV_FLEX_ALIGN_START, LV_PART_MAIN);

    lv_obj_t *last_text = lv_label_create(last_div);
    lv_label_set_text(last_text, "LAST LAPS");
    lv_obj_set_style_text_font(last_text, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(last_text, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_border_side(last_text, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
    lv_obj_set_style_border_width(last_text, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(last_text, lv_palette_darken(LV_PALETTE_GREY, 4), LV_PART_MAIN);

    for (int i = 0; i < UI_LIST_SIZE; i++)
    {
        last_laps_labels[i] = lv_label_create(last_div);
        lv_label_set_text(last_laps_labels[i], "--, --:--.--");
        lv_obj_set_style_text_color(last_laps_labels[i], lv_color_white(), LV_PART_MAIN);
    }
}

void ui_update_status(bool sd_on, int wifi_mode, const char *ip_str, bool two_gates, bool stop_flag)
{
    if (sd_on)
    {
        lv_obj_set_style_text_color(sd_status_label, lv_palette_main(LV_PALETTE_GREEN), LV_PART_MAIN);
    }
    else
    {
        lv_obj_set_style_text_color(sd_status_label, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);
    }

    if (wifi_mode == 1)
    {
        lv_label_set_text(wifi_status_label, "WIFI STA");
        lv_obj_set_style_text_color(wifi_status_label, lv_palette_main(LV_PALETTE_GREEN), LV_PART_MAIN);
    }
    else if (wifi_mode == 2)
    {
        lv_label_set_text(wifi_status_label, "WIFI AP");
        lv_obj_set_style_text_color(wifi_status_label, lv_palette_main(LV_PALETTE_GREEN), LV_PART_MAIN);
    }
    else
    {
        lv_label_set_text(wifi_status_label, "WIFI OFF");
        lv_obj_set_style_text_color(wifi_status_label, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);
    }

    if (ip_str)
    {
        lv_label_set_text(wifi_ip_label, ip_str);
    }
    else
    {
        lv_label_set_text(wifi_ip_label, "NO IP");
    }

    if (two_gates)
    {
        lv_label_set_text(gates_status_label, "2G");
    }
    else
    {
        lv_label_set_text(gates_status_label, "1G");
    }

    if (stop_flag)
    {
        lv_obj_set_style_bg_color(run_stop_btn, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);
        lv_label_set_text(run_stop_label, "STOP");
    }
    else
    {
        lv_obj_set_style_bg_color(run_stop_btn, lv_palette_main(LV_PALETTE_GREEN), LV_PART_MAIN);
        lv_label_set_text(run_stop_label, "RUN");
    }
}

void ui_update_current_lap(const char *time_str, const char *penalty_str, int oc_count, int doo_count, const char *driver_name)
{
    if (time_str)
        lv_label_set_text(current_lap_label, time_str);
    if (penalty_str)
        lv_label_set_text(penalty_label, penalty_str);

    lv_label_set_text_fmt(oc_label, "OC: %d", oc_count);
    lv_label_set_text_fmt(doo_label, "DOO: %d", doo_count);

    if (driver_name)
        lv_label_set_text_fmt(driver_label, "DRIVER: %s", driver_name);
}

void ui_update_top_lap(int index, const char *time_str)
{
    if (index >= 0 && index < UI_LIST_SIZE)
    {
        if (time_str)
            lv_label_set_text(top_laps_labels[index], time_str);
    }
}

void ui_update_last_lap(int index, const char *time_str)
{
    if (index >= 0 && index < UI_LIST_SIZE)
    {
        if (time_str)
            lv_label_set_text(last_laps_labels[index], time_str);
    }
}