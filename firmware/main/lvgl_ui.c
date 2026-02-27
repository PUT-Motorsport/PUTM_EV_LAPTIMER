#include "lvgl_ui.h"
#include <stdio.h>

#define OUTLINE 5
#define UI_LIST_SIZE 5

extern const lv_font_t roboto_mono_10;
extern const lv_font_t roboto_mono_14;
extern const lv_font_t roboto_mono_18;
extern const lv_font_t roboto_mono_38;

static lv_obj_t *sd_status_label;
static lv_obj_t *wifi_status_label;
static lv_obj_t *gates_status_label;
static lv_obj_t *run_stop_btn;
static lv_obj_t *run_stop_label;

static lv_obj_t *lap_time_label;
static lv_obj_t *penalty_time_label;
static lv_obj_t *driver_label;
static lv_obj_t *oc_count_label;
static lv_obj_t *doo_count_label;
static lv_obj_t *lap_count_label;

static lv_obj_t *top_table;
static lv_obj_t *last_table;

const lv_palette_t color_list[11] = {
    LV_PALETTE_NONE,
    LV_PALETTE_BLUE,
    LV_PALETTE_RED,
    LV_PALETTE_GREEN,
    LV_PALETTE_YELLOW,
    LV_PALETTE_BROWN,
    LV_PALETTE_CYAN,
    LV_PALETTE_DEEP_ORANGE,
    LV_PALETTE_DEEP_PURPLE,
    LV_PALETTE_LIME,
    LV_PALETTE_PINK,
};

