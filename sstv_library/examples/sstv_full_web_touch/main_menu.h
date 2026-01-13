
#include "lcd_menu.h"

bool exit() {
  return true;
}

#define MENU_ITEM(_id, _name, _onoff, _callback, _submenu) \
  { .id = _id, .name = _name, .active = true, .state = false, .is_on_off = _onoff, .callback = _callback, .sub_menu = _submenu }

#define MENU_LIST(_id, _title, _num, _callback, _items) \
  { .id = _id, .title = _title, .num_items = _num, .callback = _callback, .items = _items }

menu_item timeout_items[] = {
  MENU_ITEM(0, "Never", false, NULL, NULL),
  MENU_ITEM(1, "1 second", false, NULL, NULL),
  MENU_ITEM(2, "2 seconds", false, NULL, NULL),
  MENU_ITEM(3, "5 seconds", false, NULL, NULL),
  MENU_ITEM(4, "10 seconds", false, NULL, NULL),
  MENU_ITEM(5, "30 seconds", false, NULL, NULL),
  MENU_ITEM(6, "1 minute", false, NULL, NULL),
  MENU_ITEM(7, "5 minutes", false, NULL, NULL),
  MENU_ITEM(8, "10 minutes", false, NULL, NULL)

};

menu_list timeout_menu = MENU_LIST(0, "Lost signal timeout", 9, NULL, timeout_items);

/////////////////////////////////////////

menu_item settings_items[] = {
  MENU_ITEM(0, "Auto slant", true, NULL, NULL),
  MENU_ITEM(1, "Lost sig. timeout", false, NULL, &timeout_menu),
  MENU_ITEM(2, "Min save %", false, NULL, NULL),
  MENU_ITEM(3, "Transmit mode", false, NULL, NULL)
};

menu_list settings_menu = MENU_LIST(0, "Settings", 3, NULL, settings_items);

/////////////////////////////////////////////

menu_item main_items[] = {
  MENU_ITEM(0, "Transmit", false, NULL, NULL),
  MENU_ITEM(1, "Slideshow", false, NULL, NULL),
  MENU_ITEM(2, "Settings", false, NULL, &settings_menu)
};

menu_list main_menu = MENU_LIST(0, "Main Menu", 3, NULL, main_items);
