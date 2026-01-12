#include <stdbool.h>
#include <stdint.h>

#include "ili934x.h"

/* Forward declarations */
struct menu_item;
struct menu_list;

/* Function pointer */
typedef bool (*GeneralFunction)(void);

typedef struct menu_list {
  uint8_t id;
  const char *title;
  uint8_t num_items;
  GeneralFunction callback;
  menu_item **items;  // flexible array member
} menu_list;

typedef struct menu_item {
  uint8_t id;
  const char *name;
  bool active;
  bool state;
  GeneralFunction callback;
  menu_list *sub_menu;
} menu_item;

///////////////////////

class lcd_menu {

public:
  lcd_menu(menu_list root, ILI934X *display);
//  void draw_menu_item(uint8_t row, menu_item * item);