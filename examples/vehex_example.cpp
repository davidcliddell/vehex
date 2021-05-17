// example main for vehex

#include "pf.h"   // TODO remove
#include "vehex.h"

// Setup stream to read and write device
#ifdef ESP8266
SoftwareSerial s0;
Stream *hex_serial = &s0;
#else
Stream *hex_serial = &Serial1;
#endif

// places to store parsed data
float ve_panel_voltage = 0;
float ve_panel_power = 0;
float ve_current = 0;
float ve_voltage = 0;
float ve_yield_today = 0;
uint32_t ve_charging_mode = 0;
uint32_t ve_charger_mode = 0;
uint32_t ve_remote = 0;

// List of veIDs for get to send
veID hex_get_list[] = { VE_CHARGER_CURRENT, VE_CHARGER_VOLTAGE, VE_NOID };
veHex *ve_hex;

void setup() {
// Set console to 115200baud
  Serial.begin(115200);
// Set serial to 19200baud
#ifdef ESP8266
  s0.begin(19200, SWSERIAL_8N1, RX0PIN, TX0PIN, 0, RXBUFSIZE, RXBITBUFSIZE);
#else
  Serial1.begin(19200, SERIAL_8N1, 12, 13);     // Set to Rx and Tx pin nos for your board
#endif
  delay(5000);      // wait so can see all debugs
  Serial.println("Starting");
  // create a veHex object and give it the stream to use
  ve_hex = new veHex(hex_serial);
  // Add some commands to parse
  ve_hex->add_parser(&ve_panel_voltage, VE_PANEL_VOLTAGE, 4, 0.01);
  ve_hex->add_parser(&ve_panel_power, VE_PANEL_POWER, 8, 0.01);
  ve_hex->add_parser(&ve_current, VE_CHARGER_CURRENT, 4, 0.1);
  ve_hex->add_parser(&ve_voltage, VE_CHARGER_VOLTAGE, 4, 0.01);
  ve_hex->add_parser(&ve_yield_today, VE_YIELD_TODAY, 4, 0.01*1000*3600);
  ve_hex->add_parser_raw(&ve_charging_mode, VE_DEVICE_STATE, 2);
  ve_hex->add_parser_raw(&ve_charger_mode, VE_DEVICE_MODE, 2);
  ve_hex->add_parser_raw(&ve_remote, VE_REMOTE_CONTROL_USED, 8);
  ve_hex->get_list(&hex_get_list[0]);
  Serial.println("Added parsers");
}

void loop() {
  int ret;
  static unsigned long last_get = 0;
  static unsigned long last_send_via = 0;
  static bool charger_on = 0;

  if ((millis() - last_get) > 1000) {
    // send a get every second
    ve_hex->get();
    last_get = millis();
  }
  if ((millis() - last_send_via) > 10000) {
    // send a charger on/off command every 10 seconds
    Serial.print("Message stats ");
    Serial.print(ve_hex->chars_rec()); Serial.print(" ");
    Serial.print(ve_hex->messages_rec());  Serial.print(" ");
     Serial.print(ve_hex->messages_sent());  Serial.println(" ");
    Serial.println("Charger on/off");
    ve_hex->send_via(VE_SET, VE_DEVICE_MODE, 2, charger_on ? VE_CHARGER_OFF:VE_CHARGER_ON);
    charger_on = !charger_on;
    last_send_via = millis();
  }
  ret = ve_hex->update();
  if (ret == 0) {
    // update has found a valid message and parsed it
    Serial.print("Current is "); Serial.print(ve_current); Serial.println("A");
  }
  else if ((ret != -1) && (ret != 7)){
    // update has found an invalid message
    // print return value and the message
    Serial.print("Invalid message ret "); Serial.print(ret);
    Serial.print(" |"); Serial.println(ve_hex->last_msg());
  }
}