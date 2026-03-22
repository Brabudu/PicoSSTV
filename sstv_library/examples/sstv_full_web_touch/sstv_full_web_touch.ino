//  _  ___  _   _____ _     _
// / |/ _ \/ | |_   _| |__ (_)_ __   __ _ ___
// | | | | | |   | | | '_ \| | '_ \ / _` / __|
// | | |_| | |   | | | | | | | | | | (_| \__ \.
// |_|\___/|_|   |_| |_| |_|_|_| |_|\__, |___/
//                                  |___/
//
// Copyright (c) Jonathan P Dawson 2025
// filename: sstv_decoder.ino
// description:
//
// SSTV Decoder using pi-pico.
//
// Accepts audio on ADC input, and displays on an ILI943x display.
//
//
// License: MIT
//
// BRABUDU
//
//
//
// WIFI:
//
// Configure ssid and password
//
// Enable wifi in settings
// Look at the ip address in the menu title
// Connect from pc while the Pico sstv is in menu mode
//
// Transmission:
//
// Put your pictures (24 bit bmp) in the tx folder
// Configure your callsign in the configuration section above
// Use "reply" for replying to a cq call, then insert the receiver callsign and the rsv. Select the picture and send
// Use "transmit" in the menu section for making a cq call

#include "hardware/spi.h"
#include "ili934x.h"
#include "gfxfont.h"
#include "font_8x5.h"
#include "font_16x12.h"
#include "FreeSansBold24pt7b.h"
#include "sstv_decoder.h"
#include "bmp_classes.h"
#include "ADCAudio.h"
#include "PWMAudio.h"
#include "splash.h"
#include "sstv_encoder.h"
#include "frame_buffer.h"
#include "button.h"
#include "bmp_lib.h"
#include "XPT2046_Bitbang.h"
#include "touch_keyboard.h"

#include <SPI.h>
#include <SDFS.h>
#include <VFS.h>
#include <EEPROM.h>
#include <vector>
#include <string>
#include <algorithm>

#include "main_menu.h"

#if defined(PICO_RP2350)
#define WIFI  //Comment for disabling wifi
#endif

#ifdef WIFI

//Network configuration in file wifi.cpp !!!

#include "wifi_server.h"

wifi_server wifi_s;

#endif


#define CALLSIGN "IS0JSV\0"

//HW CONFIGURATION SECTION
///////////////////////////////////////////////////////////////////////////////

#define PIN_MISO 12  //not used by TFT but part of SPI bus
#define PIN_CS 13
#define PIN_SCK 14
#define PIN_MOSI 15
#define PIN_DC 11
#define SPI_PORT spi1

#define SDCARD_MISO 4
#define SDCARD_MOSI 7
#define SDCARD_CS 5
#define SDCARD_SCK 6

// TOUCH CONTROLLER


#define T_MOSI_PIN 19
#define T_MISO_PIN 16
#define T_CLK_PIN 18
#define T_CS_PIN 17

// Change this for enabling-disabling touch mode at first startup
// You can change it in settings->Touch mode

#define touch_installed

//Comment this if you want only touch

#define buttons_installed

//!!Note, can be quite a bit of variation between TFT displays
//if the display doesn't look right it can be fixed by changing these settings!!

//If the image doesn't fill the display, or is rotated try changing
//the ROTATION.

//#define ROTATION R0DEG
//#define ROTATION R90DEG
//#define ROTATION R180DEG
#define ROTATION R270DEG
//#define ROTATION MIRRORED0DEG
//#define ROTATION MIRRORED90DEG
//#define ROTATION MIRRORED180DEG
//#define ROTATION MIRRORED270DEG

//The splash screen should have blue lettering, if you see red lettering
//try changing the INVERT_COLOURS setting.

//#define INVERT_COLOURS false
#define INVERT_COLOURS true

//The splash screen should have a black background, if you have a white
//background try changing this setting. Many thans to ON4ABR for adding
//this option.
#define INVERT_DISPLAY false
//#define INVERT_DISPLAY true

//Chroma key color for overlay

#define CHROMA 0

//END OF CONFIGURATION SECTION
///////////////////////////////////////////////////////////////////////////////

void draw_banner(const char* message, uint16_t y = 0);
void draw_status_bar(const char* message);

void configure_display();
void initialise_sdcard();
void get_new_filename(char* buffer, uint16_t buffer_size);
int16_t display_image(const char* filename, bool show_overlay = false);
void get_timeout_seconds(const char* title, uint8_t& menu_selection);

uint16_t count_bitmaps(Dir& root);
void get_bitmap_index(Dir& root, uint16_t index);
void create_thumbnail(const char* filename, e_mode mode);
e_sstv_tx_mode convert_mode(e_mode rx_mode);

ILI934X* display;

#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 240
#define STATUS_BAR_HEIGHT 20

XPT2046_Bitbang touchscreen(T_MOSI_PIN, T_MISO_PIN, T_CLK_PIN, T_CS_PIN);
touch_keyboard t_keyboard;


lcd_menu sstv_menu = lcd_menu();

Stream* s;

button button_up(26);  //17
button button_down(20);
button button_right(21);
button button_left(22);


e_view_mode view_mode;

static const uint16_t overlay_width = 320;
static const uint16_t overlay_height = 256;
uint16_t overlay_buffer[overlay_width * overlay_height];
c_frame_buffer overlay(overlay_buffer, overlay_width, overlay_height);

uint16_t scaled_image[214 * 160];
bool first_img_received = false;  //Last image

char rxcallsign_text[10];
char rsv_text[20];

s_settings settings = {
  3,  //5 seconds
  3,  //30 seconds
  2,  // 50%
  1,  //martin m2
  1,  //auto slant correction on
  1,  // overlay on
#ifdef touch_installed
  1,
#else
  0,
#endif
  0,      //wifi off
  8,      //orange
  8,      //orange
  2,      //red
  1,      // tx preamble
  { 0 },  //overlay
  { CALLSIGN }
};

