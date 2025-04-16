////////////////////////////////
//
// datacommlib.h
// date 2022-11-14
// version 4.1.0
//
////////////////////////////////


#ifndef DATACOMMLIB_H
  #define DATACOMMLIB_H

  #if (ARDUINO >=100)
    #include "Arduino.h"
  #else
    #include "WProgram.h"
  #endif

////////////////////////////
//
// Definitions and Globals
//
////////////////////////////

//
// Hardware
//
  // LEDs
  #define LED_B  10
  #define LED_R  12
  #define LED_G  11
  #define DEB_1  7
  #define DEB_2  8
  #define DEB_3  9
  #define PIN_RX  0
  #define PIN_TX  13
  #define PIN_BUTTON  2

  // Address
  const int PIN_ADDR[] = {3,4,5,6};

// 
// L1 Communication parameters
//
  // Tx/Rx
  #define T_S 100
  #define MAX_TX_ATTEMPTS 3
  
  // Flags
  #define LEN_PREAMBLE  8
  const byte PREAMBLE_SEQ = B10101010;
  #define LEN_SFD  8
  const byte SFD_SEQ =  B01111110;
  
  // A/D Converter
  #define AD_TH  900
  
  // Test Frame
  const unsigned long testframe = 0x0F210B00; // = B00001111001000010000101100000000

//
// L2 Frame Data Unit variabels
//

  #define LEN_FRAME  32   // the frame is 32 bits according to the protocol
  #define LEN_FRAME_PAYLOAD  8
  #define LEN_FRAME_TYPE  4
  #define LEN_FRAME_SEQNUM  4
  #define LEN_FRAME_ADDR  4
  #define LEN_FRAME_CRC  8
  
  // Frame types
  #define FRAME_TYPE_ACK  1
  #define FRAME_TYPE_DATA  2

  
//
// Application Message  
//
  #define LEN_MESSAGE  2
  #define MESSAGE_ADDRESS  0
  #define MESSAGE_PAYLOAD  1

//
// Runtime
//
  // States
  #define NONE            -1 // No state
  #define L1_RECEIVE      0 // Rx: Receive frame
  #define L1_SEND         1 // Tx: Transmit frame
  #define L2_FRAME_REC    10 // Process received payload on layer L2
  #define L2_DATA_SEND    11 // Process the L2 payload to be sent
  #define L2_ACK_SEND     12 // Process reception of an ACK frame
  #define L2_ACK_REC      13 // Process sending of an ACK
  #define L2_RETRANSMIT	  14 // Handle re-transmissions
  #define APP_PRODUCE     20 // Produce content/message to send 
  #define APP_ACT         21 // Act on received payload
  #define WAIT        	  -2 // Wait
  #define DEBUG       	  -3 // Print all system proporties
  #define HALT        	  -4 // Halt i.e. go into infinite loop
  
////////////////////////////////
//
// Shield
//
///////////////////////////////

class Shield {
  public:
    // constructor
    Shield();
    // Variables
    // Methods
    void begin();
    int select_led();
    int get_address();
    void allLedsOn();
    void allLedsOff();
	void allDebsOn();
    void allDebsOff();
	void debsShowNum(int value);
    int adConv(int value);
    void halt(int dly=500);
	void setAdThreshold(int value);
	int getAdThreshold();
	void setMyAddress(int value);
	int getMyAddress();
	int sampleRecCh(int value);
  private:
    int my_address = -1;
    int readButtonState();
	int adThreshold;
};

////////////////////////////////
//
// Frame
//
///////////////////////////////

class Frame {
  public:
	// constructor
	Frame ();
	// variables
	unsigned long frame;
	int frame_from          = -1;
	int frame_to            = -1;
	int frame_type          = -1;
	int frame_payload       = -1;
	int frame_seqnum        = -1;
	int frame_crc           = -1;
	// Methods
	void print_frame();	
};

////////////////////////////////
//
// Transmit
//
///////////////////////////////

class  Transmit : public Frame {
  public:
    // constructor
    Transmit();
    // variables
	int message[LEN_MESSAGE];
	int tx_attempts 		= 0;
    // Methods
    void frame_generation();
	void add_crc(int crc);
};

////////////////////////////////
//
// Receive
//
///////////////////////////////

class Receive : public Frame {
  public:
    // constructor
    Receive ();
    // Variables
	int message[LEN_MESSAGE];
    // Methods
    void frame_decompose();
};

#endif

