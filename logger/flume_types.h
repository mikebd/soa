/**
 * Autogenerated by Thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 */
#ifndef flume_TYPES_H
#define flume_TYPES_H

#include <Thrift.h>
#include <TApplicationException.h>
#include <protocol/TProtocol.h>
#include <transport/TTransport.h>





struct Priority {
  enum type {
    FATAL = 0,
    ERROR = 1,
    WARN = 2,
    INFO = 3,
    DEBUG = 4,
    TRACE = 5
  };
};

extern const std::map<int, const char*> _Priority_VALUES_TO_NAMES;

struct EventStatus {
  enum type {
    ACK = 0,
    COMMITED = 1,
    ERR = 2
  };
};

extern const std::map<int, const char*> _EventStatus_VALUES_TO_NAMES;

typedef int64_t Timestamp;

typedef struct _ThriftFlumeEvent__isset {
  _ThriftFlumeEvent__isset() : timestamp(false), priority(false), body(false), nanos(false), host(false), fields(false) {}
  bool timestamp;
  bool priority;
  bool body;
  bool nanos;
  bool host;
  bool fields;
} _ThriftFlumeEvent__isset;

class ThriftFlumeEvent {
 public:

  static const char* ascii_fingerprint; // = "BC13FFB3246A0179557F7F60C181A557";
  static const uint8_t binary_fingerprint[16]; // = {0xBC,0x13,0xFF,0xB3,0x24,0x6A,0x01,0x79,0x55,0x7F,0x7F,0x60,0xC1,0x81,0xA5,0x57};

  ThriftFlumeEvent() : timestamp(0), body(""), nanos(0), host("") {
  }

  virtual ~ThriftFlumeEvent() throw() {}

  Timestamp timestamp;
  Priority::type priority;
  std::string body;
  int64_t nanos;
  std::string host;
  std::map<std::string, std::string>  fields;

  _ThriftFlumeEvent__isset __isset;

  bool operator == (const ThriftFlumeEvent & rhs) const
  {
    if (!(timestamp == rhs.timestamp))
      return false;
    if (!(priority == rhs.priority))
      return false;
    if (!(body == rhs.body))
      return false;
    if (!(nanos == rhs.nanos))
      return false;
    if (!(host == rhs.host))
      return false;
    if (!(fields == rhs.fields))
      return false;
    return true;
  }
  bool operator != (const ThriftFlumeEvent &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const ThriftFlumeEvent & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};



#endif
