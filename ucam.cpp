

#include "UCAM.h"
TCPClient client;
byte imageSvc[] = { 192, 168, 1, 84 }; // MacPro
IntervalTimer camTimer;
UCAM* myCam;
void camRx(void);
int diagTrigger = D2;  // Scope trigger
int BluLed = D7; // This one is the built-in tiny one to the right of the USB jack

UCAM::UCAM()
{ 
    myCam = this;
}


void UCAM::begin()
{
	// Set up uart on core tx/rx pins
    Serial1.begin(57600);   // Initial baud rate
    pinMode(diagTrigger, OUTPUT);
    digitalWrite(diagTrigger, 0);   // tbd

    rxCount = 0;
    rxCount = 0;
    pkgId = 0;
    numPkg = 0;

    rBuff.state = EMPTY;
}

void camRx(void)
{
    myCam->rxIsr();
}

// Must run often enought to prevent 64-byte rx buffer from overflowing
void UCAM::rxIsr(void) 
{
    // listen for ucam
    while ((Serial1.available() > 0) && (rBuff.state == READY))
    {
        rBuff.data[rxCount++] = Serial1.read();
        
        if (rxCount >= PACKAGE_SIZE)
        {
            // Mark this buffer full
            rBuff.state = FULL;
        }
    }
}

// Handles image dump to server
void UCAM::camLoop(void)
{
    // Send to server
    if (rBuff.state == FULL)
    {
        // tbd test...
        if (!client.connected())
        {
            client.connect(imageSvc, 9000);
        }
        if (client.connected())
        {
            // Verify tbd
            
            // tx jpeg file, strip header
            client.write(rBuff.data+4, PACKAGE_SIZE-6);
            rBuff.state = READY;
            if (++pkgId >= numPkg)
            {
                digitalWrite(diagTrigger, 1);   // tbd

                // Done image rx
                camTimer.end();
                // ack last packet
                sendCommand(ACK, 0, 0, 0xf0, 0xf0);
                digitalWrite(diagTrigger, 0);   // tbd

            }
            else
            {
                // Get next pkg
                rxCount = 0;
                sendCommand(ACK, 0, 0, pkgId &0xff, pkgId>>8);
            }
        }
    }
}


void UCAM::sendCommand(uint8_t cmdID, uint8_t parm1, uint8_t parm2, uint8_t parm3, uint8_t parm4)
{
    uint8_t command[6] = { HDR, cmdID, parm1, parm2, parm3, parm4};
    Serial1.write(command, 6);
    Serial1.flush(); // isr safe?
}

uint16_t UCAM::waitAck(uint32_t maxTime)
{
    uint8_t rxData[6];
    uint32_t timeout = millis() + maxTime;

    // Wait for 6-byte response
    while (Serial1.available() < 6)
        if (millis() > timeout)
        {
            return 0x501;   
        }

    for (int rxCount = 0; rxCount < 6; rxCount++)
    {
        rxData[rxCount] = Serial1.read();
    }

    if (rxData[1] != ACK) return (0x700 | rxData[1]);
    return rxData[1];
}

// Reads DATA message, sets up transfer
uint16_t UCAM::waitData(uint32_t maxTime)
{
    uint8_t rxData[6];
    uint32_t timeout = millis() + maxTime;

    // Wait for 6-byte response
    while (Serial1.available() < 6)
        if (millis() > timeout)
        {
            return 0x8001;   
        }

    for (int rxCount = 0; rxCount < 6; rxCount++)
    {
        rxData[rxCount] = Serial1.read();
    }

    if (rxData[1] != DATA) return (0x8100 | rxData[1]);
    
    // Init rx 
    imgSize = rxData[5];
    imgSize = imgSize<<8 | rxData[4];
    imgSize = imgSize<<8 | rxData[3];
    imgType = rxData[2];
    numPkg = imgSize/(PACKAGE_SIZE-6);
//    if ((imgSize % (PACKAGE_SIZE-6)) != 0) numPkg++;
    return rxData[1];
}


uint16_t UCAM::setInitial(uint8_t imgFormat, uint8_t rawRez, uint8_t jpegRez)
{
    sendCommand(INITIAL, 0, imgFormat, rawRez, jpegRez);
    // Wait for response ~ 50 msec
    return waitAck(100);
}

uint16_t UCAM::chkSync(void)
{
    sendCommand(SYNC, 0, 0, 0, 0);
    // Wait for response
    return waitAck(20);
}

uint16_t UCAM::setPackageSize(uint16_t pkgSize)
{
    sendCommand(SET_PKG_SIZE, 0x08, pkgSize & 0xff, pkgSize>>8, 0);
    // Wait for response
    return waitAck(20);
}

uint16_t UCAM::setLightFreq(uint8_t freqType)
{
    sendCommand(LIGHT, freqType, 0, 0,0);
    // Wait for response
    return waitAck(20);
}

uint16_t UCAM::snapShot(uint8_t snapType, uint16_t frameDelay)
{
    sendCommand(SNAPSHOT, snapType, frameDelay&0xff, frameDelay>>8, 0);
    // Wait for response
    return waitAck(400);
}

// Start data capture isr
uint16_t UCAM::getPicture(uint8_t pictType)
{
    sendCommand(GET_PICTURE, pictType, 0, 0, 0);
    // Wait for ack ~110 ms
    uint16_t stat = waitAck(200);
    if (stat == ACK)
    {
        // Wait for DATA packet ~70 ms
        stat = waitData(200);
        if (stat == DATA)
        {
            // Set up rx
            rxCount = 0;
            rBuff.state = READY;

            // Start image dl state machine
            camTimer.begin(camRx, 10, hmSec); // 5 msec
            // Send the ACK to prime the dl, pkgId 0
            pkgId = 0;
            sendCommand(ACK, 0, 0, 0, 0);
        }
    }
    return stat;
}

uint16_t UCAM::reset(void)
{
    sendCommand(RESET, 1, 0, 0, 0);
    // Wait for response
    return waitAck(20);
}

// Returns SYNC on success
uint16_t UCAM::setSync(void)
{
    uint8_t rxData[6];
    uint32_t timeout = millis() + 60;

    // Empty incoming
    while (Serial1.available() > 0) Serial1.read();
    
    sendCommand(SYNC, 0, 0, 0,0);
    
    // Wait for 6-byte response
    while (Serial1.available() < 6)
        if (millis() > timeout)
        {
            return 0x201;   
        }

    for (int rxCount = 0; rxCount < 6; rxCount++)
    {
        rxData[rxCount] = Serial1.read();
    }
    
    if (rxData[1] != ACK) return (0x300 | rxData[1]);
    
    // Wait for sync
    while (Serial1.available() != 6)
        if (millis() > timeout)
        {
            return 0x202;   
        }

    // Get sync
    for (int rxCount = 0; rxCount < 6; rxCount++)
    {
        rxData[rxCount] = Serial1.read();
    }

    // Ack it
    if (rxData[1] != SYNC) return (0x500 | rxData[1]);
    else sendCommand(ACK, SYNC, 0, 0, 0);
    return SYNC;

}