//c_sstv_decoder provides a reusable SSTV decoder
//We need to override some hardware specific functions to make it work with
//ADC audio input and TFT image display
class c_sstv_decoder_fileio : public c_sstv_decoder {
  ADCAudio adc_audio;
  uint16_t tft_row_number = 0;
  float progress = 0;
  e_mode last_mode = martin_m1;

  const uint16_t display_width = DISPLAY_WIDTH;
  const uint16_t display_height = DISPLAY_HEIGHT - STATUS_BAR_HEIGHT;  //allow space for status bar

  //override the get_audio_sample function to read ADC audio
  int16_t get_audio_sample() {
    static int16_t* samples;
    static uint16_t sample_number = ADC_BLOCK;
    //if we reach the end of a block request a new one
    if (sample_number == ADC_BLOCK) {
      //fetch a new block of 1024 samples
      samples = adc_audio.input_samples();
      sample_number = 0;
    }
    //output the next sample in the block
    return samples[sample_number++];
  }

  //override the image_write_line function to output images to a TFT display
  void image_write_line(uint16_t line_rgb565[], uint16_t y, uint16_t width, uint16_t height, e_mode decode_mode) {
    //write unscaled image to bmp file
    output_file.change_width(width);

    //update decode mode
    output_file.change_mode(decode_mode);

    if (++bmp_row_number < height) {
      output_file.change_height(y + 1);
      output_file.write_row_rgb565(line_rgb565);
    }

    //scale image to fit TFT size
    uint16_t scaled_row[display_width];
    uint16_t pixel_number = 0;
    for (uint16_t x = 0; x < width; x++) {
      uint16_t scaled_x = static_cast<uint32_t>(x) * display_width / width;
      while (pixel_number <= scaled_x) {
        //display expects byteswapped data
        scaled_row[pixel_number] = ((line_rgb565[x] & 0xff) << 8) | ((line_rgb565[x] & 0xff00) >> 8);
        pixel_number++;
      }
    }

    uint32_t scaled_y = static_cast<uint32_t>(y) * display_height / height;
    while (tft_row_number <= scaled_y) {
      display->writeHLine(0, tft_row_number, display_width, scaled_row);
      tft_row_number++;
    }

    //update progress
    char buffer[15];
    snprintf(buffer, 15, "%5s %ux%u", rx_modes_abbr[decode_mode], width, y + 1);
    draw_status_bar(buffer);
    //draw_banner(buffer);
    //Serial.println(buffer);

    progress = y / (float)height;
    last_mode = decode_mode;
  }


  void scope(uint16_t mag, int16_t freq) {
    static const uint16_t palette[] = { 0x0000, 0x0011, 0x0023, 0x0024, 0x0043, 0x0044, 0x0044, 0x0044, 0x0044, 0x0044, 0x0044, 0x0045, 0x0045, 0x0065, 0x0065, 0x0065, 0x0065, 0x0066, 0x0066, 0x0066, 0x0066, 0x0086, 0x0087, 0x0087, 0x0087, 0x0087, 0x0088, 0x00a8, 0x00a8, 0x00a8, 0x00a8, 0x00a9, 0x00c9, 0x00c9, 0x00ca, 0x00ca, 0x00ca, 0x00ea, 0x00eb, 0x00eb, 0x00eb, 0x010b, 0x010c, 0x010c, 0x012c, 0x012d, 0x012d, 0x012d, 0x014e, 0x014e, 0x014e, 0x016f, 0x016f, 0x016f, 0x0190, 0x0190, 0x0190, 0x01b1, 0x01b1, 0x01d1, 0x01d2, 0x01d2, 0x01f2, 0x01f3, 0x0213, 0x0213, 0x0214, 0x0234, 0x0234, 0x0255, 0x0255, 0x0276, 0x0276, 0x0296, 0x0297, 0x02b7, 0x02b7, 0x02d8, 0x02d8, 0x02f8, 0x02f9, 0x0319, 0x0319, 0x033a, 0x035a, 0x035a, 0x037b, 0x037b, 0x039b, 0x039c, 0x03bc, 0x03dc, 0x03dc, 0x03fd, 0x03fd, 0x041d, 0x043e, 0x043e, 0x045e, 0x045e, 0x047f, 0x049f, 0x049f, 0x04bf, 0x04df, 0x04df, 0x04ff, 0x051f, 0x051f, 0x053f, 0x053f, 0x055f, 0x057f, 0x057f, 0x059f, 0x05bf, 0x05bf, 0x0ddf, 0x0ddf, 0x0dff, 0x0e1f, 0x0e1f, 0x0e3f, 0x0e3f, 0x0e5f, 0x0e7f, 0x0e7f, 0x0e9f, 0x0e9f, 0x0ebf, 0x0ebf, 0x16df, 0x16df, 0x16ff, 0x171f, 0x171f, 0x173f, 0x173f, 0x173f, 0x175f, 0x1f5f, 0x1f7f, 0x1f7f, 0x1f9f, 0x1f9f, 0x1f9f, 0x27bf, 0x27bf, 0x27df, 0x27df, 0x27df, 0x27ff, 0x2fff, 0x2fff, 0x2fff, 0x2ffe, 0x37fe, 0x37fe, 0x37fe, 0x37fd, 0x3ffd, 0x3ffd, 0x3ffc, 0x3ffc, 0x47fc, 0x47fc, 0x47fb, 0x4ffb, 0x4ffb, 0x4ffa, 0x57fa, 0x57fa, 0x57f9, 0x5ff9, 0x5ff9, 0x5ff8, 0x67f8, 0x67f8, 0x6ff7, 0x6ff7, 0x6ff7, 0x77f6, 0x77f6, 0x7ff6, 0x7ff5, 0x87f5, 0x87f4, 0x8ff4, 0x8ff4, 0x8ff3, 0x97d3, 0x97d3, 0x9fd2, 0x9fb2, 0xa7b2, 0xa791, 0xaf91, 0xaf91, 0xb770, 0xb770, 0xbf50, 0xbf4f, 0xc72f, 0xc72f, 0xcf2e, 0xcf0e, 0xd70e, 0xdeed, 0xdecd, 0xe6cd, 0xe6ac, 0xeeac, 0xee8c, 0xf68b, 0xf66b, 0xfe6b, 0xfe4b, 0xfe2a, 0xfe2a, 0xfe0a, 0xfe0a, 0xfde9, 0xfdc9, 0xfdc9, 0xfda8, 0xfda8, 0xfd88, 0xfd68, 0xfd68, 0xfd47, 0xfd27, 0xfd27, 0xfd07, 0xfd06, 0xfce6, 0xfcc6, 0xfcc6, 0xfca6, 0xfc85, 0xfc85, 0xfc65, 0xfc45, 0xfc45, 0xfc25, 0xfc24, 0xfc04, 0xfbe4, 0xfbe4, 0xfbc4, 0xfbc4, 0xfba3, 0xfb83, 0xfb83, 0xfb63, 0xfb63, 0xfb43 };

    const uint16_t scope_x = 168;
    const uint16_t scope_y = 234;
    const uint16_t scope_width = 150;

    const uint8_t waterfall_amp = 5;

    if (view_mode != rx_mode) return;

    static uint8_t row = 0;
    static uint16_t count = 0;
    static uint32_t spectrum[scope_width];
    static uint32_t signal_strength = 0;
    static uint8_t mean_f = 0;

    const uint8_t f = (freq - 1000) * scope_width / 1500;
    const uint8_t Hz_1200 = (1200 - 1000) * scope_width / 1500;
    const uint8_t Hz_1500 = (1500 - 1000) * scope_width / 1500;
    const uint8_t Hz_2300 = (2300 - 1000) * scope_width / 1500;

    mean_f = (mean_f * 7 + f) / 8;

    if (mean_f > 0 && mean_f < scope_width) {
      spectrum[mean_f] = (spectrum[mean_f] * 15 + mag) / 16;
    }
    signal_strength = (signal_strength * 15 + mag) / 16;
    if (count > 200) {

      uint16_t waterfall[scope_width];
      for (int i = 0; i < scope_width; i++) {
        float scaled_dB = waterfall_amp * 20 * log10(spectrum[i]);
        scaled_dB = std::max(std::min(scaled_dB, 255.0f), 0.0f);
        waterfall[i] = __builtin_bswap16(palette[(int)scaled_dB]);
      }
      waterfall[Hz_1200] = COLOUR_RED;
      waterfall[Hz_1500] = COLOUR_RED;
      waterfall[Hz_2300] = COLOUR_RED;
      display->writeHLine(scope_x, scope_y - 12 + row++, scope_width, waterfall);

      for (int i = 0; i < scope_width; i++) {
        spectrum[i] = spectrum[i] >> 1;
      }

      if (row > 11) row = 0;
      count = 0;

      // Draw signal bar
      float scaled_dB = 2 * 20 * log10(signal_strength);
      scaled_dB = std::max(std::min(scaled_dB, 149.0f), 0.0f);
      display->fillRect(scope_x, scope_y, 2, scaled_dB, COLOUR_YELLOW);
      display->fillRect(scope_x + scaled_dB, scope_y, 2, scope_width - scaled_dB, COLOUR_MAROON);
    }
    count++;
  }

