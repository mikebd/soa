/**
 * Autogenerated by Thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 */
#include "flume_types.h"



int _kPriorityValues[] = {
  Priority::FATAL,
  Priority::ERROR,
  Priority::WARN,
  Priority::INFO,
  Priority::DEBUG,
  Priority::TRACE
};
const char* _kPriorityNames[] = {
  "FATAL",
  "ERROR",
  "WARN",
  "INFO",
  "DEBUG",
  "TRACE"
};
const std::map<int, const char*> _Priority_VALUES_TO_NAMES(::apache::thrift::TEnumIterator(6, _kPriorityValues, _kPriorityNames), ::apache::thrift::TEnumIterator(-1, NULL, NULL));

int _kEventStatusValues[] = {
  EventStatus::ACK,
  EventStatus::COMMITED,
  EventStatus::ERR
};
const char* _kEventStatusNames[] = {
  "ACK",
  "COMMITED",
  "ERR"
};
const std::map<int, const char*> _EventStatus_VALUES_TO_NAMES(::apache::thrift::TEnumIterator(3, _kEventStatusValues, _kEventStatusNames), ::apache::thrift::TEnumIterator(-1, NULL, NULL));

const char* ThriftFlumeEvent::ascii_fingerprint = "BC13FFB3246A0179557F7F60C181A557";
const uint8_t ThriftFlumeEvent::binary_fingerprint[16] = {0xBC,0x13,0xFF,0xB3,0x24,0x6A,0x01,0x79,0x55,0x7F,0x7F,0x60,0xC1,0x81,0xA5,0x57};

uint32_t ThriftFlumeEvent::read(::apache::thrift::protocol::TProtocol* iprot) {

  uint32_t xfer = 0;
  std::string fname;
  ::apache::thrift::protocol::TType ftype;
  int16_t fid;

  xfer += iprot->readStructBegin(fname);

  using ::apache::thrift::protocol::TProtocolException;


  while (true)
  {
    xfer += iprot->readFieldBegin(fname, ftype, fid);
    if (ftype == ::apache::thrift::protocol::T_STOP) {
      break;
    }
    switch (fid)
    {
      case 1:
        if (ftype == ::apache::thrift::protocol::T_I64) {
          xfer += iprot->readI64(this->timestamp);
          this->__isset.timestamp = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 2:
        if (ftype == ::apache::thrift::protocol::T_I32) {
          int32_t ecast0;
          xfer += iprot->readI32(ecast0);
          this->priority = (Priority::type)ecast0;
          this->__isset.priority = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 3:
        if (ftype == ::apache::thrift::protocol::T_STRING) {
          xfer += iprot->readBinary(this->body);
          this->__isset.body = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 4:
        if (ftype == ::apache::thrift::protocol::T_I64) {
          xfer += iprot->readI64(this->nanos);
          this->__isset.nanos = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 5:
        if (ftype == ::apache::thrift::protocol::T_STRING) {
          xfer += iprot->readString(this->host);
          this->__isset.host = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 6:
        if (ftype == ::apache::thrift::protocol::T_MAP) {
          {
            this->fields.clear();
            uint32_t _size1;
            ::apache::thrift::protocol::TType _ktype2;
            ::apache::thrift::protocol::TType _vtype3;
            iprot->readMapBegin(_ktype2, _vtype3, _size1);
            uint32_t _i5;
            for (_i5 = 0; _i5 < _size1; ++_i5)
            {
              std::string _key6;
              xfer += iprot->readString(_key6);
              std::string& _val7 = this->fields[_key6];
              xfer += iprot->readBinary(_val7);
            }
            iprot->readMapEnd();
          }
          this->__isset.fields = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      default:
        xfer += iprot->skip(ftype);
        break;
    }
    xfer += iprot->readFieldEnd();
  }

  xfer += iprot->readStructEnd();

  return xfer;
}

uint32_t ThriftFlumeEvent::write(::apache::thrift::protocol::TProtocol* oprot) const {
  uint32_t xfer = 0;
  xfer += oprot->writeStructBegin("ThriftFlumeEvent");
  xfer += oprot->writeFieldBegin("timestamp", ::apache::thrift::protocol::T_I64, 1);
  xfer += oprot->writeI64(this->timestamp);
  xfer += oprot->writeFieldEnd();
  xfer += oprot->writeFieldBegin("priority", ::apache::thrift::protocol::T_I32, 2);
  xfer += oprot->writeI32((int32_t)this->priority);
  xfer += oprot->writeFieldEnd();
  xfer += oprot->writeFieldBegin("body", ::apache::thrift::protocol::T_STRING, 3);
  xfer += oprot->writeBinary(this->body);
  xfer += oprot->writeFieldEnd();
  xfer += oprot->writeFieldBegin("nanos", ::apache::thrift::protocol::T_I64, 4);
  xfer += oprot->writeI64(this->nanos);
  xfer += oprot->writeFieldEnd();
  xfer += oprot->writeFieldBegin("host", ::apache::thrift::protocol::T_STRING, 5);
  xfer += oprot->writeString(this->host);
  xfer += oprot->writeFieldEnd();
  xfer += oprot->writeFieldBegin("fields", ::apache::thrift::protocol::T_MAP, 6);
  {
    xfer += oprot->writeMapBegin(::apache::thrift::protocol::T_STRING, ::apache::thrift::protocol::T_STRING, this->fields.size());
    std::map<std::string, std::string> ::const_iterator _iter8;
    for (_iter8 = this->fields.begin(); _iter8 != this->fields.end(); ++_iter8)
    {
      xfer += oprot->writeString(_iter8->first);
      xfer += oprot->writeBinary(_iter8->second);
    }
    xfer += oprot->writeMapEnd();
  }
  xfer += oprot->writeFieldEnd();
  xfer += oprot->writeFieldStop();
  xfer += oprot->writeStructEnd();
  return xfer;
}


