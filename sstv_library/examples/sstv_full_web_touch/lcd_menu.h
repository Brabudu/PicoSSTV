#include <stdbool.h>
#include <stdint.h>

#include "XPT2046_Bitbang.h"
#include "ili934x.h"

/* Forward declarations */
struct menu_item;
struct menu_list;

/* Function pointer */
typedef bool (*GeneralFunction)(menu_item);

typedef struct menu_list {
  uint8_t id;
  const char *title;
  uint8_t num_items;
  GeneralFunction callback;
  menu_item *items;  // flexible array member
} menu_list;

typedef struct menu_item {
  uint8_t id;
  const char *name;
  bool active;
  bool state;
  bool is_on_off;
  GeneralFunction callback;
  menu_list *sub_menu;
} menu_item;

///////////////////////

class lcd_menu {

private:
  ILI934X *display;
  uint8_t get_touch_row();
  uint8_t get_touch_button();
  void draw_button_bar(const char* btn1, const char* btn2, const char* btn3, const char* btn4);
  void draw_menu_item(uint8_t row, menu_item item, bool selected);
public:
  lcd_menu(menu_list *root, ILI934X *disp);
 
};