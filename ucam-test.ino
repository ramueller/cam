
// This #include statement was automatically added by the Spark IDE.
#include "UCAM.h"

// Define the pins we're going to call pinMode on
int RedLed = D0;  // You'll need to wire an LED to this one to see it blink.
int motionFlag = D1;
int buttonTrigger = D3;
const char * DEVID1 = "v0774500DFB4A479";         // PushingBox.com Scenario: "Motion Detected"

// Debug mode
boolean DEBUG = true;

const char * serverName = "api.pushingbox.com";   // PushingBox API URL
IPAddress serverIP(213,186,33,19);
IPAddress routerIPAddress(192,168,1,254);
IPAddress macproIP(192,168,1,84);
IPAddress goodIPAddress(8,8,4,4);

boolean motionState = false;             
boolean lastMotionState = false;             // Save the last state of the Pin for DEVID1

unsigned int nextTime = 0;    // Next time to contact the server
//HttpClient http;

//uint32_t lastDbgVar;  // tbd

// Headers currently need to be set at init, useful for API keys etc.
//http_header_t headers[] = {
    //  { "Content-Type", "application/json" },
    //  { "Accept" , "application/json" },
//    { "Accept" , "*/*"},
//    { NULL, NULL } // NOTE: Always terminate headers will NULL
//};

//http_request_t request;
//http_response_t response;
TCPClient PBclient;

UCAM uCam;
uint32_t currStat;   // test var tbd

SYSTEM_MODE(MANUAL); 

// This routine runs only once upon reset
void setup() {
    // Initialize D0 + D7 pin as output
    // It's important you do this here, inside the setup() function rather than outside it or in the loop function.
  pinMode(RedLed, OUTPUT);
  pinMode(motionFlag, INPUT);
  pinMode(buttonTrigger, INPUT_PULLUP);

  currStat = 0;

  Serial.begin(9600);                     // Start the USB Serial
  
  delay(1000);
  Serial.println("Alive...the sequel");
  delay(1000);
  
  WiFi.on();
  
//  if (WiFi.clearCredentials()){
//      Serial.println("Credentials cleared");
//  }
//      delay(1000);
  
  // this always returns true 
  if (!WiFi.hasCredentials){
      Serial.println("WiFi setting credentials...");
      WiFi.setCredentials("ProGuyWiFi", "hammerhead");
      delay(1000);
  }
  
  WiFi.connect();
  
  while (WiFi.connecting()){
      Serial.println("WiFi connecting...");
      delay(1000);
  }
  
  
  if (WiFi.ready()){
      Serial.println("WiFi ready");
  }
  else{
      Serial.println("WiFi NOT ready");
  }

  delay(200);

  // Prints out the network parameters over Serial
  if(DEBUG){
      Serial.print("SSID: ");
      Serial.println(WiFi.SSID());
      Serial.print("Core IP: ");
      Serial.println(WiFi.localIP());    
      Serial.print("Gateway: ");
      Serial.println(WiFi.gatewayIP());
      Serial.print("Mask: ");
      Serial.println(WiFi.subnetMask());
      Serial.print("WiFi RSSI: ");
      Serial.println(WiFi.RSSI());
      
      // Print TI firmware verstion
      uint8_t fwver[2];
      nvmem_read_sp_version(fwver);
      Serial.print(fwver[0]);
      Serial.println(fwver[1]);
    }
    
//    digitalWrite(TX, HIGH);
//    delay(20);
    uCam.begin();
//    delay(20);
//    uCam.reset();  // no effect
    delay(2);

    // Sync ucam
    uint16_t resp;
    int syncAttempts = 0;
    do 
    {    
        resp = uCam.setSync();
        delay(2);
    } while ((resp != SYNC) && (syncAttempts++ < 100));
    Serial.print("Resp = ");
    Serial.println(resp, HEX);
    if (resp != SYNC)
    {
        Serial.println("UCAM Sync Failure");
    }
    else
    {
        uint16_t stat;
        Serial.println("Synced");
        stat = uCam.setInitial(JPEG, RAW160x120, JPG640x480);
        if (stat == ACK) Serial.println("Initial set");
        else 
        {
            Serial.print("Initial failed: ");
            Serial.println(stat, HEX);
        }

        stat = uCam.setPackageSize(PACKAGE_SIZE);
        if (stat == ACK) Serial.println("Pkg set");
        else
        {
            Serial.print("Pkg failed: ");
            Serial.println(stat, HEX);
        }

        stat = uCam.setLightFreq(HZ_60);
        if (stat == ACK) Serial.println("Light set");
        else 
        {
            Serial.print("Light failed: ");
            Serial.println(stat, HEX);
        }
    }
}


// This routine gets called repeatedly, like once every 5-15 milliseconds.
// Spark firmware interleaves background CPU activity associated with WiFi + Cloud activity with your code. 
// Make sure none of your code delays or blocks for too long (like more than 5 seconds), or weird things can happen.
void loop() {
//      Serial.print("router PING: ");
//      Serial.println(WiFi.ping(routerIPAddress));

//      Serial.print("good PING: ");
//      Serial.println(WiFi.ping(goodIPAddress));

//      Serial.print("MacPro PING: ");
//      Serial.println(WiFi.ping(macproIP));


    motionState = !digitalRead(motionFlag);
    digitalWrite(RedLed, motionState);

    if (((motionState == HIGH) && (lastMotionState == LOW)) ||
        (digitalRead(buttonTrigger) == LOW))  // button depressed
    {
        delay(100);
        if(DEBUG){Serial.println("pinDevid1 is HIGH");}
        // Sending request to PushingBox when the pin is HIGH
        sendToPushingBox(DEVID1);
        // Take snap
        uint16_t stat = uCam.snapShot(JPEG_SNAP, 0);
        if (stat == ACK) Serial.println("Snap OK");
        else 
        {
            Serial.print("Snap failed: ");
            Serial.println(stat, HEX);
        }
        
        // Get snap data, start data capture
        stat = uCam.getPicture(JPEG_PICT);
        if (stat == DATA)
        {
            Serial.println("getting pict");
        }
        else
        {
            Serial.print("GetPict fail: ");
            Serial.println(stat, HEX);
        }
    }
    lastMotionState = motionState;
    
    // Data capture 
    uCam.camLoop();
/*
    if (dbgVar != lastDbgVar)
    {
        lastDbgVar = dbgVar;
        Serial.print("dbg= ");
        Serial.println(dbgVar, HEX);
    }
*/
}

void sendToPushingBox(const char * devid)
{
    if (nextTime > millis()) {
        return;
    }

    Serial.print("Notify... ");
    if(DEBUG){Serial.print("connecting... ");}
    PBclient.stop();

    if (PBclient.connect(serverName, 80)) {
        if(DEBUG){Serial.println("connected");}
        PBclient.print("GET /pushingbox?devid=");
        PBclient.print(devid);
        PBclient.println(" HTTP/1.1");
        PBclient.print("Host: ");
        PBclient.println(serverName);
    //    PBclient.println("User-Agent: Spark");
        PBclient.println("Connection: close");
        PBclient.println();
        if(DEBUG){
            Serial.print("sent!");
            Serial.println("");
        }


    } 
    else{
        if(DEBUG){Serial.println("connection failed");}

    }
    nextTime = millis() + 120000; // 2 minutes
}