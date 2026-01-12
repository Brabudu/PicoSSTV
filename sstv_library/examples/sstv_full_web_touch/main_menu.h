
#include "lcd_menu.h"
/*
menu_item settings_items[] = {
  { .id = 0,
    .name = "LED",
    .active = true,
    .state = false,
    .loopback = NULL,
    .sub_menu = NULL },
  { .id = 1,
    .name = "Info",
    .active = true,
    .state = false,
    .loopback = NULL,
    .sub_menu = NULL }
};

menu_list settings_menu = {
  .id = 0,
  .title = "Settings",
  .num_items = 2,
  .loopback = NULL,
  .items = {
    &settings_items[0],
    &settings_items[1] }
};


*/


menu_item main_items[] = {
  { .id = 0,
    .name = "Transmission",
    .active = true,
    .state = false,
    .callback = NULL,
    .sub_menu = NULL },

  { .id = 1,
    .name = "Slideshow",
    .active = true,
    .state = false,
    .callback = NULL,
    .sub_menu = NULL },
    
  { .id = 2,
    .name = "Settings",
    .active = true,
    .state = false,
    .callback = NULL,
    .sub_menu = NULL }

};

menu_item *main_menu_items[] = {
  &main_items[0],
  &main_items[1],
  &main_items[2]
};

menu_list main_menu = {
  .id = 0,
  .title = "Main Menu",
  .num_items = 3,
  .callback = NULL,
  .items = main_menu_items
};
