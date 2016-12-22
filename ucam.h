
#ifndef UCAM_h
#define UCAM_h

#include "spark_wiring.h"
#include "spark_wiring_usartserial.h"
#include "spark_wiring_tcpclient.h"
#include "SparkIntervalTimer/SparkIntervalTimer.h"

// Buffer states
enum {  EMPTY,
        READY,
        FULL
};

#define PACKAGE_SIZE 64
#define MAX_FILE_SIZE 

// UCAM command ID's
#define INITIAL 0x01
#define GET_PICTURE 0x04
#define SNAPSHOT 0x05
#define SET_PKG_SIZE 0x06
#define SET_BAUD 0x07
#define RESET 0x08
#define DATA 0x0a
#define SYNC 0x0d
#define ACK 0x0e
#define NAK 0x0f
#define LIGHT 0x13
#define HDR 0xaa

// Image formats
#define GRAY8 0x03
#define CrYCbY 0x08
#define RGB565 0x06
#define JPEG 0x07

// Raw Resolution
#define RAW80x60 0x01
#define RAW160x120 0x03
#define RAW128x128 0x09
#define RAW128x96 0x08

// JPEG Resolution
#define JPG160x128 0x03
#define JPG320x240 0x05
#define JPG640x480 0x07

// Picture Source
#define SNAP_PICT 0x01
#define RAW_PICT 0x02
#define JPEG_PICT 0x05

// Snapshot types
#define JPEG_SNAP 0x0
#define RAW_SNAP 0x1

// Light Frequency Type
#define HZ_50 0x0
#define HZ_60 0x1

class UCAM
{
	public:
		UCAM();
        void begin(void);
        int getState();
	    void camLoop(void);
        uint16_t setInitial(uint8_t imgFormat, uint8_t rawRez, uint8_t jpegRez);
        uint16_t setPackageSize(uint16_t pkgSize);
        uint16_t setLightFreq(uint8_t freqType);
        uint16_t snapShot(uint8_t snapType, uint16_t frameDelay);
        uint16_t getPicture(uint8_t pictType);
        uint16_t reset(void);
        void rxIsr(void);
        uint16_t setSync(void);
        uint16_t chkSync(void);

	protected:
        struct {
            uint8_t state;
            uint8_t data[PACKAGE_SIZE];
        } rBuff;
        
        uint16_t rxCount; // Index to buffer data
        uint16_t pkgId; // Incoming packaage ID
        uint16_t numPkg;

        uint8_t imgType;
        uint32_t imgSize;

        void sendCommand(uint8_t cmdID, uint8_t parm1, uint8_t parm2, uint8_t parm3, uint8_t parm4);
        uint16_t waitAck(uint32_t maxTime);
        uint16_t waitData(uint32_t maxTime);
};

#endif