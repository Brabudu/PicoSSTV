// Copyright (c) Francesco Capuzzi 2025 IS0JSV
// MIT License


#include <stdbool.h>
#include <stdint.h>

#include "XPT2046_Bitbang.h"
#include "ili934x.h"

/* Forward declarations */
struct menu_item;
struct menu_list;

struct bar_menu;
struct bar_item;

/* Function pointer */
// Returns true to exit the menu 

typedef bool (*GeneralMenuFunction)(menu_item);

typedef struct menu_list {
  const uint8_t id;
  const char *title;
  const uint8_t num_items;
  const bool is_selectable_list;
  menu_item *items;  
} menu_list;

typedef struct menu_item {
  const uint8_t id;
  const char *name;
  bool active;
  bool state;
  const bool is_on_off;
  const GeneralMenuFunction callback;
  menu_list *sub_menu;
} menu_item;

#define MENU_ITEM(_id, _name, _onoff, _callback, _submenu) \
  { .id = _id, .name = _name, .active = true, .state = false, .is_on_off = _onoff, .callback = _callback, .sub_menu = _submenu }

#define MENU_LIST(_id, _title, _num, _selectable, _items) \
  { .id = _id, .title = _title, .num_items = _num, .is_selectable_list = _selectable, .items = _items }

///////////////////////

#define BAR_ITEM(_id, _name, _active) \
  { .id = _id, .name = _name, .active = _active, .selected = false }

typedef struct bar_item {
  const uint8_t id;
  const char *name;
  bool active;
  bool selected;
} bar_item;

typedef struct bar_menu {
  const uint8_t num_items;
  bar_item *items;
} bar_menu;

class lcd_menu {

private:
  uint8_t page = 0;

  bool get_touch(int &x, int &y, bool filtered);
  
  uint8_t get_touch_button(bool filtered);

  void draw_bar_item(bar_item bi);
  void draw_menu_item(uint8_t row, menu_item item, bool selected);
  void flash_button_bar_item(bar_menu b, uint8_t id, int millis);
public:
  lcd_menu();
  void launch_menu(menu_list *root);
  void draw_button_bar(bar_menu b);
  uint8_t poll_button_bar(bar_menu b, bool filtered);
  uint8_t poll_button_bar(bar_menu b);
  uint8_t get_touch_row();
};