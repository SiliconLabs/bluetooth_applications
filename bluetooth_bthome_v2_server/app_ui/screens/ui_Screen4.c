// This file was generated by SquareLine Studio
// SquareLine Studio version: SquareLine Studio 1.3.2
// LVGL version: 8.3.6
// Project name: SquareLine_Project

#include "../ui.h"

void ui_Screen4_screen_init(void)
{
  ui_Screen4 = lv_obj_create(NULL);
  lv_obj_clear_flag(ui_Screen4, LV_OBJ_FLAG_SCROLLABLE);    /// Flags
  lv_obj_set_style_bg_color(ui_Screen4,
                            lv_color_hex(0xFFFFFF),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(ui_Screen4, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_keyboardpn = lv_obj_create(ui_Screen4);
  lv_obj_set_width(ui_keyboardpn, 310);
  lv_obj_set_height(ui_keyboardpn, 230);
  lv_obj_set_x(ui_keyboardpn, 1);
  lv_obj_set_y(ui_keyboardpn, 75);
  lv_obj_set_align(ui_keyboardpn, LV_ALIGN_CENTER);
  lv_obj_clear_flag(ui_keyboardpn, LV_OBJ_FLAG_SCROLLABLE);    /// Flags
  lv_obj_set_style_bg_color(ui_keyboardpn,
                            lv_color_hex(0xFFFFFF),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(ui_keyboardpn, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_side(ui_keyboardpn,
                               LV_BORDER_SIDE_NONE,
                               LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_Keyboard1 = lv_keyboard_create(ui_keyboardpn);
  lv_obj_set_width(ui_Keyboard1, 300);
  lv_obj_set_height(ui_Keyboard1, 120);
  lv_obj_set_x(ui_Keyboard1, 0);
  lv_obj_set_y(ui_Keyboard1, 60);
  lv_obj_set_align(ui_Keyboard1, LV_ALIGN_CENTER);

  ui_setenckeytxtarea = lv_textarea_create(ui_keyboardpn);
  lv_obj_set_width(ui_setenckeytxtarea, 300);
  lv_obj_set_height(ui_setenckeytxtarea, LV_SIZE_CONTENT);  /// 70
  lv_obj_set_x(ui_setenckeytxtarea, 0);
  lv_obj_set_y(ui_setenckeytxtarea, -30);
  lv_obj_set_align(ui_setenckeytxtarea, LV_ALIGN_CENTER);
  lv_textarea_set_max_length(ui_setenckeytxtarea, 32);
  lv_textarea_set_placeholder_text(ui_setenckeytxtarea,
                                   "Enter the encription key");
  lv_textarea_set_one_line(ui_setenckeytxtarea, true);
  lv_obj_set_style_border_color(ui_setenckeytxtarea,
                                lv_color_hex(0x83DFF8),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_opa(ui_setenckeytxtarea,
                              255,
                              LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_setdevnametxtarea = lv_textarea_create(ui_keyboardpn);
  lv_obj_set_width(ui_setdevnametxtarea, 300);
  lv_obj_set_height(ui_setdevnametxtarea, LV_SIZE_CONTENT);  /// 70
  lv_obj_set_x(ui_setdevnametxtarea, 0);
  lv_obj_set_y(ui_setdevnametxtarea, -73);
  lv_obj_set_align(ui_setdevnametxtarea, LV_ALIGN_CENTER);
  lv_textarea_set_max_length(ui_setdevnametxtarea, 16);
  lv_textarea_set_placeholder_text(ui_setdevnametxtarea,
                                   "Enter the device name");
  lv_textarea_set_one_line(ui_setdevnametxtarea, true);
  lv_obj_set_style_border_color(ui_setdevnametxtarea,
                                lv_color_hex(0x83DFF8),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_opa(ui_setdevnametxtarea,
                              255,
                              LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_silabslogoscr4 = lv_img_create(ui_Screen4);
  lv_img_set_src(ui_silabslogoscr4, &ui_img_1228364278);
  lv_obj_set_width(ui_silabslogoscr4, LV_SIZE_CONTENT); /// 83
  lv_obj_set_height(ui_silabslogoscr4, LV_SIZE_CONTENT);  /// 48
  lv_obj_set_x(ui_silabslogoscr4, -113);
  lv_obj_set_y(ui_silabslogoscr4, -210);
  lv_obj_set_align(ui_silabslogoscr4, LV_ALIGN_CENTER);
  lv_obj_add_flag(ui_silabslogoscr4, LV_OBJ_FLAG_ADV_HITTEST);   /// Flags
  lv_obj_clear_flag(ui_silabslogoscr4, LV_OBJ_FLAG_SCROLLABLE);    /// Flags

  ui_deviceconfigpn = lv_obj_create(ui_Screen4);
  lv_obj_set_width(ui_deviceconfigpn, 300);
  lv_obj_set_height(ui_deviceconfigpn, 140);
  lv_obj_set_x(ui_deviceconfigpn, 0);
  lv_obj_set_y(ui_deviceconfigpn, -99);
  lv_obj_set_align(ui_deviceconfigpn, LV_ALIGN_CENTER);
  lv_obj_clear_flag(ui_deviceconfigpn, LV_OBJ_FLAG_SCROLLABLE);    /// Flags

  ui_deviceconfigtxt = lv_label_create(ui_deviceconfigpn);
  lv_obj_set_width(ui_deviceconfigtxt, LV_SIZE_CONTENT); /// 1
  lv_obj_set_height(ui_deviceconfigtxt, LV_SIZE_CONTENT);  /// 1
  lv_obj_set_x(ui_deviceconfigtxt, 1);
  lv_obj_set_y(ui_deviceconfigtxt, -41);
  lv_obj_set_align(ui_deviceconfigtxt, LV_ALIGN_CENTER);
  lv_label_set_text(ui_deviceconfigtxt, "Device Configuration");
  lv_obj_set_style_text_color(ui_deviceconfigtxt,
                              lv_color_hex(0x2D09FF),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_opa(ui_deviceconfigtxt,
                            255,
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui_deviceconfigtxt,
                             &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_deviceconfigmac = lv_label_create(ui_deviceconfigpn);
  lv_obj_set_width(ui_deviceconfigmac, 170);
  lv_obj_set_height(ui_deviceconfigmac, LV_SIZE_CONTENT);  /// 1
  lv_obj_set_x(ui_deviceconfigmac, 4);
  lv_obj_set_y(ui_deviceconfigmac, 7);
  lv_obj_set_align(ui_deviceconfigmac, LV_ALIGN_CENTER);
  lv_label_set_text(ui_deviceconfigmac, "");
  lv_obj_set_style_text_color(ui_deviceconfigmac,
                              lv_color_hex(0x0092FF),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_opa(ui_deviceconfigmac,
                            255,
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui_deviceconfigmac,
                             &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_exitscr3pn1 = lv_obj_create(ui_Screen4);
  lv_obj_set_width(ui_exitscr3pn1, 58);
  lv_obj_set_height(ui_exitscr3pn1, 35);
  lv_obj_set_x(ui_exitscr3pn1, 45);
  lv_obj_set_y(ui_exitscr3pn1, 219);
  lv_obj_set_align(ui_exitscr3pn1, LV_ALIGN_CENTER);
  lv_obj_clear_flag(ui_exitscr3pn1, LV_OBJ_FLAG_SCROLLABLE);    /// Flags
  lv_obj_set_style_radius(ui_exitscr3pn1, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(ui_exitscr3pn1,
                            lv_color_hex(0xF78C81),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(ui_exitscr3pn1, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_side(ui_exitscr3pn1,
                               LV_BORDER_SIDE_NONE,
                               LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_exiticon1 = lv_img_create(ui_exitscr3pn1);
  lv_img_set_src(ui_exiticon1, &ui_img_exit_icon_png);
  lv_obj_set_width(ui_exiticon1, LV_SIZE_CONTENT); /// 128
  lv_obj_set_height(ui_exiticon1, LV_SIZE_CONTENT);  /// 128
  lv_obj_set_x(ui_exiticon1, 0);
  lv_obj_set_y(ui_exiticon1, 1);
  lv_obj_set_align(ui_exiticon1, LV_ALIGN_CENTER);
  lv_obj_add_flag(ui_exiticon1, LV_OBJ_FLAG_ADV_HITTEST);   /// Flags
  lv_obj_clear_flag(ui_exiticon1, LV_OBJ_FLAG_SCROLLABLE);    /// Flags
  lv_img_set_zoom(ui_exiticon1, 50);

  ui_tickpn1 = lv_obj_create(ui_Screen4);
  lv_obj_set_width(ui_tickpn1, 58);
  lv_obj_set_height(ui_tickpn1, 35);
  lv_obj_set_x(ui_tickpn1, -35);
  lv_obj_set_y(ui_tickpn1, 219);
  lv_obj_set_align(ui_tickpn1, LV_ALIGN_CENTER);
  lv_obj_clear_flag(ui_tickpn1, LV_OBJ_FLAG_SCROLLABLE);    /// Flags
  lv_obj_set_style_radius(ui_tickpn1, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(ui_tickpn1,
                            lv_color_hex(0x9CF2AC),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(ui_tickpn1, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_side(ui_tickpn1,
                               LV_BORDER_SIDE_NONE,
                               LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_tickicon = lv_img_create(ui_tickpn1);
  lv_img_set_src(ui_tickicon, &ui_img_tick_icon_png);
  lv_obj_set_width(ui_tickicon, LV_SIZE_CONTENT); /// 87
  lv_obj_set_height(ui_tickicon, LV_SIZE_CONTENT);  /// 87
  lv_obj_set_align(ui_tickicon, LV_ALIGN_CENTER);
  lv_obj_add_flag(ui_tickicon, LV_OBJ_FLAG_ADV_HITTEST);   /// Flags
  lv_obj_clear_flag(ui_tickicon, LV_OBJ_FLAG_SCROLLABLE);    /// Flags
  lv_img_set_zoom(ui_tickicon, 75);

  lv_keyboard_set_textarea(ui_Keyboard1, ui_setenckeytxtarea);
  lv_obj_add_event_cb(ui_setenckeytxtarea,
                      ui_event_setenckeytxtarea,
                      LV_EVENT_ALL,
                      NULL);
  lv_obj_add_event_cb(ui_setdevnametxtarea,
                      ui_event_setdevnametxtarea,
                      LV_EVENT_ALL,
                      NULL);
  lv_obj_add_event_cb(ui_exitscr3pn1, ui_event_exitscr3pn1, LV_EVENT_ALL, NULL);
  lv_obj_add_event_cb(ui_tickpn1, ui_event_tickpn1, LV_EVENT_ALL, NULL);
}