#include "DiY_URtool.h"
#include <SPI.h>

// Helper function to parse IP from string
IPAddress parseIP(String ipStr) {
    IPAddress ip;
    if (ip.fromString(ipStr)) {
        return ip;
    }
    return IPAddress(0,0,0,0); // Return invalid IP on failure
}

// --------------------------------------------------
// Constructor
// --------------------------------------------------
DiY_URtool::DiY_URtool(const byte* mac, uint16_t port, uint8_t csPin)
: _port(port),
  _csPin(csPin),
  _server(port),
  _lastCommunication(0),
  _watchdogError(false)
{
    memcpy(_mac, mac, 6);

    // Valors per defecte si EEPROM no existeix
    _model = 1;          // Gripper Model
    _firmware_A = 1;     // Firmware version
    _firmware_B = 0;     // Firmware sub version

    _localIp = IPAddress(192,168,1,50);
    _gateway = IPAddress(192,168,1,1);
    _subnet  = IPAddress(255,255,255,0);
    

    memset(_inBuf, 0, BUF_SIZE);
    memset(_outBuf, 0, BUF_SIZE);
}


// --------------------------------------------------
// Inici
// --------------------------------------------------
bool DiY_URtool::begin()
{
    EEPROM.begin(64);

    loadNetworkConfig();
    setOutByte(29, _model);
    setOutByte(30, _firmware_A);
    setOutByte(31, _firmware_B);

    SPI.begin(18,19,23,_csPin);
    Ethernet.init(_csPin);

    // La crida original tenia el gateway repetit, s'ha corregit per utilitzar el DNS per defecte (0.0.0.0) o l'omissió.
    // Usarem Ethernet.begin(_mac, _localIp, _gateway, _subnet);
    // Nota: L'ESP32 Ethernet/Ethernet2 pot no donar suport al cinquè argument DNS.
    Ethernet.begin(_mac, _localIp, _gateway, _subnet);

    _server.begin();
    _lastCommunication = millis();
    _watchdogError = false;

    Serial.print("DiY_URtool started. IP: ");
    Serial.println(Ethernet.localIP());
    Serial.println("Ready. Use 'STATUS' or 'IP <addr>' via Serial monitor.");

    return true;
}


// --------------------------------------------------
// Tick – a cridar a loop() (MODIFICAT PER ROBUSTESA WATCHDOG)
// --------------------------------------------------
void DiY_URtool::handler()
{
    // Acceptar client si no tenim
    if (!clientConnected()) {
        EthernetClient newClient = _server.available();

        if (newClient) {
            if (_client && _client.connected())
                _client.stop();

            _client = newClient;

            //Serial.print("Client connected: ");
            //Serial.println(_client.remoteIP());
            
            // Netejar l'estat del Watchdog quan hi ha una nova connexió (NOU)
            resetWatchdog(); 
        }
        return;
    }

    // Llegir 32 bytes
    uint8_t idx = 0;
    unsigned long start = millis();

    while (idx < BUF_SIZE && (millis() - start) < READ_TIMEOUT_MS)
    {
        if (_client.available()) {
            int b = _client.read();
            if (b >= 0)
                _inBuf[idx++] = (uint8_t)b;
        } else {
            delay(1);
        }
    }

    if (idx == BUF_SIZE)
    {
        // Comunicació OK: netegem l'error
        _lastCommunication = millis();
        _watchdogError = false;

        processInput();

        size_t sent = _client.write(_outBuf, BUF_SIZE);
        _client.flush();

        if (sent != BUF_SIZE) {
            Serial.print("Warning: sent ");
            Serial.print(sent);
            Serial.println(" bytes");
        }
    }

    // Watchdog (MODIFICAT)
    if ((millis() - _lastCommunication) > WATCHDOG_TIMEOUT_MS)
    {
        if (!_watchdogError)
        {
            //Serial.println("WATCHDOG ERROR: No communication. Resetting buffers.");
            _watchdogError = true; // Activar l'error (Estava comentat a la versió proporcionada)
            
            // Buidar buffers d'entrada i sortida quan salta el Watchdog (NOU)
            memset(_inBuf, 0, BUF_SIZE);
            memset(_outBuf, 0, BUF_SIZE);
        }
    }

    // Si el client marxa
    if (!_client.connected())
    {
        //Serial.println("Client disconnected");
        resetClientState();
    }
}


