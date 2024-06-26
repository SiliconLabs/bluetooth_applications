// This file was generated by SquareLine Studio
// SquareLine Studio version: SquareLine Studio 1.3.2
// LVGL version: 8.3.6
// Project name: SquareLine_Project

#include "../ui.h"

void ui_Screen3_screen_init(void)
{
  ui_Screen3 = lv_obj_create(NULL);
  lv_obj_clear_flag(ui_Screen3, LV_OBJ_FLAG_SCROLLABLE);    /// Flags
  lv_obj_set_style_bg_color(ui_Screen3,
                            lv_color_hex(0xFFFFFF),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(ui_Screen3, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_devlistpn = lv_obj_create(ui_Screen3);
  lv_obj_set_width(ui_devlistpn, 300);
  lv_obj_set_height(ui_devlistpn, 347);
  lv_obj_set_x(ui_devlistpn, 2);
  lv_obj_set_y(ui_devlistpn, -5);
  lv_obj_set_align(ui_devlistpn, LV_ALIGN_CENTER);
  lv_obj_clear_flag(ui_devlistpn, LV_OBJ_FLAG_SCROLLABLE);    /// Flags
  lv_obj_set_style_border_color(ui_devlistpn,
                                lv_color_hex(0x1BA7EC),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_opa(ui_devlistpn, 255,
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_side(ui_devlistpn,
                               LV_BORDER_SIDE_NONE,
                               LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_devscannedpn0 = lv_obj_create(ui_devlistpn);
  lv_obj_set_width(ui_devscannedpn0, 280);
  lv_obj_set_height(ui_devscannedpn0, 40);
  lv_obj_set_x(ui_devscannedpn0, 1);
  lv_obj_set_y(ui_devscannedpn0, -100);
  lv_obj_set_align(ui_devscannedpn0, LV_ALIGN_CENTER);
  lv_obj_clear_flag(ui_devscannedpn0, LV_OBJ_FLAG_SCROLLABLE);    /// Flags
  lv_obj_add_flag(ui_devscannedpn0, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_style_bg_color(ui_devscannedpn0,
                            lv_color_hex(0x0278F0),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(ui_devscannedpn0, 255,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_side(ui_devscannedpn0,
                               LV_BORDER_SIDE_NONE,
                               LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_macdevlb0 = lv_label_create(ui_devscannedpn0);
  lv_obj_set_width(ui_macdevlb0, 180);
  lv_obj_set_height(ui_macdevlb0, LV_SIZE_CONTENT);  /// 2
  lv_obj_set_x(ui_macdevlb0, 12);
  lv_obj_set_y(ui_macdevlb0, 0);
  lv_obj_set_align(ui_macdevlb0, LV_ALIGN_CENTER);
  lv_label_set_text(ui_macdevlb0, "");
  lv_obj_set_style_text_color(ui_macdevlb0,
                              lv_color_hex(0xFFFFFF),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_opa(ui_macdevlb0, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui_macdevlb0,
                             &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_devscannedpn1 = lv_obj_create(ui_devlistpn);
  lv_obj_set_width(ui_devscannedpn1, 280);
  lv_obj_set_height(ui_devscannedpn1, 40);
  lv_obj_set_x(ui_devscannedpn1, 0);
  lv_obj_set_y(ui_devscannedpn1, -50);
  lv_obj_set_align(ui_devscannedpn1, LV_ALIGN_CENTER);
  lv_obj_clear_flag(ui_devscannedpn1, LV_OBJ_FLAG_SCROLLABLE);    /// Flags
  lv_obj_add_flag(ui_devscannedpn1, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_style_bg_color(ui_devscannedpn1,
                            lv_color_hex(0x027EEC),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(ui_devscannedpn1, 255,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_side(ui_devscannedpn1,
                               LV_BORDER_SIDE_NONE,
                               LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_macdevlb1 = lv_label_create(ui_devscannedpn1);
  lv_obj_set_width(ui_macdevlb1, 180);
  lv_obj_set_height(ui_macdevlb1, LV_SIZE_CONTENT);  /// 2
  lv_obj_set_x(ui_macdevlb1, 12);
  lv_obj_set_y(ui_macdevlb1, 0);
  lv_obj_set_align(ui_macdevlb1, LV_ALIGN_CENTER);
  lv_label_set_text(ui_macdevlb1, "");
  lv_obj_set_style_text_color(ui_macdevlb1,
                              lv_color_hex(0xFFFFFF),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_opa(ui_macdevlb1, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui_macdevlb1,
                             &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_devscannedpn2 = lv_obj_create(ui_devlistpn);
  lv_obj_set_width(ui_devscannedpn2, 280);
  lv_obj_set_height(ui_devscannedpn2, 40);
  lv_obj_set_align(ui_devscannedpn2, LV_ALIGN_CENTER);
  lv_obj_clear_flag(ui_devscannedpn2, LV_OBJ_FLAG_SCROLLABLE);    /// Flags
  lv_obj_add_flag(ui_devscannedpn2, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_style_bg_color(ui_devscannedpn2,
                            lv_color_hex(0x1284E8),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(ui_devscannedpn2, 255,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_side(ui_devscannedpn2,
                               LV_BORDER_SIDE_NONE,
                               LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_macdevlb2 = lv_label_create(ui_devscannedpn2);
  lv_obj_set_width(ui_macdevlb2, 180);
  lv_obj_set_height(ui_macdevlb2, LV_SIZE_CONTENT);  /// 2
  lv_obj_set_x(ui_macdevlb2, 12);
  lv_obj_set_y(ui_macdevlb2, 0);
  lv_obj_set_align(ui_macdevlb2, LV_ALIGN_CENTER);
  lv_label_set_text(ui_macdevlb2, "");
  lv_obj_set_style_text_color(ui_macdevlb2,
                              lv_color_hex(0xFFFFFF),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_opa(ui_macdevlb2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui_macdevlb2,
                             &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_devscannedpn3 = lv_obj_create(ui_devlistpn);
  lv_obj_set_width(ui_devscannedpn3, 280);
  lv_obj_set_height(ui_devscannedpn3, 40);
  lv_obj_set_x(ui_devscannedpn3, 0);
  lv_obj_set_y(ui_devscannedpn3, 50);
  lv_obj_set_align(ui_devscannedpn3, LV_ALIGN_CENTER);
  lv_obj_clear_flag(ui_devscannedpn3, LV_OBJ_FLAG_SCROLLABLE);    /// Flags
  lv_obj_add_flag(ui_devscannedpn3, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_style_bg_color(ui_devscannedpn3,
                            lv_color_hex(0x2389E4),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(ui_devscannedpn3, 255,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_side(ui_devscannedpn3,
                               LV_BORDER_SIDE_NONE,
                               LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_macdevlb3 = lv_label_create(ui_devscannedpn3);
  lv_obj_set_width(ui_macdevlb3, 180);
  lv_obj_set_height(ui_macdevlb3, LV_SIZE_CONTENT);  /// 2
  lv_obj_set_x(ui_macdevlb3, 12);
  lv_obj_set_y(ui_macdevlb3, 0);
  lv_obj_set_align(ui_macdevlb3, LV_ALIGN_CENTER);
  lv_label_set_text(ui_macdevlb3, "");
  lv_obj_set_style_text_color(ui_macdevlb3,
                              lv_color_hex(0xFFFFFF),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_opa(ui_macdevlb3, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui_macdevlb3,
                             &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_devscannedpn4 = lv_obj_create(ui_devlistpn);
  lv_obj_set_width(ui_devscannedpn4, 280);
  lv_obj_set_height(ui_devscannedpn4, 40);
  lv_obj_set_x(ui_devscannedpn4, 0);
  lv_obj_set_y(ui_devscannedpn4, 100);
  lv_obj_set_align(ui_devscannedpn4, LV_ALIGN_CENTER);
  lv_obj_clear_flag(ui_devscannedpn4, LV_OBJ_FLAG_SCROLLABLE);    /// Flags
  lv_obj_add_flag(ui_devscannedpn4, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_style_bg_color(ui_devscannedpn4,
                            lv_color_hex(0x328EDE),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(ui_devscannedpn4, 255,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_side(ui_devscannedpn4,
                               LV_BORDER_SIDE_NONE,
                               LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_macdevlb4 = lv_label_create(ui_devscannedpn4);
  lv_obj_set_width(ui_macdevlb4, 180);
  lv_obj_set_height(ui_macdevlb4, LV_SIZE_CONTENT);  /// 2
  lv_obj_set_x(ui_macdevlb4, 12);
  lv_obj_set_y(ui_macdevlb4, 0);
  lv_obj_set_align(ui_macdevlb4, LV_ALIGN_CENTER);
  lv_label_set_text(ui_macdevlb4, "");
  lv_obj_set_style_text_color(ui_macdevlb4,
                              lv_color_hex(0xFFFFFF),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_opa(ui_macdevlb4, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui_macdevlb4,
                             &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_devscannedpn5 = lv_obj_create(ui_devlistpn);
  lv_obj_set_width(ui_devscannedpn5, 280);
  lv_obj_set_height(ui_devscannedpn5, 40);
  lv_obj_set_x(ui_devscannedpn5, 0);
  lv_obj_set_y(ui_devscannedpn5, 150);
  lv_obj_set_align(ui_devscannedpn5, LV_ALIGN_CENTER);
  lv_obj_clear_flag(ui_devscannedpn5, LV_OBJ_FLAG_SCROLLABLE);    /// Flags
  lv_obj_add_flag(ui_devscannedpn5, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_style_bg_color(ui_devscannedpn5,
                            lv_color_hex(0x4192D9),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(ui_devscannedpn5, 255,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_side(ui_devscannedpn5,
                               LV_BORDER_SIDE_NONE,
                               LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_macdevlb5 = lv_label_create(ui_devscannedpn5);
  lv_obj_set_width(ui_macdevlb5, 180);
  lv_obj_set_height(ui_macdevlb5, LV_SIZE_CONTENT);  /// 2
  lv_obj_set_x(ui_macdevlb5, 12);
  lv_obj_set_y(ui_macdevlb5, 0);
  lv_obj_set_align(ui_macdevlb5, LV_ALIGN_CENTER);
  lv_label_set_text(ui_macdevlb5, "");
  lv_obj_set_style_text_color(ui_macdevlb5,
                              lv_color_hex(0xFFFFFF),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_opa(ui_macdevlb5, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui_macdevlb5,
                             &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_devlisttxt0 = lv_label_create(ui_devlistpn);
  lv_obj_set_width(ui_devlisttxt0, LV_SIZE_CONTENT); /// 1
  lv_obj_set_height(ui_devlisttxt0, LV_SIZE_CONTENT);  /// 1
  lv_obj_set_x(ui_devlisttxt0, 2);
  lv_obj_set_y(ui_devlisttxt0, -142);
  lv_obj_set_align(ui_devlisttxt0, LV_ALIGN_CENTER);
  lv_label_set_text(ui_devlisttxt0, "Device List");
  lv_obj_set_style_text_color(ui_devlisttxt0,
                              lv_color_hex(0x2D09FF),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_opa(ui_devlisttxt0, 255,
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui_devlisttxt0,
                             &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_headerscr3 = lv_obj_create(ui_Screen3);
  lv_obj_set_width(ui_headerscr3, 320);
  lv_obj_set_height(ui_headerscr3, 60);
  lv_obj_set_x(ui_headerscr3, 0);
  lv_obj_set_y(ui_headerscr3, -209);
  lv_obj_set_align(ui_headerscr3, LV_ALIGN_CENTER);
  lv_obj_clear_flag(ui_headerscr3, LV_OBJ_FLAG_SCROLLABLE);    /// Flags
  lv_obj_set_style_border_side(ui_headerscr3,
                               LV_BORDER_SIDE_NONE,
                               LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_silabslogoscr3 = lv_img_create(ui_headerscr3);
  lv_img_set_src(ui_silabslogoscr3, &ui_img_1228364278);
  lv_obj_set_width(ui_silabslogoscr3, LV_SIZE_CONTENT); /// 83
  lv_obj_set_height(ui_silabslogoscr3, LV_SIZE_CONTENT);  /// 48
  lv_obj_set_x(ui_silabslogoscr3, -108);
  lv_obj_set_y(ui_silabslogoscr3, -1);
  lv_obj_set_align(ui_silabslogoscr3, LV_ALIGN_CENTER);
  lv_obj_add_flag(ui_silabslogoscr3, LV_OBJ_FLAG_ADV_HITTEST);   /// Flags
  lv_obj_clear_flag(ui_silabslogoscr3, LV_OBJ_FLAG_SCROLLABLE);    /// Flags

  ui_exitscr3pn = lv_obj_create(ui_Screen3);
  lv_obj_set_width(ui_exitscr3pn, 58);
  lv_obj_set_height(ui_exitscr3pn, 35);
  lv_obj_set_x(ui_exitscr3pn, 6);
  lv_obj_set_y(ui_exitscr3pn, 219);
  lv_obj_set_align(ui_exitscr3pn, LV_ALIGN_CENTER);
  lv_obj_clear_flag(ui_exitscr3pn, LV_OBJ_FLAG_SCROLLABLE);    /// Flags
  lv_obj_set_style_radius(ui_exitscr3pn, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(ui_exitscr3pn,
                            lv_color_hex(0xF78C81),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(ui_exitscr3pn, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_side(ui_exitscr3pn,
                               LV_BORDER_SIDE_NONE,
                               LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_exiticon = lv_img_create(ui_exitscr3pn);
  lv_img_set_src(ui_exiticon, &ui_img_exit_icon_png);
  lv_obj_set_width(ui_exiticon, LV_SIZE_CONTENT); /// 128
  lv_obj_set_height(ui_exiticon, LV_SIZE_CONTENT);  /// 128
  lv_obj_set_align(ui_exiticon, LV_ALIGN_CENTER);
  lv_obj_add_flag(ui_exiticon, LV_OBJ_FLAG_ADV_HITTEST);   /// Flags
  lv_obj_clear_flag(ui_exiticon, LV_OBJ_FLAG_SCROLLABLE);    /// Flags
  lv_img_set_zoom(ui_exiticon, 50);

  lv_obj_add_event_cb(ui_devscannedpn0,
                      ui_event_devscannedpn0,
                      LV_EVENT_ALL,
                      NULL);
  lv_obj_add_event_cb(ui_devscannedpn1,
                      ui_event_devscannedpn1,
                      LV_EVENT_ALL,
                      NULL);
  lv_obj_add_event_cb(ui_devscannedpn2,
                      ui_event_devscannedpn2,
                      LV_EVENT_ALL,
                      NULL);
  lv_obj_add_event_cb(ui_devscannedpn3,
                      ui_event_devscannedpn3,
                      LV_EVENT_ALL,
                      NULL);
  lv_obj_add_event_cb(ui_devscannedpn4,
                      ui_event_devscannedpn4,
                      LV_EVENT_ALL,
                      NULL);
  lv_obj_add_event_cb(ui_devscannedpn5,
                      ui_event_devscannedpn5,
                      LV_EVENT_ALL,
                      NULL);
  lv_obj_add_event_cb(ui_devlistpn, ui_event_devlistpn, LV_EVENT_ALL, NULL);
  lv_obj_add_event_cb(ui_exitscr3pn, ui_event_exitscr3pn, LV_EVENT_ALL, NULL);
}
