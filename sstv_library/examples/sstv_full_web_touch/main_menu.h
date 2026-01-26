
#include "lcd_menu.h"


static const uint16_t timeouts[] = { UINT16_MAX, 1, 2, 5, 10, 30, 60, 60 * 2, 60 * 5 };
static const float completion[] = { 0.9, 0.75, 0.5 };

///////////////////////////////////

struct s_settings {
  uint8_t slideshow_timeout;
  uint8_t lost_signal_timeout;
  uint8_t min_completion;
  uint8_t transmit_mode;
  uint8_t auto_slant_correction;
  uint8_t overlay;
  uint8_t touch;
  uint8_t wifi;
  uint8_t color1;
  uint8_t color2;
  uint8_t color3;
  uint8_t tx_preamble;

  char overlay_text[25];
  char tx_callsign[25];
};

extern s_settings settings;

bool tx_mode(menu_item m) {
  settings.transmit_mode = m.id;
  return true;
}

bool tx_preamble(menu_item m) {
  settings.tx_preamble = m.id;
  return false;
}

bool autoslant(menu_item m) {
  settings.auto_slant_correction = m.state;
  return false;
}

bool ov_top_bar(menu_item m) {
  settings.overlay = m.state;
  return false;
}

/////////////////////

extern void text_entry(char string[], uint8_t n, const char* title);

bool tx_callsign(menu_item m) {
  text_entry(settings.tx_callsign, 10,"Enter your callsign");
  return true;
}


/////////////////////

extern void reconnectWiFiAndClient();
extern void disconnectWiFi();

bool wifi(menu_item m) {
  settings.wifi = m.state;
  if (settings.wifi) {
    reconnectWiFiAndClient();
  } else {
    disconnectWiFi();
  }
  return false;
}

bool ls_timeout(menu_item m) {
  settings.lost_signal_timeout = m.id;
  return true;
}

bool sl_timeout(menu_item m) {
  settings.slideshow_timeout = m.id;
  return true;
}

bool im_save(menu_item m) {
  settings.min_completion = m.id;
  return true;
}

///////////////////////////////

menu_item ls_timeout_items[] = {
  MENU_ITEM(0, "Never", false, &ls_timeout, NULL),
  MENU_ITEM(1, "1 second", false, &ls_timeout, NULL),
  MENU_ITEM(2, "2 seconds", false, &ls_timeout, NULL),
  MENU_ITEM(3, "5 seconds", false, &ls_timeout, NULL),
  MENU_ITEM(4, "10 seconds", false, &ls_timeout, NULL),
  MENU_ITEM(5, "30 seconds", false, &ls_timeout, NULL),
  MENU_ITEM(6, "1 minute", false, &ls_timeout, NULL),
  MENU_ITEM(7, "5 minutes", false, &ls_timeout, NULL),
  MENU_ITEM(8, "10 minutes", false, &ls_timeout, NULL)
};

menu_list ls_timeout_menu = MENU_LIST(0, "Lost signal timeout", 9, true, ls_timeout_items);

///////////////////////////////

menu_item sl_timeout_items[] = {
  MENU_ITEM(0, "Never", false, &sl_timeout, NULL),
  MENU_ITEM(1, "1 second", false, &sl_timeout, NULL),
  MENU_ITEM(2, "2 seconds", false, &sl_timeout, NULL),
  MENU_ITEM(3, "5 seconds", false, &sl_timeout, NULL),
  MENU_ITEM(4, "10 seconds", false, &sl_timeout, NULL),
  MENU_ITEM(5, "30 seconds", false, &sl_timeout, NULL),
  MENU_ITEM(6, "1 minute", false, &sl_timeout, NULL),
  MENU_ITEM(7, "5 minutes", false, &sl_timeout, NULL),
  MENU_ITEM(8, "10 minutes", false, &sl_timeout, NULL)

};

menu_list sl_timeout_menu = MENU_LIST(0, "Slideshow timeout", 9, true, sl_timeout_items);

/////////////////////////////////////////

menu_item im_save_items[] = {
  MENU_ITEM(0, "90%", false, &im_save, NULL),
  MENU_ITEM(1, "75%", false, &im_save, NULL),
  MENU_ITEM(2, "50%", false, &im_save, NULL),
};

menu_list im_save_menu = MENU_LIST(0, "Min % to save", 3, true, im_save_items);