// --------------------------------------------------
// Handle Serial Commands (NOU)
// --------------------------------------------------
void DiY_URtool::handleSerialCommands()
{
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        input.toUpperCase();

        if (input.length() == 0) return;

        Serial.print("> ");
        Serial.println(input);
        
        int spaceIndex = input.indexOf(' ');
        String command = (spaceIndex == -1) ? input : input.substring(0, spaceIndex);
        String argument = (spaceIndex == -1) ? "" : input.substring(spaceIndex + 1);
        argument.trim();

        if (command == "STATUS") {
            Serial.println("--- Configuració Ethernet Actual ---");
            Serial.print("IP: "); Serial.println(_localIp);
            Serial.print("MASK: "); Serial.println(_subnet);
            Serial.print("GW: "); Serial.println(_gateway);
            Serial.println("------------------------------------");
        }
        else if (command == "IP") {
            IPAddress newIp = parseIP(argument);
            if (newIp != IPAddress(0,0,0,0)) {
                setIPAddress(newIp);
                Serial.print("Nova IP temporal: "); Serial.println(_localIp);
                Serial.println("Utilitza 'SAVE' per guardar. REINICIA per aplicar.");
            } else {
                Serial.println("Error: Format d'IP no vàlid. Format: IP 192.168.1.10");
            }
        }
        else if (command == "MASK") {
            IPAddress newMask = parseIP(argument);
            if (newMask != IPAddress(0,0,0,0)) {
                setSubnet(newMask);
                Serial.print("Nova MASK temporal: "); Serial.println(_subnet);
                Serial.println("Utilitza 'SAVE' per guardar. REINICIA per aplicar.");
            } else {
                Serial.println("Error: Format de MASK no vàlid. Format: MASK 255.255.255.0");
            }
        }
        else if (command == "GW") {
            IPAddress newGw = parseIP(argument);
            if (newGw != IPAddress(0,0,0,0)) {
                setGateway(newGw);
                Serial.print("Nou GW temporal: "); Serial.println(_gateway);
                Serial.println("Utilitza 'SAVE' per guardar. REINICIA per aplicar.");
            } else {
                Serial.println("Error: Format de GATEWAY no vàlid. Format: GW 192.168.1.1");
            }
        }
        else if (command == "SAVE") {
            saveNetworkConfig();
            Serial.println("Configuració de xarxa guardada a l'EEPROM.");
            Serial.println("REINICIA l'ESP32 per aplicar els canvis a Ethernet.");
        }
        else {
            Serial.println("Comanda desconeguda. Comandes vàlides: STATUS, IP <addr>, MASK <addr>, GW <addr>, SAVE.");
        }
    }
}


// --------------------------------------------------
// getWatchdogError 
// --------------------------------------------------
bool DiY_URtool::getWatchdogError()
{
    return _watchdogError;
}


// --------------------------------------------------
// Process Input (substituïble per la teva lògica)
// --------------------------------------------------
void DiY_URtool::processInput()
{
    //memcpy(_outBuf, _inBuf, BUF_SIZE);
    ;
}


// --------------------------------------------------
// Buffer Access
// --------------------------------------------------
uint8_t DiY_URtool::getInByte(uint8_t index) const
{
    return (index < BUF_SIZE) ? _inBuf[index] : 0;
}

void DiY_URtool::setOutByte(uint8_t index, uint8_t value)
{
    if (index < BUF_SIZE)
        _outBuf[index] = value;
}

void DiY_URtool::readInputBuffer(uint8_t *dst, uint8_t len) const
{
    memcpy(dst, _inBuf, (len < BUF_SIZE ? len : BUF_SIZE));
}

void DiY_URtool::writeOutputBuffer(const uint8_t *src, uint8_t len)
{
    memcpy(_outBuf, src, (len < BUF_SIZE ? len : BUF_SIZE));
}


// --------------------------------------------------
// Communication status
// --------------------------------------------------
bool DiY_URtool::clientConnected()
{
    return (_client && _client.connected());
}

bool DiY_URtool::commOk() const
{
    return !_watchdogError;
}