  c_bmp_writer_stdio output_file;
  uint16_t bmp_row_number = 0;

public:

  float getProgress() {
    return progress;
  }

  e_mode getLastMode() {
    return last_mode;
  }

  void open(const char* bmp_file_name) {
    tft_row_number = 0;
    bmp_row_number = 0;
    Serial.print("opening output bmp file: ");
    Serial.println(bmp_file_name);
    output_file.open(bmp_file_name, 10, 10);
  }
  void close() {
    tft_row_number = 0;
    bmp_row_number = 0;
    Serial.println("closing bmp file");
    output_file.update_header();
    output_file.close();
  }
  void start() {
    adc_audio.begin(28, 15000);
  }
  void stop() {
    adc_audio.end();
  }
  c_sstv_decoder_fileio(float fs)
    : c_sstv_decoder{ fs } {}
};

void set_overlay(const char message[]) {
  //Create a background gradient
  for (uint16_t x = 0; x < overlay_width; x++) {
    for (uint16_t y = 0; y < 14; y++) {
      overlay.set_pixel(x, y, overlay.colour565(0, x * 255 / overlay_width, 255));
    }
  }
  uint16_t text_width = strlen(message) * 12;
  overlay.draw_string((overlay_width - text_width) / 2, 4, font_8x5, message, COLOUR_ORANGE);
}

//Derive a class from sstv encoder and override hardware specific functions
const uint16_t audio_buffer_length = 4096u;
class c_sstv_encoder_pwm : public c_sstv_encoder {
  uint16_t audio_buffer[2][audio_buffer_length];
  uint16_t audio_buffer_index = 0;
  uint8_t ping_pong = 0;
  PWMAudio audio_output;
  uint16_t sample_min, sample_max;

  void output_sample(int16_t sample) {
    uint16_t scaled_sample = ((sample + 32767) >> 5);  // + 1024;
    audio_buffer[ping_pong][audio_buffer_index++] = scaled_sample;

    if (audio_buffer_index == audio_buffer_length) {
      audio_output.output_samples(audio_buffer[ping_pong], audio_buffer_length);
      ping_pong ^= 1;
      audio_buffer_index = 0;
      sample_max = scaled_sample;
      sample_min = scaled_sample;
      if (button_right.is_pressed() || sstv_menu.poll_button_bar(sstv_txx_bar, false) == 2) abort();
    } else {
      sample_max = max(sample_max, scaled_sample);
      sample_min = min(sample_min, scaled_sample);
    }
  }

