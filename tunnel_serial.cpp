
#include <tunnel_serial.h>

TunnelSerial::TunnelSerial(Stream* debug_serial, Stream* protocol_serial)
{
    _debug_serial = debug_serial;
    _protocol_serial = protocol_serial;

    start_wait_time = 0;

    _read_buffer = new char[TunnelProtocol::MAX_PACKET_LEN];
    _write_buffer = new char[TunnelProtocol::MAX_PACKET_LEN];

    _protocol = new TunnelProtocol(debug_serial);
    _result = new PacketResult(debug_serial, TunnelProtocol::NULL_ERROR, 0);

    for (int index = 0; index < TunnelProtocol::MAX_PACKET_LEN; index++) {
        _read_buffer[index] = '\0';
    }
}

PacketResult* TunnelSerial::readPacket()
{
    if (!_protocol_serial->available()) {
        return NULL;
    }

    char c = _protocol_serial->read();
    if (c == TunnelProtocol::PACKET_START_0) {
       start_wait_time = millis();
        while (!_protocol_serial->available()) {
            if (millis() - start_wait_time > PACKET_STOP_TIMEOUT) {
                if (_protocol->getSerial() != NULL) {
                    _protocol->getSerial()->println(F("Time out exceeded for start"));
                }
                return NULL;
            }
        }
        c = _protocol_serial->read();
        if (c != TunnelProtocol::PACKET_START_1) {
            return NULL;
        }
    }
    else {
        return NULL;
    }
    int _num_chars_read = 0;
    _read_buffer[_num_chars_read++] = TunnelProtocol::PACKET_START_0;
    _read_buffer[_num_chars_read++] = TunnelProtocol::PACKET_START_1;
    
    start_wait_time = millis();
    int packet_len = 0;
    while (true)
    {
        if (millis() - start_wait_time > PACKET_STOP_TIMEOUT) {
            if (_protocol->getSerial() != NULL) {
                _protocol->getSerial()->println(F("Time out exceeded"));
            }
            break;
        }
        if (!_protocol_serial->available()) {
            continue;
        }

        c = _protocol_serial->read();
        _read_buffer[_num_chars_read++] = c;
        if (_num_chars_read >= TunnelProtocol::MAX_PACKET_LEN) {
            if (_protocol->getSerial() != NULL) {
                _protocol->getSerial()->println(F("Max num chars exceeded"));
            }
            return NULL;
        }
        if (_num_chars_read == TunnelProtocol::CHECKSUM_START_INDEX) {
            packet_len = (int)to_uint16(_read_buffer + TunnelProtocol::LENGTH_START_INDEX);
        }
        else if (_num_chars_read > TunnelProtocol::CHECKSUM_START_INDEX) {
            if (packet_len >= TunnelProtocol::MAX_PACKET_LEN) {
                if (_protocol->getSerial() != NULL) {
                    _protocol->getSerial()->println(F("Max packet len exceeded"));
                }
                return NULL;
            }
            if (_num_chars_read - TunnelProtocol::CHECKSUM_START_INDEX > packet_len)
            {
                if (c != TunnelProtocol::PACKET_STOP) {
                    if (_protocol->getSerial() != NULL) {
                        _protocol->getSerial()->print(F("_num_chars_read: "));
                        _protocol->getSerial()->println(_num_chars_read);
                        _protocol->getSerial()->println(F("Last char not stop"));
                    }
                    return NULL;
                }
                break;
            }
        }
    }
    _read_buffer[_num_chars_read] = '\0';
    
    // _protocol->getSerial()->print(F("_num_chars_read: "));
    // _protocol->getSerial()->println(_num_chars_read);

    // _protocol->getSerial()->print(F("packet_len: "));
    // _protocol->getSerial()->println(packet_len);

    // for (int index = 0; index < TunnelProtocol::MAX_PACKET_LEN; index++) {
    //     _protocol->getSerial()->print(_read_buffer[index], HEX);
    //     _protocol->getSerial()->print(' ');
    // }
    // _protocol->getSerial()->print('\n');

    _result->setErrorCode(TunnelProtocol::NULL_ERROR);
    _protocol->parsePacket(_read_buffer, 0, _num_chars_read, _result);
    int code = _result->getErrorCode();
    if (code == TunnelProtocol::NULL_ERROR) {
        return NULL;
    }
    if (_protocol->isCodeError(code)) {
        if (_protocol->getSerial() != NULL) {
            _protocol->getSerial()->print(F("Encountered error code: "));
            _protocol->getSerial()->println(code);
        }
        return NULL;
    }
    if (_result->getPacketType() == PACKET_TYPE_HANDSHAKE) {
        writeConfirmingPacket(_result->getCategory().c_str(), "ud", _result->getPacketNum(), _result->getErrorCode());
    }
    return _result;
}

// TODO: add writeHandshakePacket

void TunnelSerial::writeConfirmingPacket(const char *category, const char *formats, ...)
{
    va_list args;
    va_start(args, formats);
    int length = _protocol->makePacket(PACKET_TYPE_CONFIRMING, _write_buffer, category, formats, args);
    writeBuffer(length);
    va_end(args);
}


void TunnelSerial::writePacket(const char *category, const char *formats, ...)
{
    va_list args;
    va_start(args, formats);
    int length = _protocol->makePacket(PACKET_TYPE_NORMAL, _write_buffer, category, formats, args);
    writeBuffer(length);
    va_end(args);
}

void TunnelSerial::writeBuffer(int length)
{
    if (0 < length && length < TunnelProtocol::MAX_PACKET_LEN) {
        _protocol_serial->write(_write_buffer, length);
    }
    else {
        if (_protocol->getSerial() != NULL) {
            _protocol->getSerial()->println(F("Skipping write for packet"));
        }
    }
}