void DiY_URtool::resetWatchdog()
{
    _watchdogError = false;
    _lastCommunication = millis();
}


// --------------------------------------------------
// Reset client
// --------------------------------------------------
void DiY_URtool::resetClientState()
{
    if (_client)
        _client.stop();

    //memset(_inBuf, 0, BUF_SIZE);
    //memset(_outBuf, 0, BUF_SIZE);

    _lastCommunication = millis();
    _watchdogError = false;
}


// --------------------------------------------------
// EEPROM Save / Load (robust, byte a byte)
// --------------------------------------------------
void DiY_URtool::saveNetworkConfig()
{
    EEPROM.write(EEPROM_BASE + 0, _localIp[0]);
    EEPROM.write(EEPROM_BASE + 1, _localIp[1]);
    EEPROM.write(EEPROM_BASE + 2, _localIp[2]);
    EEPROM.write(EEPROM_BASE + 3, _localIp[3]);

    EEPROM.write(EEPROM_BASE + 4, _subnet[0]);
    EEPROM.write(EEPROM_BASE + 5, _subnet[1]);
    EEPROM.write(EEPROM_BASE + 6, _subnet[2]);
    EEPROM.write(EEPROM_BASE + 7, _subnet[3]);

    EEPROM.write(EEPROM_BASE + 8,  _gateway[0]);
    EEPROM.write(EEPROM_BASE + 9,  _gateway[1]);
    EEPROM.write(EEPROM_BASE + 10, _gateway[2]);
    EEPROM.write(EEPROM_BASE + 11, _gateway[3]);

    EEPROM.write(EEPROM_BASE + 12, _model);
    EEPROM.write(EEPROM_BASE + 13, _firmware_A);
    EEPROM.write(EEPROM_BASE + 14, _firmware_B);

    EEPROM.write(EEPROM_BASE + 15, EEPROM_SIGNATURE);

    EEPROM.commit();
}

void DiY_URtool::loadNetworkConfig()
{
    uint8_t sig = EEPROM.read(EEPROM_BASE + 15);

    if (sig != EEPROM_SIGNATURE)
    {
        Serial.println("EEPROM empty: saving defaults");
        saveNetworkConfig();
        return;
    }

    _localIp = IPAddress(
        EEPROM.read(EEPROM_BASE + 0),
        EEPROM.read(EEPROM_BASE + 1),
        EEPROM.read(EEPROM_BASE + 2),
        EEPROM.read(EEPROM_BASE + 3)
    );

    _subnet = IPAddress(
        EEPROM.read(EEPROM_BASE + 4),
        EEPROM.read(EEPROM_BASE + 5),
        EEPROM.read(EEPROM_BASE + 6),
        EEPROM.read(EEPROM_BASE + 7)
    );

    _gateway = IPAddress(
        EEPROM.read(EEPROM_BASE + 8),
        EEPROM.read(EEPROM_BASE + 9),
        EEPROM.read(EEPROM_BASE + 10),
        EEPROM.read(EEPROM_BASE + 11)
    );

    _model = EEPROM.read(EEPROM_BASE + 12);
    _firmware_A = EEPROM.read(EEPROM_BASE + 13);
    _firmware_B = EEPROM.read(EEPROM_BASE + 14);

    Serial.println("Network config loaded from EEPROM:");
    Serial.print(" IP: ");   Serial.println(_localIp);
    Serial.print(" MASK: "); Serial.println(_subnet);
    Serial.print(" GW: ");   Serial.println(_model);
    Serial.print(" DiY_URtool Model: ");   Serial.println(_gateway);
    Serial.print(" Firmware version: ");   Serial.print(_firmware_A); Serial.print("."); Serial.println(_firmware_B);
}


// --------------------------------------------------
// Setters & Getters
// --------------------------------------------------
void DiY_URtool::setIPAddress(IPAddress ip)  { _localIp = ip; }
void DiY_URtool::setGateway(IPAddress gw)    { _gateway = gw; }
void DiY_URtool::setSubnet(IPAddress sn)     { _subnet = sn; }

IPAddress DiY_URtool::getIPAddress() const   { return _localIp; }
IPAddress DiY_URtool::getGateway() const     { return _gateway; }
IPAddress DiY_URtool::getSubnet() const      { return _subnet; }