  uint8_t get_image_pixel(uint16_t width, uint16_t height, uint16_t y, uint16_t x, uint8_t colour) {
    uint16_t image_y = (uint32_t)y * image_height / height;
    uint16_t image_x = (uint32_t)x * image_width / width;

    while (image_y >= row_number) {
      bitmap.read_row_rgb565(row);
      row_number++;
      char status[100];
      snprintf(status, 100, "transmitting %u/%u (%u%%)", y + 1, height, (100 * (y + 1)) / height);
      draw_banner(status);
    }
    uint16_t pixel;
    //overlay a text banner
    uint16_t overlay_y = (uint32_t)y * overlay_width / width;
    uint16_t overlay_x = (uint32_t)x * overlay_width / width;
    if (overlay_y < overlay_height) {
      pixel = overlay_buffer[(overlay_y * overlay_width) + overlay_x];
      pixel = (pixel >> 8) | (pixel << 8);
      if (pixel == CHROMA) pixel = row[image_x];
    } else {
      pixel = row[image_x];
    }

    if (colour == 0) return ((pixel >> 11) & 0x1F) << 3;      //r
    else if (colour == 1) return ((pixel >> 5) & 0x3F) << 2;  //g
    else if (colour == 2) return (pixel & 0x1F) << 3;         //b
    else return 0;
  }

  c_bmp_reader_stdio bitmap;
  FILE* pcm;
  uint16_t row_number = 0;
  uint16_t row[640];
  uint16_t image_width, image_height;

public:
  void open(const char* bmp_file_name) {
    bitmap.open(bmp_file_name, image_width, image_height);
    bitmap.read_row_rgb565(row);
    row_number = 0;
    audio_output.begin(0, 15000, rp2040.f_cpu());
  }
  void close() {
    bitmap.close();
    audio_output.end();
  }

  c_sstv_encoder_pwm(double fs_Hz)
    : c_sstv_encoder(fs_Hz) {}
};

class c_slideshow {

private:
  bool redraw = false;
  Dir root;
  uint16_t num_bitmaps = 0;
  uint16_t bitmap_index = 0;
  String filename;
  uint32_t last_update_time = 0;

public:
  void launch_slideshow() {
    root = SDFS.openDir("/");
    num_bitmaps = count_bitmaps(root);
    bitmap_index = num_bitmaps - 2;
    last_update_time = 0;
  }

  void update_slideshow() {
    if (num_bitmaps == 0) return;

    uint8_t touch_button = sstv_menu.poll_button_bar(sstv_sl_bar);

    bool redraw = false;
    static const uint16_t timeouts[] = { 0, 1, 2, 5, 10, 30, 60, 60 * 2, 60 * 5 };
    uint16_t timeout_milliseconds = 1000 * timeouts[settings.slideshow_timeout];

    if (((millis() - last_update_time) > timeout_milliseconds) && (timeout_milliseconds != 0)) {
      last_update_time = millis();
      if (bitmap_index == num_bitmaps - 1) bitmap_index = 0;
      else bitmap_index++;
      redraw = true;
    }
    if (touch_button == 1) {
      view_mode = rx_mode;
      return;
    }
    if (button_right.is_pressed() || touch_button == 2) {

      get_bitmap_index(root, bitmap_index);
      filename = root.fileName();
      SDFS.remove(filename);
      char msg[] = "Deleted ";
      draw_banner(strcat(msg, filename.c_str()));
      bitmap_index = std::min((int)bitmap_index, num_bitmaps - 2);
      root = SDFS.openDir("/");
      num_bitmaps--;
      delay(300);
      if (num_bitmaps == 0) return;
      redraw = true;
    }
    if (button_up.is_pressed() || touch_button == 4) {
      if (bitmap_index == num_bitmaps - 1) bitmap_index = 0;
      else bitmap_index++;
      redraw = true;
    }
    if (button_down.is_pressed() || touch_button == 3) {
      if (bitmap_index == 0) bitmap_index = num_bitmaps - 1;
      else bitmap_index--;
      redraw = true;
    }
    if (redraw) {
      get_bitmap_index(root, bitmap_index);
      filename = root.fileName();
      Serial.println(filename);
      int16_t mode = display_image(filename.c_str());
      uint16_t width = strlen(filename.c_str()) * 6 + 10;
      draw_banner(filename.c_str());
      if (mode >= 0) draw_banner(rx_modes[mode], 200);
      sstv_menu.draw_button_bar(sstv_sl_bar);
      last_update_time = millis();
    }
  }
};

void setup() {
  EEPROM.begin(512);
  Serial.begin(115200);
  Serial.println("Pico SSTV Copyright (C) Jonathan P Dawson 2025");
  Serial.println("github: https://github.com/dawsonjon/101Things");
  Serial.println("docs: 101-things.readthedocs.io");
  pinMode(LED_BUILTIN, OUTPUT);
  configure_display();
  initialise_sdcard();
  VFS.root(SDFS);

  touchscreen.begin();
  s = &Serial;

  load();
  sync_menu();

#ifdef WIFI
  WiFi.mode(WIFI_STA);
  if (settings.wifi) wifi_s.connectToWiFi();
#endif
}