menu_item tx_mode_items[] = {
  MENU_ITEM(0, "Martin M1", false, &tx_mode, NULL),
  MENU_ITEM(1, "Martin M2", false, &tx_mode, NULL),
  MENU_ITEM(2, "Scottie S1", false, &tx_mode, NULL),
  MENU_ITEM(3, "Scottie S2", false, &tx_mode, NULL),
  MENU_ITEM(4, "Scottie DX", false, &tx_mode, NULL),
  MENU_ITEM(5, "PD 50", false, &tx_mode, NULL),
  MENU_ITEM(6, "PD 90", false, &tx_mode, NULL),
  MENU_ITEM(7, "PD 120", false, &tx_mode, NULL),
  MENU_ITEM(8, "PD 180", false, &tx_mode, NULL),
  MENU_ITEM(9, "Robot 24", false, &tx_mode, NULL),
  MENU_ITEM(10, "Robot 36", false, &tx_mode, NULL),
  MENU_ITEM(11, "Robot 72", false, &tx_mode, NULL),
  MENU_ITEM(12, "Robot B&W 8", false, &tx_mode, NULL),
  MENU_ITEM(13, "Robot B&W 12", false, &tx_mode, NULL),
  MENU_ITEM(14, "Robot B&W 24", false, &tx_mode, NULL),
  MENU_ITEM(15, "Robot B&W 36", false, &tx_mode, NULL)
};

menu_list tx_mode_menu = MENU_LIST(0, "Tx mode", 16, true, tx_mode_items);

/////////////////////////////////////////

menu_item settings_items[] = {
  MENU_ITEM(0, "Wi-fi", true, &wifi, NULL),
  MENU_ITEM(1, "Auto slant", true, &autoslant, NULL),
  MENU_ITEM(2, "Transmit mode", false, NULL, &tx_mode_menu),
  MENU_ITEM(3, "Min save %", false, NULL, &im_save_menu),
  MENU_ITEM(4, "Lost sig. timeout", false, NULL, &ls_timeout_menu),
  MENU_ITEM(5, "Slideshow timeout", false, NULL, &sl_timeout_menu),
  MENU_ITEM(6, "Tx preamble", true, &tx_preamble, NULL),
  MENU_ITEM(7, "Top bar overlay", true, &ov_top_bar,NULL),
  MENU_ITEM(8, "Set callsign", false, &tx_callsign,NULL)

};

menu_list settings_menu = MENU_LIST(0, "Settings", 9, false, settings_items);

/////////////////////////////////////////////

enum e_view_mode { rx_mode,
                   slideshow_mode };

extern e_view_mode view_mode;

bool im_slideshow(menu_item m) {
  view_mode = slideshow_mode;
  return true;
}

/////////////////////////////////////////////
extern void tx_file_browser(bool reply);

bool tx_file(menu_item m) {
  tx_file_browser(false);
  return true;
}


////////////////

menu_item main_items[] = {
  MENU_ITEM(0, "Transmit", false, &tx_file, NULL),
  MENU_ITEM(1, "Slideshow", false, &im_slideshow, NULL),
  MENU_ITEM(2, "Settings", false, NULL, &settings_menu)
};

menu_list main_menu = MENU_LIST(0, "Main Menu", 3, false, main_items);

///////////////////////////////////////

///////////////////
void sync_menu() {
  settings_items[0].state = settings.wifi;
  settings_items[1].state = settings.auto_slant_correction;
  settings_items[6].state = settings.tx_preamble;
  settings_items[7].state = settings.overlay;
  tx_mode_items[settings.transmit_mode].state = true;
  im_save_items[settings.min_completion].state = true;
  sl_timeout_items[settings.slideshow_timeout].state = true;
  ls_timeout_items[settings.lost_signal_timeout].state = true;
}


//////////////////

bar_item sstv_bar_items[] = {
  BAR_ITEM(0, "Menu", true),
  BAR_ITEM(1, "Reply", true),
};

bar_menu sstv_bar = { 2, sstv_bar_items };

//////////////////

bar_item sstv_sl_bar_items[] = {
  BAR_ITEM(0, "Menu", true),
  BAR_ITEM(1, "Delete", true),
  BAR_ITEM(2, "Prev", true),
  BAR_ITEM(3, "Next", true)
};

bar_menu sstv_sl_bar = { 4, sstv_sl_bar_items };

//////////////////

bar_item sstv_tx_bar_items[] = {
  BAR_ITEM(0, "Transmit", true),
  BAR_ITEM(1, "Cancel", true),
  BAR_ITEM(2, "Prev", true),
  BAR_ITEM(3, "Next", true)
};

bar_menu sstv_tx_bar = { 4, sstv_tx_bar_items };
//////////////////

bar_item sstv_txx_bar_items[] = {
  BAR_ITEM(0, "Transmit", false),
  BAR_ITEM(1, "Cancel", true),
  BAR_ITEM(2, "Prev", false),
  BAR_ITEM(3, "Next", false)
};

bar_menu sstv_txx_bar = { 4, sstv_txx_bar_items };

///////////////////////
bar_item sstv_rx_bar_items[] = {
  BAR_ITEM(0, "", false),
  BAR_ITEM(1, "Stop", true),
};

bar_menu sstv_rx_bar = { 2, sstv_rx_bar_items };

///////////////////////

bar_item sstv_text_bar_items[] = {
  BAR_ITEM(0, "<", true),
  BAR_ITEM(1, ">", true),
  BAR_ITEM(2, "+", true),
  BAR_ITEM(3, "-", true)
};

bar_menu sstv_text_bar = { 4, sstv_text_bar_items };
