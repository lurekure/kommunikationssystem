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
void send_byte(byte data);
bool detect_byte(byte data, int timeout);
void detect_frame(long data);
long read_next_bits(long data, int length);

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
long recFrame;

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

int my_address = PIN_ADDR[3]; //Hard coded address
setMyAddress(my_address);

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
      state = L1_RECEIVE;
	  
      // ---
      break;

    //pulses are first recieved at layer 1 in bits. We use the help function l1_recieve to 
    case L1_RECEIVE:
      Serial.println("[State] L1_RECEIVE");
      // +++ add code here and to the predefined function boolean l1_receive(int timeout) below
      //If shell recieved a premable + SFD + Frame within 5000ms, go to 
      if (l1_receive(5000) == true){
        Serial.println("frame recieved");
        digitalWrite(DEB_2,HIGH); //Turns on debug led 2

      }
      else {
        Serial.println("timeout reached");
        digitalWrite(DEB_1, HIGH); //Turns on debug led 1
      }
      state = L2_FRAME_REC;

      // ---
      break;

    case L2_DATA_SEND:
      Serial.println("[State] L2_DATA_SEND");
      // +++ add code here
      tx.frame_to = get_address();
      tx.frame_from = getMyAddress();
      tx.frame_type = FRAME_TYPE_DATA;
      tx.frame_seqnum = 0;
      tx.frame_crc = 0;
      //tx.frame_payload = 12;
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
      rx.frame = recFrame; //Puts the recieved frame recFrame into rx.frame object
      rx.frame_decompose(); //Decomposes the frame (32bit) into it's parts
        if (rx.frame_to != getMyAddress()) {
          Serial.print("Frame not for me. My address: ");
          Serial.print(getMyAddress());
          Serial.print(", frame to: ");
          Serial.println(rx.frame_to);
    
        // Silently drop frame by going back to listening
        state = L1_RECEIVE;
        break;
        }

        //DON'T KNOW IF DEVELOPER NODE SHOULD ACT ON INCOMING DATA YET???
        /*if (rx.frame_type == 2){
          byte ledindex = rx.frame_payload;
          sh.
        }
          */

        state = APP_PRODUCE; //Start sending again (Starting the application again), because the frame recognision went through
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
      Serial.println(sh.select_led());
    
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

//Stores the detected frame into recFrame and returns true if it succeded doing so
boolean l1_receive(int timeout) {
  recFrame = 0;
  long time = millis(); //start time
  int foundBit;
  bool preambleOK = detect_byte(PREAMBLE_SEQ, timeout); //True if expected premable is the premable
  if (!preambleOK) return false;

  if (preambleOK){  
    Serial.println("Preamble found");
    bool SFDOK = detect_byte(SFD_SEQ, timeout); //True if expected SFD is the SFD
    if (!SFDOK) return false;

    if (SFDOK){
      Serial.println("SFD found");
      // decoding frame
      for (int i = 0; i <= 31; i++) {
        foundBit = sh.sampleRecCh(PIN_RX);
        recFrame = (recFrame << 1) | foundBit;
        delay(T_S);  // 100ms per bit
      }
      Serial.println(recFrame);
    }
  }
	return true;
}

//////////////////////////////////////////////////////////
//
// Add your functions here
//

void send_byte(byte data) {
  for (int i = 7; i >= 0; i--) {
    // Extract bit i (most significant bit first)
    int bitVal = (data >> i) & 1;
    digitalWrite(PIN_TX, bitVal ? HIGH : LOW);
    delay(T_S);  // 100ms per bit
  }
}

//ADD PRINTING OUT OF THE SDF AND PREMABLE
//The function detectbyte takes the a byte parameted wantedByte and integer timeout (ms), Returns true if the wantedByte is detected, If the timeout integer (ms) is reached, the function returns false
bool detect_byte(byte wantedByte, int timeout_ms) { 
  unsigned long start = millis(); 
  //While loop that tries to find wantedbyte until the timeout_ms is reached. The "Or" statement gives readByte the value of the input byte from photo diode, but it looks for a full byte and then compares it to wantedbyte.
  while ((millis() - start) < timeout_ms) {
    byte readByte = 0;
    for (int i = 0; i < 8; i++) {
      readByte = (readByte << 1) | sh.sampleRecCh(PIN_RX);
      delay(T_S);  
    }
    if (readByte == wantedByte) return true;
  }
  return false;
}