void loop() {


  static uint8_t touch_button = 0;

  c_sstv_decoder_fileio sstv_decoder(15000);
  sstv_decoder.start();
  sstv_decoder.open("temp");
  char rx_filename[100];
  get_new_filename(rx_filename, 100);
  c_slideshow slideshow;
  bool draw = true;
  bool image_in_progress = false;
  bool last_image_in_progress = false;
  bool image_complete = false;
  view_mode = rx_mode;
  draw_blank_screen();
  strncpy(settings.overlay_text, "Pi Pico SSTV", 24);
  load();


  while (1) {

    image_complete = sstv_decoder.decode_image_non_blocking(timeouts[settings.lost_signal_timeout], settings.auto_slant_correction, image_in_progress);

    if ((image_in_progress) && (!last_image_in_progress)) {  //Starting reception
      draw_blank_screen();
      sstv_menu.draw_button_bar(sstv_rx_bar);
    }

    last_image_in_progress = image_in_progress;

    if (image_complete) {
      //Save image
      sstv_decoder.close();
      if (sstv_decoder.getProgress() > completion[settings.min_completion]) {
        SDFS.rename("temp", rx_filename);
        create_thumbnail(rx_filename, sstv_decoder.getLastMode());
        get_new_filename(rx_filename, 100);
        first_img_received = true;
      }
      sstv_decoder.open("temp");
      draw = true;
    } else if (image_in_progress) {  //Receiving
      view_mode = rx_mode;
      if (button_right.is_pressed() || touch_button == 2) {
        touch_button = 0;
        sstv_decoder.stop();
        return;
      }
    } else {  //Idle
      if (button_left.is_pressed() || touch_button == 1) {
        touch_button = 0;
        sstv_menu.launch_menu(&main_menu);
        save();

        if (view_mode == slideshow_mode) {
          slideshow.launch_slideshow();
        }
        if (view_mode == rx_mode) {
          draw_blank_screen();
          touch_button = 0;
          draw = true;
        }
      } else if ((button_right.is_pressed() || touch_button == 2) && (view_mode != slideshow_mode)) {
        // Replying to a call
        touch_button = 0;
        text_entry(rxcallsign_text, 10, "Enter callsign");
        rsv_entry(rsv_text);

        overlay.clear(0);
        if (first_img_received) {
          overlay.draw_image(10, 130, 106, 80, scaled_image);
          overlay.draw_rect(9, 129, 108, 82, COLOUR_WHITE);
          settings.transmit_mode = convert_mode(sstv_decoder.getLastMode());
        }
        delay(500);
        tx_file_browser(true);
        draw = true;
      }
    }
    if (view_mode == slideshow_mode) {

      slideshow.update_slideshow();
      if (view_mode == rx_mode) {  //quitting
        draw = true;
        draw_blank_screen();
      }
    }
    if (view_mode == rx_mode) {

      touch_button = sstv_menu.poll_button_bar(sstv_bar);

      if (draw) {
        sstv_menu.draw_button_bar(sstv_bar);

        display->fillRect(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT - STATUS_BAR_HEIGHT - 1, STATUS_BAR_HEIGHT, DISPLAY_WIDTH / 2, COLOUR_BLACK);
        draw = false;
      }
    }

#ifdef WIFI

    wifi_s.connect();
#endif
  }
  sstv_decoder.stop();
}

uint8_t get_touch_row() {
  static uint8_t last_touch = 0;

  static const uint8_t margin_y = 32;

  TouchPoint touch = touchscreen.getTouch();

  if (touch.zRaw > 600) {
    uint8_t row = (touch.y - margin_y) / 25;
    if (touch.x > 10 && touch.x < 300) {
      if (last_touch != row + 1) {
        last_touch = row + 1;
        delay(100);

        return row + 1;
      }
    }
  }
  last_touch = 0;
  return 0;
}

void draw_splash_screen() {
  display->writeImage(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, splash);
  sleep_ms(1000);
}

void draw_blank_screen() {

  display->clear(COLOUR_NAVY);
  display->drawString((DISPLAY_WIDTH - (12 * strlen("Pico SSTV"))) / 2, 100, font_16x12, "Pico SSTV", COLOUR_GREY, COLOUR_NAVY);
#ifdef WIFI
  display->drawString((DISPLAY_WIDTH - (12 * strlen(WiFi.localIP().toString().c_str()))) / 2, 130, font_16x12, WiFi.localIP().toString().c_str(), COLOUR_GREY, COLOUR_NAVY);
#endif
}

void configure_display() {
  spi_init(SPI_PORT, 62500000);
  gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
  gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
  gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
  gpio_init(PIN_CS);
  gpio_set_dir(PIN_CS, GPIO_OUT);
  gpio_init(PIN_DC);
  gpio_set_dir(PIN_DC, GPIO_OUT);
  display = new ILI934X(SPI_PORT, PIN_CS, PIN_DC, DISPLAY_WIDTH, DISPLAY_HEIGHT);
  display->init(ROTATION, INVERT_COLOURS, INVERT_DISPLAY, ILI9341_2);
  display->powerOn(true);
  display->clear();
  draw_splash_screen();
}

void initialise_sdcard() {

  Serial.print("Initializing SD card...");
  bool sdInitialized = false;
  SDFSConfig c2;
  c2.setAutoFormat(true);
  SDFS.setConfig(c2);

  SPI.setRX(SDCARD_MISO);
  SPI.setTX(SDCARD_MOSI);
  SPI.setSCK(SDCARD_SCK);
  SDFS.setConfig(SDFSConfig(SDCARD_CS, SD_SCK_MHZ(40), SPI));
  sdInitialized = SDFS.begin();

  Serial.println("initialization done.");
}

void get_new_filename(char* buffer, uint16_t buffer_size) {
  static uint16_t serial_number = 0;
  do {
    snprintf(buffer, buffer_size, "sstv_rx_%u.bmp", serial_number);
    serial_number++;
  } while (SDFS.exists(buffer));
}

void draw_banner(const char* message, uint16_t y) {
  uint16_t width = strlen(message) * 6 + 10;
  display->fillRoundedRect((DISPLAY_WIDTH - width) / 2, y, 10, width, 3, 0);
  display->drawString((DISPLAY_WIDTH - width) / 2 + 5, y + 1, font_8x5, message, COLOUR_WHITE, COLOUR_BLACK);
}

void draw_status_bar(const char* message) {
  display->fillRect(0, DISPLAY_HEIGHT - STATUS_BAR_HEIGHT - 1, STATUS_BAR_HEIGHT, DISPLAY_WIDTH / 4, COLOUR_BLACK);
#define MARGIN ((STATUS_BAR_HEIGHT - 8) / 2)
  display->drawString(MARGIN, DISPLAY_HEIGHT - STATUS_BAR_HEIGHT + MARGIN, font_8x5, message, COLOUR_WHITE, COLOUR_BLACK);
}

uint16_t count_bitmaps(Dir& root) {
  uint16_t count = 0;
  root.rewind();
  while (root.next()) {
    String filename = root.fileName();
    if (root.isFile() && filename.endsWith(".bmp")) count++;
  }
  return count;
}

