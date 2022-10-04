#pragma once

#include <Arduino.h>
#include "tunnel/protocol.h"

class TunnelSerial
{
private:
    const uint32_t PACKET_STOP_TIMEOUT = 500;
    Stream* _debug_serial;
    Stream* _protocol_serial;
    
    char* _read_buffer;
    char* _write_buffer;
    uint32_t start_wait_time;
    TunnelProtocol* _protocol;
    PacketResult* _result;

public:
    TunnelSerial(Stream* debug_serial, Stream* protocol_serial);
    ~TunnelSerial();

    void begin();
    PacketResult* readPacket();
    void writePacket(const char *category, const char *formats, ...);
    void writeConfirmingPacket(const char *category, const char *formats, ...);
    void writeBuffer(int length);
};
