////////////////////////////
//
// V21.1 Skeleton.ino
//
//
// 2022-12-17 Jens Andersson
//
////////////////////////////

//
// Select library
#include <datacommlib.h>

//
// Prototypes
//
// predefined functions
void l1_send(unsigned long l2frame, int framelen);
boolean l1_receive(int timeout);

// your own
void send_frame(uint32_t frame);
void send_byte(byte data);
//
// Runtime
//

// Runtime variables
byte LED_PAYLOAD;

// State
int state = NONE;

//////////////////////////////////////////////////////////
//
// Add global constant and variable declarations here
//
Shield sh;  // note! no () since constructor takes no arguments
Transmit tx;
Receive rx;
Frame fr;

//////////////////////////////////////////////////////////

//
// Code
//
void setup() {
  Serial.begin(9600);
  sh.begin();

  //////////////////////////////////////////////////////////
  //
  // Add init code here
  //

  state = APP_PRODUCE;
  
  // Set your development node's address here

  //////////////////////////////////////////////////////////
}

void loop() {

  //////////////////////////////////////////////////////////
  //
  // State machine
  // Add code for the different states here
  //


  switch (state) {

    case L1_SEND:
      Serial.println("[State] L1_SEND");
      // +++ add code here and to the predefined function void l1_send(unsigned long l2frame, int framelen) below
      l1_send(tx.frame, LEN_FRAME);
      state = HALT;
	  
      // ---
      break;

    case L1_RECEIVE:
      Serial.println("[State] L1_RECEIVE");
      // +++ add code here and to the predefined function boolean l1_receive(int timeout) below
	  
      // ---
      break;

    case L2_DATA_SEND:
      Serial.println("[State] L2_DATA_SEND");
      // +++ add code here
      tx.frame_to = 0;
      tx.frame_from = 0;
      tx.frame_type = FRAME_TYPE_DATA;
      tx.frame_seqnum = 0;
      tx.frame_crc = 0;
      tx.frame_payload = LED_PAYLOAD;
      tx.frame_generation();
      
      state = L1_SEND;
      // ---
      break;

    case L2_RETRANSMIT:
      Serial.println("[State] L2_RETRANSMIT");
      // +++ add code here

      // ---
      break;

    case L2_FRAME_REC:
      Serial.println("[State] L2_FRAME_REC");
      // +++ add code here

      // ---
      break;

    case L2_ACK_SEND:
      Serial.println("[State] L2_ACK_SEND");
      // +++ add code here

      // ---
      break;

    case L2_ACK_REC:
      Serial.println("[State] L2_ACK_REC");
      // +++ add code here

      // ---
      break;

    case APP_PRODUCE:
      Serial.println("[State] APP_PRODUCE");
      // +++ add code here
      LED_PAYLOAD = sh.select_led();
      state = L2_DATA_SEND;

      // ---
      break;

    case APP_ACT:
      Serial.println("[State] APP_ACT");
      // +++ add code here
      //Serial.println(sh.select_led());
    
      // ---
      break;

    case HALT:
      Serial.println("[State] HALT");
      sh.halt();
      break;

    default:
      Serial.println("UNDEFINED STATE");
      break;
  }

  //////////////////////////////////////////////////////////
}

//////////////////////////////////////////////////////////
//
// Add code to the predefined functions
//

void l1_send(unsigned long frame, int framelen) {
  send_byte(PREAMBLE_SEQ);
  send_byte(SFD_SEQ);

  for (int i = framelen-1; i >= 0; i--) {
    int bitVal = (frame >> i) & 1;
    digitalWrite(PIN_TX, bitVal ? HIGH : LOW);
    delay(T_S);  // 100ms per bit
  }
}

boolean l1_receive(int timeout) {
	
	return true;
}

void send_byte(byte data) {
  for (int i = 7; i >= 0; i--) {
    // Extract bit i (most significant bit first)
    int bitVal = (data >> i) & 1;
    digitalWrite(PIN_TX, bitVal ? HIGH : LOW);
    delay(T_S);  // 100ms per bit
  }
}


//////////////////////////////////////////////////////////
//
// Add your functions here
//