void get_bitmap_index(Dir& root, uint16_t index) {
  uint16_t count = 0;
  root.rewind();
  while (root.next()) {
    String filename = root.fileName();
    if (root.isFile() && filename.endsWith(".bmp")) {
      if (count == index) return;
      count++;
    }
  }
}

void transmit_image(const char* filename) {
  const uint16_t divider = rp2040.f_cpu() / 15000;
  const float sample_rate_Hz = (double)rp2040.f_cpu() / divider;
  c_sstv_encoder_pwm sstv_encoder(sample_rate_Hz);
  sstv_encoder.open(filename);
  digitalWrite(LED_BUILTIN, 1);
  sstv_encoder.generate_sstv((e_sstv_tx_mode)settings.transmit_mode, settings.tx_preamble);
  digitalWrite(LED_BUILTIN, 0);
  sstv_encoder.close();
  draw_blank_screen();
}

e_sstv_tx_mode convert_mode(e_mode rx_mode) {
  switch (rx_mode) {
    case martin_m1:
      return tx_martin_m1;
    case martin_m2:
      return tx_martin_m2;
    case scottie_s1:
      return tx_scottie_s1;
    case scottie_s2:
      return tx_scottie_s2;
    case scottie_dx:
      return tx_scottie_dx;
    case pd_50:
      return tx_PD_50;
    case pd_90:
      return tx_PD_90;
    case pd_120:
      return tx_PD_120;
    case pd_180:
      return tx_PD_180;
    case robot24:
      return tx_robot_24;
    case robot36:
      return tx_robot_36;
    case robot72:
      return tx_robot_72;
    case bw8:
      return tx_bw_8;
    case bw12:
      return tx_bw_12;
    default:
      return tx_martin_m1;
  }
}

e_mode convert_mode(e_sstv_tx_mode tx_mode) {
  switch (tx_mode) {
    case tx_martin_m1:
      return martin_m1;
    case tx_martin_m2:
      return martin_m2;
    case tx_scottie_s1:
      return scottie_s1;
    case tx_scottie_s2:
      return scottie_s2;
    case tx_scottie_dx:
      return scottie_dx;
    case tx_PD_50:
      return pd_50;
    case tx_PD_90:
      return pd_90;
    case tx_PD_120:
      return pd_120;
    case tx_PD_180:
      return pd_180;
    case tx_robot_24:
      return robot24;
    case tx_robot_36:
      return robot36;
    case tx_robot_72:
      return robot72;
    case tx_bw_8:
      return bw8;
    case tx_bw_12:
      return bw12;
    default:
      return martin_m1;
  }
}

void tx_file_browser(bool reply) {
  bool redraw = true;
  Dir root = SDFS.openDir("/tx/");
  const uint16_t num_bitmaps = count_bitmaps(root);
  if (num_bitmaps == 0) return;
  uint16_t bitmap_index = 0;
  String filename;
  uint8_t touch_button = 0;

  uint8_t touch_row;

  bool edit = false;

  while (1) {

    touch_row = sstv_menu.get_touch_row();

    if (touch_row == 8) {
      touch_button = sstv_menu.poll_button_bar(sstv_tx_bar, false);  //Menu bar
    } else {
      touch_button = 0;
    }

    if (!reply && (touch_row == 3 || touch_row == 2)) {
      text_entry(rxcallsign_text, 10, "Enter text");
      rsv_entry(rsv_text);
      edit = true;
    }
    if (touch_row == 1 || touch_row == 2 || touch_row == 3 || touch_row == 6 || touch_row == 7) {
      t_keyboard.make_color_kb();
      delay(500);
      char color;
      do {
        delay(10);
        color = t_keyboard.get_key_press();
      } while (color == '-');
      if (touch_row == 1) settings.color1 = color;
      else if (touch_row == 2 || touch_row == 3) settings.color2 = color;
      else settings.color3 = color;
      save();
      redraw = true;
    }

    if (button_up.is_pressed() || touch_button == 4) {
      if (bitmap_index == num_bitmaps - 1) bitmap_index = 0;
      else bitmap_index++;
      redraw = true;
    }
    if (button_down.is_pressed() || touch_button == 3) {
      if (bitmap_index == 0) bitmap_index = num_bitmaps - 1;
      else bitmap_index--;
      redraw = true;
    }
    if (redraw) {
      get_bitmap_index(root, bitmap_index);
      filename = "/tx/" + root.fileName();

      if (reply) {
        draw_overlay(settings.tx_callsign, rxcallsign_text, rsv_text);
      } else {
        overlay.clear(0);
        if (edit) draw_overlay(settings.tx_callsign, rxcallsign_text, rsv_text);
        else draw_overlay(settings.tx_callsign, "CQ CQ", "");
      }


      if (settings.overlay) set_overlay(settings.overlay_text);
      display_image(filename.c_str(), true);

      draw_banner(rx_modes[convert_mode((e_sstv_tx_mode)settings.transmit_mode)]);
      sstv_menu.draw_button_bar(sstv_tx_bar);
      redraw = false;
    }
    if (button_left.is_pressed() || touch_button == 1) {
      sstv_menu.draw_button_bar(sstv_txx_bar);
      transmit_image(filename.c_str());
      return;
    }
    if (button_right.is_pressed() || touch_button == 2) {
      draw_blank_screen();
      return;
    }
  }
}

