
#include "lcd_menu.h"
#include "ili934x.h"
#include "font_16x12.h"

#include <Arduino.h>

#define NUM_LINES 7

#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 240
/*
#define MARGIN_LEFT 40
#define MARGIN_TOP 30
#define HEIGHT 25
#define WIDTH 200
*/
extern Stream *s;

lcd_menu ::lcd_menu(menu_list root, ILI934X* display) {
  delay(1000);
  display->clear(COLOUR_BLACK);
  uint16_t width = strlen(root.title) * 12;
  display->drawString((DISPLAY_WIDTH - width) / 2, 2, font_16x12, root.title, COLOUR_WHITE, COLOUR_BLUE);

  uint8_t items = 0;
  
  for (uint8_t idx = 0; idx < root.num_items; idx++) {
    if (root.items[idx]->active) {      
     // draw_menu_item(items,root.items[idx]);
      items++;
    }
    if (items > NUM_LINES) break;
  }
}
/*
void lcd_menu ::draw_menu_item(uint8_t row, menu_item * item) {
  display->fillRoundedRect(MARGIN_LEFT - 3, MARGIN_TOP + (row * HEIGHT) + 3, HEIGHT-5, WIDTH, 3, COLOUR_GREY);                  //Button Shading
  display->fillRoundedRect(MARGIN_LEFT, MARGIN_TOP + (row * HEIGHT), HEIGHT-5, WIDTH, 3, COLOUR_WHITE);                         // outter button color
  display->fillRoundedRect(MARGIN_LEFT + 1, MARGIN_TOP + (row * HEIGHT) + 1, HEIGHT - 7, WIDTH - 2, 3, COLOUR_BLUE);  //inner button color
  display->drawString(MARGIN_LEFT + 5, MARGIN_TOP + (row * HEIGHT) + 5, font_16x12, item->name, COLOUR_ORANGE, COLOUR_BLUE);
}
*/