// vehex.cpp

#include <Arduino.h>
#include "pf.h"
#include "utility.h"
#include "vehex.h"

extern void write_checksum(char *ptr, int size);
extern bool check_checksum(char *ptr, int size);
extern int32_t parse_value(char *msg, size_t size, size_t width, bool *err);

#define MINWIDTH 10

void veHex::add_parser(float *value_p, veID id, size_t width, float multiplier) {
  veHexData *p = new veHexData((char *)value_p,id, width, multiplier);
  // add to list
  veHexData *tmp = veHexData_head_;
  veHexData_head_ = p;
  p->next_ = tmp;
}

void veHex::add_parser_raw(uint32_t *value_p, veID id, size_t width) {
  veHexData *p = new veHexData((char *)value_p, id, width, 0.0);
  // add to list
  veHexData *tmp = veHexData_head_;
  veHexData_head_ = p;
  p->next_ = tmp;
}

int veHex::update() {
  int c;
  int ret;
  //pfln("solar.update available %i", ss_->available());
  while((c = ss_->read()) >= 0) {
    chars_rec_++;
    if (message_index_ == 0) {
      if (c == ':')
        message_[message_index_++] = c;
    }
    else if (message_index_ >= (sizeof(message_) - 1))
      message_index_ = 0;     
    else if (message_index_ > 0) {
      if (c == '\n') {
        // For testing corrupt data every 1000 characters!!!
        //if (chars_rec_%1000 == 0)
        //  message_[3] = ~message_[3];
        ret = parse(message_, message_index_);
        message_index_ = 0;
        return(ret);
      }
      else {
        message_[message_index_++] = c;
        message_[message_index_] = '\0';  // ensure message_ is always null terminated
      }
    }
  }
  return(-1);
}

int veHex::parse(char *msg, size_t size) {
  bool err = 0;
  veID id;
  veCmd cmd;
  uint32_t value;
  veHexData *dp = veHex::veHexData_head_;

  //pf("ve.parse %i ", size); pfixs(msg, size); pf();
  if (msg[0] != ':') {
    //pfln("No colon");
    return(6);
  }
  if (!check_checksum(msg+1, size-1)) {
    //pfln("Bad checksum");
    return(2);
    }
  if (size < MINWIDTH) {
    //pfln("Bad size");
    return(1);
    }
  cmd = (veCmd)msg[1];
  switch(cmd){
  case VE_GET:
  case VE_SET:
    // check flags
    if ((hextoln(msg+6, 2, &err) != 0) || (err)) {
      //pfln("Bad flags");
      return(3);
    }
    // ID
    id = (veID)hextoln_little(msg+2, 4, &err);
    if (err) {
      //pfln("Non hex char");
      return(1);
    }
    while (dp != NULL) {
      if (id == dp->id_) {
        value = parse_value(msg, size, dp->width_, &err);
        if (err) {
          return(1);
        }
        messages_rec_++;
        if (dp->multiplier_ == 0)
          *(int32_t *)dp->value_p_ = value;
        else
          *(float *)dp->value_p_ = (float)value*dp->multiplier_;
        //pfln("ve.parse found id 0x%x raw_value %i %s", id, value, msg);       
        return(0);
        }
      dp = dp->next_;
    }
    return(5);
  case VE_ASYNC:
    return(7);
  default:
    return(4);    // Unknown command
  }   // switch(cmd)
  return(0);
}

char *veHex::make_msg(char *buf, veCmd cmd, veID id, size_t width, uint32_t data) {
  size_t i = 0;
  
  if (width > 8) {
    pfln("make_msg size too big");
    return(NULL);
  }
  //pf("make_msg cmd %c id 0x%x width %i data 0x%lx ", cmd, id, width, data);
  buf[i] = ':'; i++;
  buf[i] = cmd; i++;
  lntohex_little(id, buf+i, 4); i += 4;
  // flags
  buf[i] = '0'; i++;
  buf[i] = '0'; i++;
  if (width) {
    lntohex_little(data, buf+i, width);
    i+= width;
  }
  write_checksum(buf+1, 7 + width); i += 2;
  buf[i] = '\n'; i++;
  buf[i] = '\0';
  //pf("make_msg %s", buf);
  return(buf);
}

// send a message to mppt
void veHex::send_msg(veCmd cmd, veID id, size_t noof_bytes, uint32_t data) {
  char buf[VE_BUFSIZE];
  make_msg(buf, cmd, id, noof_bytes, data);
  messages_sent_++;
  ss_->print(buf);
  //pf("%s", buf);
}

// Sends Get commands to MPPT
void veHex::get() {
  //pfln("ve.get %p send_via %p", get_list_, send_via_);
  // send a set command
  if (send_via_buf_[0] != '\0') {  
    //pf("send_via %s", send_via_buf_);
    ss_->print(send_via_buf_);
    send_via_buf_[0] = '\0';
    return;
  }
  if (get_list_ == NULL) {
    //pfln("ve.get no get_list");
    return;
  }
  //pfln("ve.get send get id 0x%x", get_list_[get_no_]);
  send_msg(VE_GET, get_list_[get_no_], 0, 0);
  get_no_++;
  if (get_list_[get_no_] == VE_NOID)
    get_no_ = 0;
}

// Returns charger mode as a String from a Device State get
const char * veHex::charging_mode_str(int8_t value) {
  switch(value) {
  case 0:
    return("Not charging");
  case 2:
    return("Fault");
  case 3:
    return("Bulk");
  case 4:
    return("Absorption");
  case 5:
    return("Float");
  default:
    return("Unknown");
  }
}

// Returns a data value from a vedirect get msg
// size is the total size of msg and width is the data width, both in characters
int32_t parse_value(char *msg, size_t size, size_t width, bool *err) {
  int32_t value;

  switch (size) {
  case MINWIDTH:       // no data
    break;
  case MINWIDTH + 2:      // 8 bit
  case MINWIDTH + 4:      // 16 bit
  case MINWIDTH + 8:      // 32 bit
    if (size - width != MINWIDTH) {
      //pfln("ve.parse_value invalid size %i width %i", size, width);
      *err = 1;
      break;
    }
    value = hextoln_little(msg+8,width, err);
    if(*err) {
      //pfln("ve.parse_value non hex char");
      break;
    }
    //pfln("ve.parse_value size %i width %i value %i", size, width, value);
    return(value);
  default:
    *err = 1;
    break;
  }
  return(0);
}

#define CHECKSUM 0x55
// This a pretty wierd way to do a checksum
// It adds the first 1 digit hex number to succeding 2 digit hex numbers,
// the checksum is the 2 digit hex number you need to add to get 0x55!
byte checksum(char *ptr, int size) {
  int i;
  //pf("checksum size %i ",size);pfixs(ptr, size);
  byte res = hextoln(ptr, 1);
  for (i = 1;i < size;i += 2) {
    res += hextoln(ptr+i, 2);
  }
  //pfln(" ret 0x%x", res);
  return(res);
}

// write the checksum into the the next 2 characters after the buffer pointed at by ptr
// which points at the first character after :
void write_checksum(char *ptr, int size) {
  byte sum = checksum(ptr, size);
  lntohex(CHECKSUM - sum, ptr+size, 2);
}

// check_checksum - returns 1 if good checksum
bool check_checksum(char *ptr, int size) {
  if (size < 1)
    return(0);
  byte sum = checksum(ptr, size);
  if (sum != CHECKSUM)
    return(0);
  return(1);
}