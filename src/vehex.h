// Header for Victron ve.direct Hex devices
#pragma once

#include <Arduino.h>

#define VE_BUFSIZE 20     // the size of buffer you need to send or receive any ve hex message

// Command
enum veCmd{
  VE_BOOT = '0',
  VE_PING = '1',
  VE_VERSION = '3',
  VE_PRODUCT = '4',
  VE_RESTART = '6',
  VE_GET = '7',
  VE_SET = '8',
  VE_ASYNC = 'A',
};

// ID
enum veID {
  VE_NOID = 0,
  VE_CHARGER_CURRENT =  0xEDD7,
  VE_CHARGER_VOLTAGE =  0xEDD5,
  VE_PANEL_VOLTAGE =    0xEDBB,
  VE_PANEL_POWER =      0xEDBC,
  VE_PANEL_CURRENT =    0xEDBD, // Not available on 10,15 and 20A mppts!
  VE_YIELD_TODAY =      0xEDD3,
  VE_DEVICE_MODE =      0x0200,
  VE_DEVICE_STATE =     0x0201,
  VE_REMOTE_CONTROL_USED = 0x0202,
};

// Device mode
enum veDeviceMode {
  VE_CHARGER_ON =   1,
  VE_CHARGER_OFF =  4,
};

// Remote control used
enum veRemoteControlUsed {
  VE_ENABLE_REMOTE_CONTROL  = 0x00000002,
};

// Device state
enum veDeviceState {
  VE_NOT_CHARGING = 0,
  VE_FAULT = 2,
  VE_BULK = 3,
  VE_ABSORPTION = 4,
  VE_FLOAT = 5,
};

/*
ve.direct HEX protocol is a send respond protocol.

If HEX messages are not sent to the device it will send all it data every second
using the ve.direct format. To change to HEX mode you must send it a
valid HEX command and keep sending otherwise it reverts to continously sending.
This can be done by calling get periodically or with send_msg.

Methods

veHex - constructor
'serial' is a ptr to stream which sends and receives characters from the device. It must be
set to 19200 baud.

update - reads from the device and when a full message found it is sent to 'parse'.
It should be called as often as possible to stop overflows in serial receive buffer.
Returns the return value from 'parse' if a message found or -1 if no valid message so far.

add_parser - adds a parser for the command 'id'
'value_p' is the address of the variable to store the value in.
'width' is the width in hex characters of the data (8bit is 2, 16 bit 4 and 32bit 8).
'multiplier' converts from the ve.direct value to SI units to store in 'value_p'

add_parser_raw - adds a parser for command 'id' which stores the raw value from the device
Note you can't have both add_parser and add_parser_raw for the same command.

parse - parse ve.direct HEX messages
'msg' should start with the ':' and end with the last character of the checksum (strip the \n).
'size' is the length of the 'msg' in bytes
Returns 0 if valid message, 1 non hex or other protocol error, 2 checksum error, 3 bad flag,
4 unknown command, 5 unknown ID, 6 no colon, 7 async command.
The parsed data is stored in the address supplied in 'add_parser.'

last_msg - returns last message parsed or being assembled (for debugging)

send_msg - send a message to the device, 'width' is the size of data in bytes (1,2 or 4).
Note for VE_GET width and data should be 0.

get_list - 'list' is an array of veIDs for 'get' to send to the device

get - sends 'get_list' messages to the device
'get' should be called periodically and interates though 'get_list' sending one on each get call.

send_via - sends a get/set command via 'get'
'msg' is sent on the next 'get' call. If another 'send_via' call is made before the
next 'get' it will overwrite the previous 'send_via' call.

send_via_busy - returns 1 if send_via is busy else 0.

charger_mode_str - return a String from the value from a VE_DEVICE_STATE get.

chars_rec, messages_rec, messages_sent - returns no of ... recieved or sent, each call resets count.
*/

class veHexData {
 friend class veHex;
 public:
  veHexData(char *value_p, veID id, size_t width, float multiplier)
    { id_=id; width_ = width, multiplier_ = multiplier; value_p_ = value_p; }
 private:
  uint16_t id_;
  size_t width_;
  float multiplier_;
  veHexData *next_ = NULL;
  char *value_p_;
};

class veHex {
 friend class veHexData;
 public:
  veHex(Stream *serial) { ss_ = serial; send_via_buf_[0] = '\0'; }
  void add_parser(float *value_p, veID id, size_t width, float multiplier);
  void add_parser_raw(uint32_t *value_p, veID id, size_t width);
  int update();
  int parse(char *msg, size_t size);
  char *last_msg() { return(message_); }
  void send_msg(veCmd cmd, veID id, size_t width, uint32_t data);
  void get_list(veID list[]) { get_list_ = &list[0]; } 
  void get();
  void send_via(veCmd cmd, veID id, size_t width = 0, uint32_t data = 0)
    { make_msg(send_via_buf_, cmd, id, width, data); }
  bool send_via_busy() { return(send_via_buf_[0] != '\0'); }
  const char *charging_mode_str(int8_t value);
  size_t chars_rec() { size_t tmp = chars_rec_; chars_rec_ = 0; return(tmp); }
  size_t messages_rec() { size_t tmp = messages_rec_; messages_rec_ = 0; return(tmp); }
  size_t messages_sent() { size_t tmp = messages_sent_; messages_sent_ = 0; return(tmp); }
  void debug() { pfln("veHex: get_no %i index %i ",get_no_, message_index_); pfixs(message_, message_index_); }
private:
  char *make_msg(char *buf, veCmd cmd, veID id, size_t width, uint32_t data);
  Stream *ss_;
  veID *get_list_ = NULL;
  size_t get_no_ = 0;
  char send_via_buf_[VE_BUFSIZE];
  char message_[VE_BUFSIZE];
  size_t message_index_ = 0;
  size_t chars_rec_ = 0;
  size_t messages_rec_ = 0;     // valid messages received
  size_t messages_sent_ = 0;
  veHexData *veHexData_head_ = NULL;
};
