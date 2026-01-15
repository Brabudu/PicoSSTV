
#include "lcd_menu.h"

///////////////////////////////////

struct s_settings {
  uint8_t slideshow_timeout;
  uint8_t lost_signal_timeout;
  uint8_t min_completion;
  uint8_t transmit_mode;
  uint8_t auto_slant_correction;
  uint8_t overlay;
  uint8_t touch;
  uint8_t wifi;
  uint8_t color1;
  uint8_t color2;
  uint8_t color3;

  char overlay_text[25];
};

extern s_settings settings;

bool autoslant(menu_item m) {
  settings.auto_slant_correction=m.state;
  return true;
}

bool ls_timeout(menu_item m) {
  settings.lost_signal_timeout=m.id;
  return true;
}

bool sl_timeout(menu_item m) {
  settings.slideshow_timeout=m.id;
  return true;
}

bool im_save(menu_item m) {
  settings.min_completion=m.id;
  return true;
}

///////////////////////////////

menu_item ls_timeout_items[] = {
  MENU_ITEM(0, "Never", false, &ls_timeout, NULL),
  MENU_ITEM(1, "1 second", false, &ls_timeout, NULL),
  MENU_ITEM(2, "2 seconds", false, &ls_timeout, NULL),
  MENU_ITEM(3, "5 seconds", false, &ls_timeout, NULL),
  MENU_ITEM(4, "10 seconds", false, &ls_timeout, NULL),
  MENU_ITEM(5, "30 seconds", false, &ls_timeout, NULL),
  MENU_ITEM(6, "1 minute", false, &ls_timeout, NULL),
  MENU_ITEM(7, "5 minutes", false, &ls_timeout, NULL),
  MENU_ITEM(8, "10 minutes", false, &ls_timeout, NULL)
};

menu_list ls_timeout_menu = MENU_LIST(0, "Lost signal timeout", 9, NULL, ls_timeout_items);

///////////////////////////////

menu_item sl_timeout_items[] = {
  MENU_ITEM(0, "Never", false, &sl_timeout, NULL),
  MENU_ITEM(1, "1 second", false, &sl_timeout, NULL),
  MENU_ITEM(2, "2 seconds", false, &sl_timeout, NULL),
  MENU_ITEM(3, "5 seconds", false, &sl_timeout, NULL),
  MENU_ITEM(4, "10 seconds", false, &sl_timeout, NULL),
  MENU_ITEM(5, "30 seconds", false, &sl_timeout, NULL),
  MENU_ITEM(6, "1 minute", false, &sl_timeout, NULL),
  MENU_ITEM(7, "5 minutes", false, &sl_timeout, NULL),
  MENU_ITEM(8, "10 minutes", false, &sl_timeout, NULL)

};

menu_list sl_timeout_menu = MENU_LIST(0, "Slideshow timeout", 9, NULL, sl_timeout_items);

/////////////////////////////////////////

menu_item im_save_items[] = {
  MENU_ITEM(0, "90%", false, &im_save, NULL),
  MENU_ITEM(1, "75%", false, &im_save, NULL),
  MENU_ITEM(2, "50%", false, &im_save, NULL),
};

menu_list im_save_menu = MENU_LIST(0, "Min % to save", 3, NULL, im_save_items);

/////////////////////////////////////////

menu_item settings_items[] = {
  MENU_ITEM(0, "Auto slant", true, &autoslant, NULL),
  MENU_ITEM(1, "Lost sig. timeout", false, NULL, &ls_timeout_menu),
  MENU_ITEM(1, "Slideshow timeout", false, NULL, &sl_timeout_menu),
  MENU_ITEM(2, "Min save %", false, NULL, &im_save_menu),
  MENU_ITEM(3, "Transmit mode", false, NULL, NULL)
};

menu_list settings_menu = MENU_LIST(0, "Settings", 5, NULL, settings_items);

/////////////////////////////////////////////

menu_item main_items[] = {
  MENU_ITEM(0, "Transmit", false, NULL, NULL),
  MENU_ITEM(1, "Slideshow", false, NULL, NULL),
  MENU_ITEM(2, "Settings", false, NULL, &settings_menu)
};

menu_list main_menu = MENU_LIST(0, "Main Menu", 3, NULL, main_items);

///////////////////////////////////////