int16_t display_image(const char* filename, bool show_overlay) {
  c_bmp_reader_stdio bitmap;
  uint16_t width, height;
  int16_t mode;
  bitmap.open(filename, width, height, mode);

  const uint16_t display_width = DISPLAY_WIDTH, display_height = DISPLAY_HEIGHT - STATUS_BAR_HEIGHT;
  uint16_t tft_row_number = 0;

  for (uint16_t y = 0; y < height; y++) {
    uint16_t line_rgb565[width];
    bitmap.read_row_rgb565(line_rgb565);

    //scale image to fit TFT size
    uint16_t scaled_row[display_width];
    uint16_t pixel_number = 0;
    uint16_t overlay_y = (uint32_t)y * overlay_width / width;

    for (uint16_t x = 0; x < width; x++) {
      uint16_t scaled_x = (static_cast<uint32_t>(x) * display_width + (display_width / 2)) / width;
      uint16_t overlay_x = (uint32_t)x * overlay_width / width;
      while (pixel_number <= scaled_x) {
        //display expects byteswapped data
        uint16_t pixel = overlay_buffer[(overlay_y * overlay_width) + overlay_x];
        if (pixel == CHROMA || !show_overlay) pixel = ((line_rgb565[x] & 0xff) << 8) | ((line_rgb565[x] & 0xff00) >> 8);
        scaled_row[pixel_number] = pixel;
        pixel_number++;
      }
    }

    uint32_t scaled_y = (static_cast<uint32_t>(y) * display_height + (display_height / 2)) / height;
    while (tft_row_number <= scaled_y) {
      display->writeHLine(0, tft_row_number, display_width, scaled_row);
      tft_row_number++;
    }
  }

  bitmap.close();
  return mode;
}

void draw_outlined_big(uint16_t x, uint16_t y, String msg, uint16_t fg, uint16_t bg) {
  overlay.draw_string(x - 2, y - 2, &FreeSansBold24pt7b, msg.c_str(), bg);
  overlay.draw_string(x + 2, y - 2, &FreeSansBold24pt7b, msg.c_str(), bg);
  overlay.draw_string(x - 2, y + 2, &FreeSansBold24pt7b, msg.c_str(), bg);
  overlay.draw_string(x + 2, y + 2, &FreeSansBold24pt7b, msg.c_str(), bg);
  overlay.draw_string(x, y, &FreeSansBold24pt7b, msg.c_str(), fg);
}
void draw_outlined_small(uint16_t x, uint16_t y, String msg, uint16_t fg, uint16_t bg) {
  //delta y
  y -= 20;
  overlay.draw_string(x - 1, y - 2, font_16x12, msg.c_str(), bg);
  overlay.draw_string(x + 1, y - 2, font_16x12, msg.c_str(), bg);
  overlay.draw_string(x - 1, y + 2, font_16x12, msg.c_str(), bg);
  overlay.draw_string(x + 1, y + 2, font_16x12, msg.c_str(), bg);
  overlay.draw_string(x, y, font_16x12, msg.c_str(), fg);
}


void draw_overlay(String callsignSender, String callsignReceiver, String msg) {
  draw_outlined_big(20, 60, callsignReceiver, palette[settings.color1], COLOUR_WHITE);

  if (msg.length() > 5) draw_outlined_small(40, 110, msg, palette[settings.color2], COLOUR_WHITE);
  else draw_outlined_big(40, 110, msg, palette[settings.color2], COLOUR_WHITE);

  draw_outlined_big(300 - callsignSender.length() * 29, 220, callsignSender, palette[settings.color3], COLOUR_WHITE);
}

button* buttons[] = { &button_left, &button_right, &button_down, &button_up };
static char char_select[4][4][4] = {
  {
    { 'A', 'B', 'C', 'D' },
    { 'E', 'F', 'G', 'H' },
    { ' ', ' ', ' ', '_' },
    { '<', '>', '#', '!' },
  },
  {
    { 'I', 'J', 'K', 'L' },
    { 'M', 'N', 'O', 'P' },
    { 'Q', ' ', ' ', '_' },
    { '<', '>', '#', '!' },
  },
  {
    { 'R', 'S', 'T', 'U' },
    { 'V', 'W', 'X', 'Y' },
    { 'Z', ' ', ' ', '_' },
    { '<', '>', '#', '!' },
  },
  {
    { '0', '1', '2', '3' },
    { '4', '5', '6', '7' },
    { '8', '9', ' ', '_' },
    { '<', '>', '#', '!' },
  }
};

uint8_t get_char0() {
  display->fillRect(0, 120, 120, 320, COLOUR_BLACK);
  for (uint8_t i = 0; i < 4; i++) {
    for (uint8_t j = 0; j < 4; j++) {
      char disp[20];
      snprintf(disp, 20, " %c%c%c%c", char_select[i][j][0], char_select[i][j][1], char_select[i][j][2], char_select[i][j][3]);
      display->drawString(10 + i * 75, 120 + j * 20, font_16x12, disp, COLOUR_WHITE, COLOUR_BLACK);
    }
  }

  while (1) {
    for (uint8_t i = 0; i < 4; i++) {
      if (buttons[i]->is_pressed()) return i;
    }
  }
}

uint8_t get_char1(uint8_t sel1) {
  display->fillRect(0, 120, 120, 320, COLOUR_BLACK);
  for (uint8_t i = 0; i < 4; i++) {
    char disp[20];
    snprintf(disp, 20, " %c%c%c%c", char_select[sel1][i][0], char_select[sel1][i][1], char_select[sel1][i][2], char_select[sel1][i][3]);
    display->drawString(10 + i * 75, 120, font_16x12, disp, COLOUR_WHITE, COLOUR_BLACK);
  }
  while (1) {
    for (uint8_t i = 0; i < 4; i++) {
      if (buttons[i]->is_pressed()) return i;
    }
  }
}

uint8_t get_char2(uint8_t sel0, uint8_t sel1) {
  display->fillRect(0, 120, 120, 320, COLOUR_BLACK);
  if (sel1 == 3) {
    display->drawString(16, 120, font_16x12, " LEFT RIGHT ENTER CLEAR", COLOUR_WHITE, COLOUR_BLACK);
  } else {
    for (uint8_t i = 0; i < 4; i++) {
      char disp[20];
      snprintf(disp, 20, "%c", char_select[sel0][sel1][i]);
      display->drawString(10 + i * 75, 120, font_16x12, disp, COLOUR_WHITE, COLOUR_BLACK);
    }
  }
  while (1) {
    for (uint8_t i = 0; i < 4; i++) {
      if (buttons[i]->is_pressed()) return i;
    }
  }
}

