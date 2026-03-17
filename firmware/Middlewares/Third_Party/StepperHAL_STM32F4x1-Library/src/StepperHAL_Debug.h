#pragma once

//#define StepperHAL_DEBUG_ENABLE true  // Debug abilitato
#define StepperHAL_DEBUG_ENABLE false // Debug disabilitato

#if StepperHAL_DEBUG_ENABLE
  #define SerialDB Serial
#else
  class DummySerial {
  public:
    void begin(...) {}
    void print(...) {}
    void println(...) {}
    void printf(...) {}
  };
  extern DummySerial SerialDB;
#endif



