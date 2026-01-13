
#include "lcd_menu.h"
#include "ili934x.h"
#include "font_16x12.h"
#include "font_8x5.h"
#include "XPT2046_Bitbang.h"

#include <Arduino.h>

#define NUM_LINES 7

#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 240

#define STATUS_BAR_HEIGHT 20

#define MARGIN_LEFT 40
#define MARGIN_TOP 26
#define HEIGHT 27
#define WIDTH 240

#define BGCOLOR COLOUR_BLUE



extern Stream* s;
extern XPT2046_Bitbang touchscreen;

lcd_menu ::lcd_menu(menu_list* root, ILI934X* disp) {
  delay(1000);
  display = disp;

  bool redraw = true;
  uint8_t items = 0;

  bool exit = false;

  while (!exit) {

    if (redraw) {
      display->clear(COLOUR_LIGHTGREY);
      uint16_t width = strlen(root->title) * 12;
      display->drawString((DISPLAY_WIDTH - width) / 2, 2, font_16x12, root->title, COLOUR_ORANGE, COLOUR_LIGHTGREY);
      draw_button_bar("Exit", "", "", "");

      items = 0;

      for (uint8_t idx = 0; idx < root->num_items; idx++) {
        if (root->items[idx].active) {
          draw_menu_item(items, root->items[idx], false);
          items++;
        }
        if (items >= NUM_LINES) break;
      }
      redraw = false;
    }
    uint8_t selection = 0;
    do {
      selection = get_touch_row();
    } while (selection == 0 || (selection > items && selection != 8));

    if (selection != 8) {
      menu_item actual = root->items[selection - 1];
      draw_menu_item(selection - 1, actual, true);
      delay(200);

      if (actual.sub_menu != NULL) {
        root = actual.sub_menu;
        redraw = true;
      } else if (actual.is_on_off) {
        actual.state = !(actual.state);
      }
      draw_menu_item(selection - 1, actual, false);

      if (actual.callback != NULL) {
        exit = (actual.callback)(actual);
      }
    } else {  //menu bar
        uint8_t pos=get_touch_button();
        if (pos==1) return;   
    }
  }
}


void lcd_menu ::draw_menu_item(uint8_t row, menu_item item, bool selected) {

  int delta = 0;
  uint16_t color = BGCOLOR;

  if (selected) {
    delta = -1;
    color = COLOUR_ORANGE;
  }

  display->fillRoundedRect(MARGIN_LEFT - 3, MARGIN_TOP + (row * HEIGHT) + 3, HEIGHT - 5, WIDTH, 3, COLOUR_GREY);                //Button Shading
  display->fillRoundedRect(MARGIN_LEFT + delta, MARGIN_TOP + (row * HEIGHT), HEIGHT - 5 - delta, WIDTH, 3, COLOUR_WHITE);       // outter button color
  display->fillRoundedRect(MARGIN_LEFT + delta + 1, MARGIN_TOP + (row * HEIGHT) - delta + 1, HEIGHT - 7, WIDTH - 2, 3, color);  //inner button color
  display->drawString(MARGIN_LEFT + 5 + delta, MARGIN_TOP + (row * HEIGHT) + 4 - delta, font_16x12, item.name, COLOUR_WHITE, color);

  if (item.sub_menu == NULL) {  //Final selection
    if (item.is_on_off) {
      if (item.state) display->drawString(MARGIN_LEFT + WIDTH - 30, MARGIN_TOP + (row * HEIGHT) + 4, font_16x12, "ON", COLOUR_YELLOW, color);
      else display->drawString(MARGIN_LEFT + WIDTH - 40, MARGIN_TOP + (row * HEIGHT) + 4, font_16x12, "OFF", COLOUR_ORANGE, color);
    }
  } else {  // Submenu
    display->drawString(MARGIN_LEFT + WIDTH - 20, MARGIN_TOP + (row * HEIGHT) + 4, font_16x12, ">", COLOUR_YELLOW, color);
  }
}

void lcd_menu ::draw_button_bar(const char* btn1, const char* btn2, const char* btn3, const char* btn4) {
  display->fillRect(0, DISPLAY_HEIGHT - STATUS_BAR_HEIGHT, STATUS_BAR_HEIGHT, DISPLAY_WIDTH, COLOUR_BLACK);
  const uint16_t button_width = 60;
  const uint16_t button_height = 14;
  const uint16_t padding = (DISPLAY_WIDTH - (4 * button_width)) / 5;
  const char* btn_txt[] = { btn1, btn2, btn3, btn4 };
  uint16_t button_x = padding;
  for (uint8_t idx = 0; idx < 4; ++idx) {
    bool active = strlen(btn_txt[idx]);
    display->fillRoundedRect(button_x, DISPLAY_HEIGHT - STATUS_BAR_HEIGHT + 3, button_height, button_width, 3, active ? COLOUR_BLUE : COLOUR_GREY);
    display->drawRoundedRect(button_x, DISPLAY_HEIGHT - STATUS_BAR_HEIGHT + 3, button_height, button_width, 3, active ? COLOUR_WHITE : COLOUR_LIGHTGREY);
    display->drawString(button_x + ((button_width - (6 * strlen(btn_txt[idx]))) / 2), DISPLAY_HEIGHT - STATUS_BAR_HEIGHT + 6, font_8x5, btn_txt[idx], COLOUR_WHITE, COLOUR_BLUE);
    button_x += button_width + padding;
  }
}

uint8_t lcd_menu::get_touch_row() {
  static uint8_t last_touch = 0;

  TouchPoint touch = touchscreen.getTouch();

  if (touch.zRaw > 600) {
    uint8_t row = (touch.y - MARGIN_TOP) / HEIGHT;
    if (touch.x > 10 && touch.x < 300) {
      if (last_touch != row + 1) {
        last_touch = row + 1;

        return row + 1;
      }
    }
  }
  last_touch = 0;
  return 0;
}

uint8_t lcd_menu::get_touch_button() {
  static uint8_t last_touch = 0;

  TouchPoint touch = touchscreen.getTouch();

  if (touch.zRaw > 600) {
    uint8_t pos = touch.x / 80;  //320 / 4
    if (touch.y > 200) {
      if (last_touch != pos + 1) {
        last_touch = pos + 1;
        delay(100);
        return pos + 1;
      }
    }
  }
  last_touch = 0;
  return 0;
}