void text_entry(char string[], uint8_t n, const char* title) {
  uint8_t cursor = 0;

  display->clear(COLOUR_BLACK);
  if (settings.touch) {
    t_keyboard.make_kb();
    display->drawString(70, 220, font_16x12, title, COLOUR_YELLOW, COLOUR_BLACK);
  }

  while (1) {
    char entry;
    if (!settings.touch) {
      display->clear(COLOUR_BLACK);
      display->drawRect((DISPLAY_WIDTH - (n * 12)) / 2, 20, 16, n * 12, COLOUR_NAVY);
      display->drawString((DISPLAY_WIDTH - (n * 12)) / 2, 20, font_16x12, string, COLOUR_WHITE, COLOUR_NAVY);
      display->drawRect((DISPLAY_WIDTH - (n * 12)) / 2 + cursor * 12, 20, 16, 12, COLOUR_RED);
      uint8_t sel0 = get_char0();
      uint8_t sel1 = get_char1(sel0);
      uint8_t sel2 = get_char2(sel0, sel1);
      entry = char_select[sel0][sel1][sel2];
    } else {
      display->fillRect((DISPLAY_WIDTH - (n * 12)) / 2, 20, 16, n * 12, COLOUR_NAVY);
      display->drawString((DISPLAY_WIDTH - (n * 12)) / 2, 20, font_16x12, string, COLOUR_WHITE, COLOUR_NAVY);
      do {
        entry = t_keyboard.get_key_press();
      } while (entry == '-');
      delay(200);
    }
    if (entry == '<') {
      cursor--;
      if (settings.touch) string[cursor] = 0;
    } else if (entry == '>') {
      cursor++;
    } else if (entry == '#') {
      return;
    } else if (entry == '!') {
      for (uint8_t i = 0; i < n; i++) string[i] = 0;
      cursor = 0;
    } else if (entry == '_') {
      string[cursor++] = ' ';
    } else {
      string[cursor++] = entry;
    }
    if (cursor == n) return;
    cursor %= n;
  }
}

void rsv_entry(char string[]) {
  uint8_t cursor = 0;
  uint8_t n = 3;

  display->clear(COLOUR_BLACK);

  if (!settings.touch) {
    sstv_menu.draw_button_bar(sstv_text_bar);
    display->drawString((DISPLAY_WIDTH - (18 * 12)) / 2, 90, font_16x12, "Select RSV (or 73)", COLOUR_YELLOW, COLOUR_BLACK);
    while (1) {

      display->drawRect((DISPLAY_WIDTH - (n * 12)) / 2, 120, 16, n * 12, COLOUR_NAVY);
      display->drawString((DISPLAY_WIDTH - (n * 12)) / 2, 120, font_16x12, string, COLOUR_WHITE, COLOUR_NAVY);
      display->drawRect((DISPLAY_WIDTH - (n * 12)) / 2 + cursor * 12, 120, 16, 12, COLOUR_RED);

      if (button_down.is_pressed()) string[cursor]++;
      if (button_up.is_pressed()) string[cursor]--;
      if (button_left.is_pressed()) cursor--;
      if (button_right.is_pressed()) cursor++;

      if (cursor < 0) cursor = 0;
      if (string[cursor] < ' ') string[cursor] = ' ';
      else if (string[cursor] == '/') string[cursor] = ' ';
      else if (string[cursor] == '!') string[cursor] = '0';
      else if (string[cursor] > '9') string[cursor] = '9';
      if (cursor == n) return;
      cursor %= n;
      delay(10);
    }
  } else {
    char value;

    const char* rsv[] = {
      "595",
      "575",
      "553",
      "475",
      "473",
      "453",
      "73 ",
      "txt",
      ""
    };
    display->drawString(50, 220, font_16x12, "Enter RSV, 73 or text", COLOUR_YELLOW, COLOUR_BLACK);

    t_keyboard.make_rsv_kb(rsv, 8);
    do {
      value = t_keyboard.get_key_press();
    } while (value == '-');

    if (value == 7) {
      text_entry(string, 15, "Enter text");
    } else {
      rsv[3] = 0;
      strcpy(string, rsv[(int)value]);
    }
  }
}
#define version 104

void save() {
  EEPROM.put(2, settings);
  uint16_t scores_stored = 0;
  EEPROM.get(0, scores_stored);
  if (scores_stored != version) EEPROM.put(0, version);
  EEPROM.commit();
}

void load() {
  uint16_t scores_stored = 0;
  EEPROM.get(0, scores_stored);
  if (scores_stored == version) EEPROM.get(2, settings);
}

void create_thumbnail(const char* filename, e_mode mode) {
  c_bmp_reader_stdio bitmap;
  uint16_t width, height;

  bitmap.open(filename, width, height);
  float step_x;
  float step_y;

  switch (mode) {
    case pd_120:
    case pd_180:
      step_x = 6;
      step_y = 6;
      break;
    case bw8:
    case robot24:
      step_x = 1.5;
      step_y = 1.5;
      break;
    default:
      step_x = 3;
      step_y = 3;
      break;
  }

  //todo: mode depedent scale

  const uint16_t t_width = width / step_x, ty_height = height / step_y;
  uint16_t tft_row_number = 0;

  for (uint16_t y = 0; y < height; y++) {
    uint16_t line_rgb565[width];
    bitmap.read_row_rgb565(line_rgb565);

    uint16_t pixel_number = 0;

    for (uint16_t x = 0; x < width; x++) {

      //while(pixel_number <= x) {
      //display expects byteswapped data
      uint16_t pixel = ((line_rgb565[x] & 0xff) << 8) | ((line_rgb565[x] & 0xff00) >> 8);
      uint16_t scaled_x = x / step_x;
      uint16_t scaled_y = y / step_y;
      scaled_image[scaled_x + scaled_y * t_width] = pixel;
      // pixel_number++;
    }
  }

  bitmap.close();
}

void poll_news() {
#ifdef WIFI
  wifi_s.poll_wifi_client();
#endif
}
