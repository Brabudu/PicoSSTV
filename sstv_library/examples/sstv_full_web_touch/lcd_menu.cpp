
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

#define BUTTON_WIDTH 60
#define BUTTON_HEIGHT 14
#define PADDING (DISPLAY_WIDTH - (4 * BUTTON_WIDTH)) / 5

#define MARGIN_LEFT 40
#define MARGIN_TOP 26
#define HEIGHT 27
#define WIDTH 240

#define BGCOLOR COLOUR_BLUE

#define POLL_MS 5 //min ms to wait when polling touchscreen

extern Stream* s;
extern XPT2046_Bitbang touchscreen;
extern ILI934X* display;

extern void poll_news();

bar_item menu_bar_items[] = {
  BAR_ITEM(0, "Exit", true),
  BAR_ITEM(1, "", false),
  BAR_ITEM(2, "Prev", false),
  BAR_ITEM(3, "Next", false)
};

bar_menu menu_bar = { 4, menu_bar_items };

/////////////////////////////////



///////////////////////////

lcd_menu ::lcd_menu() {
  touchscreen.setCalibration(100, 3900, 100, 3900);
}

void lcd_menu ::launch_menu(menu_list* root) {

  bool redraw = true;
  uint8_t items = 0;
  page = 0;

  bool exit = false;

  while (!exit) {


    

    if (redraw) {
      display->fillRect(0, 0, DISPLAY_HEIGHT - STATUS_BAR_HEIGHT, DISPLAY_WIDTH, COLOUR_LIGHTGREY);
      uint16_t width = strlen(root->title) * 12;
      display->drawString((DISPLAY_WIDTH - width) / 2, 5, font_16x12, root->title, COLOUR_RED, COLOUR_LIGHTGREY);

      items = 0;

      for (uint8_t idx = page * NUM_LINES; idx < root->num_items; idx++) {
        if (root->items[idx].active) {
          draw_menu_item(items, root->items[idx], false);
          items++;
        }
        if (items >= NUM_LINES) {
          menu_bar.items[3].active = true;  //Next button
          break;
        } else {
          menu_bar.items[3].active = false;
        }
      }

      menu_bar.items[2].active = (page > 0);  //Prev button
      redraw = false;

      draw_button_bar(menu_bar);
    }
    uint8_t selection = 0;
    do {
      selection = get_touch_row();
      poll_news();
    } while (selection == 0 || (selection > items && selection != 8));

    if (selection != 8) {
      menu_item* actual = &root->items[page * NUM_LINES + selection - 1];
      draw_menu_item(selection - 1, *actual, true);
      delay(300);

      if (actual->sub_menu != NULL) {
        root = actual->sub_menu;
        redraw = true;
      } else if (actual->is_on_off) {
        actual->state = !(actual->state);
      } else if (root->is_selectable_list) {
        for (int i = 0; i < root->num_items; i++) {
          root->items[i].state = false;
        }
        root->items[actual->id].state = true;
        
      }


      draw_menu_item(selection - 1, *actual, false);

      if (actual->callback != NULL) {
        exit = (actual->callback)(*actual);
      }

    } else {  //menu bar
      uint8_t pos;
      do {
        pos = get_touch_button(true);
      } while (pos == 0);
      if (menu_bar.items[pos - 1].active) {

        flash_button_bar_item(menu_bar, pos - 1, 100);

        bar_item bi = menu_bar.items[pos - 1];
        if (bi.id == 0) return;  //exit
        if (bi.id == 2) page--;  //prev
        if (bi.id == 3) page++;  //prev
        redraw = true;
      }
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
    if (item.is_on_off) {       //toggle item
      if (item.state) display->drawString(MARGIN_LEFT + WIDTH - 30, MARGIN_TOP + (row * HEIGHT) + 4, font_16x12, "ON", COLOUR_YELLOW, color);
      else display->drawString(MARGIN_LEFT + WIDTH - 40, MARGIN_TOP + (row * HEIGHT) + 4, font_16x12, "OFF", COLOUR_ORANGE, color);
    } else {  //Selected item
      if (item.state) display->drawString(MARGIN_LEFT + WIDTH - 20, MARGIN_TOP + (row * HEIGHT) + 4, font_16x12, "*", COLOUR_YELLOW, color);
    }
  } else {  // Submenu
    display->drawString(MARGIN_LEFT + WIDTH - 20, MARGIN_TOP + (row * HEIGHT) + 4, font_16x12, ">", COLOUR_YELLOW, color);
  }
}

void lcd_menu ::draw_button_bar(bar_menu b) {
  display->fillRect(0, DISPLAY_HEIGHT - STATUS_BAR_HEIGHT, STATUS_BAR_HEIGHT, (PADDING + BUTTON_WIDTH) * b.num_items + PADDING, COLOUR_BLACK);

  for (uint8_t idx = 0; idx < b.num_items; ++idx) {

    bar_item item = b.items[idx];
    draw_bar_item(item);
  }
}

uint8_t lcd_menu::poll_button_bar(bar_menu b) {
  return poll_button_bar(b, true);
}

uint8_t lcd_menu::poll_button_bar(bar_menu b, bool filtered) {
  static uint32_t last_update_time = millis();

  if ((millis() - last_update_time) < POLL_MS) return 0;

  last_update_time = millis();

  uint8_t pos = get_touch_button(filtered);
  if (pos == 0) return 0;

  if (b.items[pos - 1].active) flash_button_bar_item(b, pos - 1, 100);
  return pos;
}

void lcd_menu::flash_button_bar_item(bar_menu b, uint8_t id, int millis) {

  bar_item bi = b.items[id];
  
  bi.selected = true;
  draw_bar_item(bi);
  delay(millis);
  bi.selected = false;
  draw_bar_item(bi);
}
void lcd_menu::draw_bar_item(bar_item bi) {


  uint16_t button_x = (PADDING + BUTTON_WIDTH) * bi.id + PADDING;
  uint16_t bg = COLOUR_GREY;
  uint16_t fg = COLOUR_LIGHTGREY;
  if (bi.active) {
    bg = COLOUR_BLUE;
    fg = COLOUR_WHITE;
  }
  if (bi.selected) {
    bg = COLOUR_ORANGE;
    fg = COLOUR_WHITE;
  }

  display->fillRoundedRect(button_x, DISPLAY_HEIGHT - STATUS_BAR_HEIGHT + 3, BUTTON_HEIGHT, BUTTON_WIDTH, 3, bg);
  display->drawRoundedRect(button_x, DISPLAY_HEIGHT - STATUS_BAR_HEIGHT + 3, BUTTON_HEIGHT, BUTTON_WIDTH, 3, fg);
  display->drawString(button_x + ((BUTTON_WIDTH - (6 * strlen(bi.name))) / 2), DISPLAY_HEIGHT - STATUS_BAR_HEIGHT + 6, font_8x5, bi.name, fg, bg);
}

uint8_t lcd_menu::get_touch_row() {
  static uint8_t last_touch = 0;

  int x, y;

  if (get_touch(x, y,true) && y > MARGIN_TOP) {

    uint8_t row = (y - MARGIN_TOP) / HEIGHT;

    if (x > PADDING && x < (DISPLAY_WIDTH - PADDING)) {
      if (last_touch != row + 1) {
        last_touch = row + 1;

        return row + 1;
      }
    }
  }
  last_touch = 0;
  return 0;
}

uint8_t lcd_menu::get_touch_button(bool filtered) {
  static uint8_t last_touch = 0;
  int x, y;

  if (get_touch(x, y, filtered)) {
    uint8_t pos = x / 80;  //320 / 4

    if (y > DISPLAY_HEIGHT - STATUS_BAR_HEIGHT) {

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

bool lcd_menu::get_touch(int& x, int& y, bool filtered) {
  // Retrieve a point
  static int mX = 0;
  static int mY = 0;
  static uint8_t count = 0;
  TouchPoint p = touchscreen.getTouch();
  x = p.x;
  y = p.y;

  if (p.zRaw < 600) {
    count = 0;
    mX = 0;
    mY = 0;

    //display->drawCircle(x, y, 2, COLOUR_BLACK);
    return false;
  }

  if (mX == 0) mX = x;
  if (mY == 0) mY = y;

  mX = (mX + x) / 2;
  mY = (mY + y) / 2;

  count++;

  x = mX;
  y = mY;

  //display->drawCircle(mX, mY, 2, COLOUR_RED);

  if (count < 3 && filtered) return false;
  //display->drawCircle(mX, mY, 2, COLOUR_GREEN);
  count = 0;
  return true;
}