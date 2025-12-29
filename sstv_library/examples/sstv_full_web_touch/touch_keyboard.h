
#include <Arduino.h>

const char Mobile_KB[4][12] = {
  { 0, 12, '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' },
  { 0, 12, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P' },
  { 1, 11, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L' },
  { 1, 11, 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '/', '?' },
};

class touch_keyboard {

  static const char textLimit = 25;
  char string_buffer[textLimit];

  void draw_kb_button_c(int x, int y, int w, int h, const char c);
  void draw_kb_button(int x, int y, int w, int h, const char* c);

public:
  void make_kb(const char type[][12]);
  char get_key_press();
};