void ui_init(void)
{

    static lv_style_t default_style;
    lv_style_init(&default_style);
    lv_style_set_bg_opa(&default_style, LV_OPA_TRANSP);
    lv_style_set_text_font(&default_style, &roboto_mono_14);
    lv_style_set_radius(&default_style, 0);
    lv_style_set_border_width(&default_style, 0);
    lv_style_set_pad_all(&default_style, 3);

    static lv_style_t center_style;
    lv_style_init(&center_style);
    lv_style_set_radius(&center_style, 5);
    lv_style_set_border_width(&center_style, 0);
    lv_style_set_bg_color(&center_style, lv_palette_darken(LV_PALETTE_GREY, 4));
    lv_style_set_pad_all(&center_style, 3);
    lv_style_set_margin_all(&center_style, 0);

    static lv_style_t statur_bar_style;
    lv_style_init(&statur_bar_style);
    lv_style_set_border_width(&statur_bar_style, 0);
    lv_style_set_pad_all(&statur_bar_style, 0);
    lv_style_set_margin_all(&statur_bar_style, 0);

    static lv_style_t table_style;
    lv_style_init(&table_style);
    lv_style_set_radius(&table_style, 0);
    lv_style_set_border_width(&table_style, 0);
    lv_style_set_bg_color(&table_style, lv_color_black());
    lv_style_set_pad_all(&table_style, 0);
    lv_style_set_margin_all(&table_style, 0);

    static lv_style_t table_cell_style;
    lv_style_init(&table_cell_style);
    lv_style_set_radius(&table_cell_style, 0);
    lv_style_set_border_width(&table_cell_style, 1);
    lv_style_set_border_color(&table_cell_style, lv_palette_darken(LV_PALETTE_GREY, 4));
    lv_style_set_bg_color(&table_cell_style, lv_color_black());
    lv_style_set_pad_all(&table_cell_style, 1);
    lv_style_set_margin_all(&table_cell_style, 0);
    lv_style_set_text_font(&table_cell_style, &roboto_mono_14);
    lv_style_set_text_color(&table_cell_style, lv_color_white());

    // BACKGROUND
    lv_obj_t *background_div = lv_obj_create(lv_screen_active());
    lv_obj_set_size(background_div, lv_pct(100), lv_pct(100));

    lv_obj_set_style_pad_all(background_div, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(background_div, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_radius(background_div, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(background_div, 0, LV_PART_MAIN);
    lv_obj_remove_flag(background_div, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_layout(background_div, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(background_div, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(background_div, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /// STATUS BAR
    lv_obj_t *status_bar_div = lv_obj_create(background_div);
    lv_obj_set_size(status_bar_div, lv_pct(100), lv_pct(10));
    lv_obj_add_style(status_bar_div, &default_style, LV_PART_MAIN);
    lv_obj_set_style_border_width(status_bar_div, 1, LV_BORDER_SIDE_BOTTOM);
    lv_obj_set_style_border_color(status_bar_div, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);
    lv_obj_remove_flag(status_bar_div, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_layout(status_bar_div, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(status_bar_div, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status_bar_div, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    sd_status_label = lv_label_create(status_bar_div);
    lv_obj_set_width(sd_status_label, lv_pct(5));
    lv_obj_add_style(sd_status_label, &statur_bar_style, LV_PART_MAIN);
    lv_label_set_long_mode(sd_status_label, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(sd_status_label, LV_TEXT_ALIGN_LEFT, 0);
    lv_label_set_text(sd_status_label, LV_SYMBOL_SD_CARD);
    lv_obj_set_style_text_color(sd_status_label, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);

    wifi_status_label = lv_label_create(status_bar_div);
    lv_obj_set_width(wifi_status_label, lv_pct(60));
    lv_obj_add_style(wifi_status_label, &statur_bar_style, LV_PART_MAIN);
    lv_label_set_long_mode(wifi_status_label, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(wifi_status_label, LV_TEXT_ALIGN_LEFT, 0);
    lv_label_set_text(wifi_status_label, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(wifi_status_label, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);

    gates_status_label = lv_label_create(status_bar_div);
    lv_obj_set_width(gates_status_label, lv_pct(20));
    lv_obj_add_style(gates_status_label, &statur_bar_style, LV_PART_MAIN);
    lv_label_set_long_mode(gates_status_label, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(gates_status_label, LV_TEXT_ALIGN_LEFT, 0);
    lv_label_set_text(gates_status_label, "- GATES");
    lv_obj_set_style_text_color(gates_status_label, lv_color_white(), LV_PART_MAIN);

    run_stop_btn = lv_button_create(status_bar_div);
    lv_obj_add_style(run_stop_btn, &statur_bar_style, LV_PART_MAIN);
    lv_obj_add_flag(run_stop_btn, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_height(run_stop_btn, LV_SIZE_CONTENT);
    lv_obj_set_width(run_stop_btn, lv_pct(10));
    lv_obj_set_style_pad_all(run_stop_btn, 1, LV_PART_MAIN);
    lv_obj_set_style_bg_color(run_stop_btn, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);
    lv_obj_set_style_text_color(run_stop_btn, lv_color_white(), LV_PART_MAIN);

    run_stop_label = lv_label_create(run_stop_btn);
    lv_label_set_text(run_stop_label, LV_SYMBOL_STOP);
    lv_obj_center(run_stop_label);

    /// CURRENT LAP TIME
    lv_obj_t *lap_current_div = lv_obj_create(background_div);
    lv_obj_set_size(lap_current_div, lv_pct(100), lv_pct(20));
    lv_obj_add_style(lap_current_div, &default_style, LV_PART_MAIN);
    lv_obj_set_style_bg_color(lap_current_div, lv_palette_main(LV_PALETTE_YELLOW), LV_PART_MAIN);
    lv_obj_remove_flag(lap_current_div, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_layout(lap_current_div, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(lap_current_div, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(lap_current_div, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *lap_time_div = lv_obj_create(lap_current_div);
    lv_obj_set_size(lap_time_div, lv_pct(70), lv_pct(100));
    lv_obj_add_style(lap_time_div, &center_style, LV_PART_MAIN);
    lv_obj_set_style_radius(lap_time_div, 20, LV_PART_MAIN);
    lv_obj_remove_flag(lap_time_div, LV_OBJ_FLAG_SCROLLABLE);

    lap_time_label = lv_label_create(lap_time_div);
    lv_label_set_text(lap_time_label, "00:00.00");
    lv_obj_center(lap_time_label);
    lv_obj_set_style_text_font(lap_time_label, &roboto_mono_38, LV_PART_MAIN);
    lv_obj_set_style_text_color(lap_time_label, lv_color_white(), LV_PART_MAIN);

    /// LAP INFO
    lv_obj_t *lap_info_div = lv_obj_create(background_div);
    lv_obj_set_size(lap_info_div, lv_pct(100), lv_pct(20));
    lv_obj_add_style(lap_info_div, &default_style, LV_PART_MAIN);
    lv_obj_set_style_bg_color(lap_info_div, lv_palette_main(LV_PALETTE_BLUE), LV_PART_MAIN);
    lv_obj_remove_flag(lap_info_div, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_layout(lap_info_div, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(lap_info_div, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(lap_info_div, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /// DRIVER
    lv_obj_t *driver_div = lv_obj_create(lap_info_div);
    lv_obj_set_size(driver_div, lv_pct(25), lv_pct(100));
    lv_obj_add_style(driver_div, &center_style, LV_PART_MAIN);

    lv_obj_set_layout(driver_div, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(driver_div, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(driver_div, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *driver_title = lv_label_create(driver_div);
    lv_label_set_text(driver_title, "DRIVER");
    lv_obj_set_style_text_font(driver_title, &roboto_mono_10, LV_PART_MAIN);
    lv_obj_set_style_text_color(driver_title, lv_color_white(), LV_PART_MAIN);

    driver_label = lv_label_create(driver_div);
    lv_label_set_text(driver_label, "WWW");
    lv_obj_set_style_text_font(driver_label, &roboto_mono_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(driver_label, lv_color_white(), LV_PART_MAIN);

    /// LAP COUNT
    lv_obj_t *lap_count_div = lv_obj_create(lap_info_div);
    lv_obj_set_size(lap_count_div, lv_pct(15), lv_pct(100));
    lv_obj_add_style(lap_count_div, &center_style, LV_PART_MAIN);

    lv_obj_set_layout(lap_count_div, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(lap_count_div, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(lap_count_div, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *lap_count_title = lv_label_create(lap_count_div);
    lv_label_set_text(lap_count_title, "LAP");
    lv_obj_set_style_text_font(lap_count_title, &roboto_mono_10, LV_PART_MAIN);
    lv_obj_set_style_text_color(lap_count_title, lv_color_white(), LV_PART_MAIN);

    lap_count_label = lv_label_create(lap_count_div);
    lv_label_set_text(lap_count_label, "00");
    lv_obj_set_style_text_font(lap_count_label, &roboto_mono_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(lap_count_label, lv_color_white(), LV_PART_MAIN);

    /// OC COUNT
    lv_obj_t *oc_count_div = lv_obj_create(lap_info_div);
    lv_obj_set_size(oc_count_div, lv_pct(15), lv_pct(100));
    lv_obj_add_style(oc_count_div, &center_style, LV_PART_MAIN);

    lv_obj_set_layout(oc_count_div, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(oc_count_div, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(oc_count_div, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *oc_count_title = lv_label_create(oc_count_div);
    lv_label_set_text(oc_count_title, "OC");
    lv_obj_set_style_text_font(oc_count_title, &roboto_mono_10, LV_PART_MAIN);
    lv_obj_set_style_text_color(oc_count_title, lv_palette_main(LV_PALETTE_AMBER), LV_PART_MAIN);

    oc_count_label = lv_label_create(oc_count_div);
    lv_label_set_text(oc_count_label, "00");
    lv_obj_set_style_text_font(oc_count_label, &roboto_mono_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(oc_count_label, lv_palette_main(LV_PALETTE_AMBER), LV_PART_MAIN);

    /// DOO COUNT
    lv_obj_t *doo_count_div = lv_obj_create(lap_info_div);
    lv_obj_set_size(doo_count_div, lv_pct(15), lv_pct(100));
    lv_obj_add_style(doo_count_div, &center_style, LV_PART_MAIN);

    lv_obj_set_layout(doo_count_div, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(doo_count_div, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(doo_count_div, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *doo_count_title = lv_label_create(doo_count_div);
    lv_label_set_text(doo_count_title, "DOO");
    lv_obj_set_style_text_font(doo_count_title, &roboto_mono_10, LV_PART_MAIN);
    lv_obj_set_style_text_color(doo_count_title, lv_palette_main(LV_PALETTE_AMBER), LV_PART_MAIN);

    doo_count_label = lv_label_create(doo_count_div);
    lv_label_set_text(doo_count_label, "00");
    lv_obj_set_style_text_font(doo_count_label, &roboto_mono_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(doo_count_label, lv_palette_main(LV_PALETTE_AMBER), LV_PART_MAIN);

    /// PENALTY TIME
    lv_obj_t *penalty_time_div = lv_obj_create(lap_info_div);
    lv_obj_set_size(penalty_time_div, lv_pct(25), lv_pct(100));
    lv_obj_add_style(penalty_time_div, &center_style, LV_PART_MAIN);

    lv_obj_set_layout(penalty_time_div, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(penalty_time_div, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(penalty_time_div, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *penalty_time_title = lv_label_create(penalty_time_div);
    lv_label_set_text(penalty_time_title, "PENALTY");
    lv_obj_set_style_text_font(penalty_time_title, &roboto_mono_10, LV_PART_MAIN);
    lv_obj_set_style_text_color(penalty_time_title, lv_palette_main(LV_PALETTE_AMBER), LV_PART_MAIN);

    penalty_time_label = lv_label_create(penalty_time_div);
    lv_label_set_text(penalty_time_label, "+00:00");
    lv_obj_set_style_text_font(penalty_time_label, &roboto_mono_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(penalty_time_label, lv_palette_main(LV_PALETTE_AMBER), LV_PART_MAIN);

    /// LISTS
    lv_obj_t *lists_div = lv_obj_create(background_div);
    lv_obj_set_size(lists_div, lv_pct(100), lv_pct(50));
    lv_obj_add_style(lists_div, &default_style, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(lists_div, 100, LV_PART_MAIN);
    lv_obj_set_style_pad_all(lists_div, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(lists_div, lv_palette_darken(LV_PALETTE_GREY, 4), LV_PART_MAIN);
    lv_obj_remove_flag(lists_div, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_layout(lists_div, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(lists_div, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(lists_div, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    top_table = lv_table_create(lists_div);
    lv_obj_set_size(top_table, lv_pct(50), lv_pct(100));
    lv_table_set_column_count(top_table, 3);
    lv_table_set_row_count(top_table, 6);
    lv_table_set_column_width(top_table, 0, 30);
    lv_table_set_column_width(top_table, 1, 75);
    lv_table_set_column_width(top_table, 2, 55);
    lv_table_set_cell_ctrl(top_table, 0, 0, LV_TABLE_CELL_CTRL_MERGE_RIGHT);
    lv_table_set_cell_ctrl(top_table, 0, 1, LV_TABLE_CELL_CTRL_MERGE_RIGHT);
    lv_obj_add_style(top_table, &table_style, LV_PART_MAIN);
    lv_obj_add_style(top_table, &table_cell_style, LV_PART_ITEMS);
    lv_obj_remove_flag(top_table, LV_OBJ_FLAG_SCROLLABLE);
    lv_table_set_cell_value(top_table, 0, 0, "TOP LAPS");

    lv_table_set_cell_value(top_table, 1, 0, "00");
    lv_table_set_cell_value(top_table, 1, 1, "24:55.98");
    lv_table_set_cell_value(top_table, 1, 2, "WWW");

    lv_table_set_cell_value(top_table, 2, 0, "00");
    lv_table_set_cell_value(top_table, 2, 1, "24:55.98");
    lv_table_set_cell_value(top_table, 2, 2, "WWW");

    lv_table_set_cell_value(top_table, 3, 0, "00");
    lv_table_set_cell_value(top_table, 3, 1, "24:55.98");
    lv_table_set_cell_value(top_table, 3, 2, "WWW");

    lv_table_set_cell_value(top_table, 4, 0, "00");
    lv_table_set_cell_value(top_table, 4, 1, "24:55.98");
    lv_table_set_cell_value(top_table, 4, 2, "WWW");

    lv_table_set_cell_value(top_table, 5, 0, "00");
    lv_table_set_cell_value(top_table, 5, 1, "24:55.98");
    lv_table_set_cell_value(top_table, 5, 2, "WWW");

    last_table = lv_table_create(lists_div);
    lv_obj_set_size(last_table, lv_pct(50), lv_pct(100));
    lv_table_set_column_count(last_table, 3);
    lv_table_set_row_count(last_table, 6);
    lv_table_set_column_width(last_table, 0, 30);
    lv_table_set_column_width(last_table, 1, 75);
    lv_table_set_column_width(last_table, 2, 55);
    lv_table_set_cell_ctrl(last_table, 0, 0, LV_TABLE_CELL_CTRL_MERGE_RIGHT);
    lv_table_set_cell_ctrl(last_table, 0, 1, LV_TABLE_CELL_CTRL_MERGE_RIGHT);
    lv_obj_add_style(last_table, &table_style, LV_PART_MAIN);
    lv_obj_add_style(last_table, &table_cell_style, LV_PART_ITEMS);
    lv_obj_remove_flag(last_table, LV_OBJ_FLAG_SCROLLABLE);
    lv_table_set_cell_value(last_table, 0, 0, "LAST LAPS");

    lv_table_set_cell_value(last_table, 1, 0, "00");
    lv_table_set_cell_value(last_table, 1, 1, "24:55.98");
    lv_table_set_cell_value(last_table, 1, 2, "WWW");

    lv_table_set_cell_value(last_table, 2, 0, "00");
    lv_table_set_cell_value(last_table, 2, 1, "24:55.98");
    lv_table_set_cell_value(last_table, 2, 2, "WWW");

    lv_table_set_cell_value(last_table, 3, 0, "00");
    lv_table_set_cell_value(last_table, 3, 1, "24:55.98");
    lv_table_set_cell_value(last_table, 3, 2, "WWW");

    lv_table_set_cell_value(last_table, 4, 0, "00");
    lv_table_set_cell_value(last_table, 4, 1, "24:55.98");
    lv_table_set_cell_value(last_table, 4, 2, "WWW");

    lv_table_set_cell_value(last_table, 5, 0, "00");
    lv_table_set_cell_value(last_table, 5, 1, "24:55.98");
    lv_table_set_cell_value(last_table, 5, 2, "WWW");
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

    static char wifi_str[62] = "\0";
    if (wifi_mode == 1)
    {
        snprintf(wifi_str, sizeof(wifi_str), LV_SYMBOL_WIFI "STA: %s", ip_str);
        lv_label_set_text(wifi_status_label, wifi_str);
        lv_obj_set_style_text_color(wifi_status_label, lv_palette_main(LV_PALETTE_GREEN), LV_PART_MAIN);
    }
    else if (wifi_mode == 2)
    {
        snprintf(wifi_str, sizeof(wifi_str), LV_SYMBOL_WIFI "AP: %s", ip_str);
        lv_label_set_text(wifi_status_label, wifi_str);
        lv_obj_set_style_text_color(wifi_status_label, lv_palette_main(LV_PALETTE_GREEN), LV_PART_MAIN);
    }
    else
    {
        snprintf(wifi_str, sizeof(wifi_str), LV_SYMBOL_WIFI);
        lv_label_set_text(wifi_status_label, wifi_str);
        lv_obj_set_style_text_color(wifi_status_label, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);
    }

    if (two_gates)
    {
        lv_label_set_text(gates_status_label, "2 GATES");
    }
    else
    {
        lv_label_set_text(gates_status_label, "1 GATE");
    }

    if (stop_flag)
    {
        lv_obj_set_style_bg_color(run_stop_btn, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);
        lv_label_set_text(run_stop_label, LV_SYMBOL_STOP);
    }
    else
    {
        lv_obj_set_style_bg_color(run_stop_btn, lv_palette_main(LV_PALETTE_GREEN), LV_PART_MAIN);
        lv_label_set_text(run_stop_label, LV_SYMBOL_PLAY);
    }
}

void ui_update_current_lap(const char *time_str, const char *penalty_str, int oc_count, int doo_count, const char *driver_str, int driver_id)
{
    if (time_str)
        lv_label_set_text(lap_time_label, time_str);
    if (penalty_str)
        lv_label_set_text(penalty_time_label, penalty_str);

    lv_label_set_text_fmt(oc_count_label, "%d", oc_count);
    lv_label_set_text_fmt(doo_count_label, "%d", doo_count);

    if (driver_str)
    {
        lv_label_set_text_fmt(driver_label, "%s", driver_str);
        lv_obj_set_style_text_color(driver_label, lv_palette_main(color_list[driver_id]), LV_PART_MAIN);
    }
}

void ui_update_top_lap(int index, const char *lap_str, const char *time_str, const char *driver_str, int driver_id)
{
    if (index >= 0 && index <= UI_LIST_SIZE)
    {
        if (time_str && driver_str && lap_str)
        {
            lv_table_set_cell_value(top_table, index, 0, lap_str);
            lv_table_set_cell_value(top_table, index, 1, time_str);
            lv_table_set_cell_value(top_table, index, 2, driver_str);
        }
    }
}

void ui_update_last_lap(int index, const char *lap_str, const char *time_str, const char *driver_str, int driver_id)
{
    if (index >= 0 && index <= UI_LIST_SIZE)
    {
        if (time_str && driver_str && lap_str)
        {
            lv_table_set_cell_value(last_table, index, 0, lap_str);
            lv_table_set_cell_value(last_table, index, 1, time_str);
            lv_table_set_cell_value(last_table, index, 2, driver_str);
        }
    }
}