#pragma once
#include <Arduino.h>
#include <Ethernet.h>
#include <EEPROM.h>

class DiY_URtool
{
public:
    static const uint8_t BUF_SIZE = 32;

    DiY_URtool(const byte* mac, uint16_t port, uint8_t csPin);

    bool begin();
    void handler();

    // Buffer access
    uint8_t getInByte(uint8_t index) const;
    void setOutByte(uint8_t index, uint8_t value);

    void readInputBuffer(uint8_t *dst, uint8_t len) const;
    void writeOutputBuffer(const uint8_t *src, uint8_t len);

    bool clientConnected();
    bool commOk() const;

    // Watchdog
    void resetWatchdog();

    // Network config (EEPROM)
    void saveNetworkConfig();
    void loadNetworkConfig();

    // Comandes de xarxa
    void setIPAddress(IPAddress ip);
    void setGateway(IPAddress gw);
    void setSubnet(IPAddress sn);
    
    // NOU: Funció per gestionar les comandes rebudes pel port sèrie
    void handleSerialCommands();

    bool getWatchdogError(void);

    IPAddress getIPAddress() const;
    IPAddress getGateway() const;
    IPAddress getSubnet() const;

private:
    void processInput();
    void resetClientState();

private:
    byte _mac[6];
    uint16_t _port;
    uint8_t _csPin;

    EthernetServer _server;
    EthernetClient _client;

    IPAddress   _localIp;
    IPAddress   _gateway;
    IPAddress   _subnet;
    uint8_t       _model;
    uint8_t       _firmware_A;
    uint8_t       _firmware_B;

    uint8_t _inBuf[BUF_SIZE];
    uint8_t _outBuf[BUF_SIZE];

    unsigned long _lastCommunication;
    bool _watchdogError;

    static const uint32_t WATCHDOG_TIMEOUT_MS = 3000;   // 3s sense comunicació
    static const uint32_t READ_TIMEOUT_MS     = 100;    // Timeout lectura

    // EEPROM layout
    static const uint8_t EEPROM_SIGNATURE = 0xA5;
    static const uint16_t EEPROM_BASE     = 0;
};