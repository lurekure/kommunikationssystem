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
byte    crc8(const byte *data, int len);




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
//lab4 global variables
int tx_seqnum;
int expected_seqnum;
int calculated_crc;

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

int my_address = 1; //Hard coded address
sh.setMyAddress(my_address);

tx_seqnum = 0;
expected_seqnum = 0;

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
      if (l1_receive(15000) == true){
        Serial.println("frame recieved");

        //are we waiting for ack or data?
        if(tx.frame_type == FRAME_TYPE_DATA){
          state = L2_ACK_REC; //If we sent data, we expect ACK
        }
        else{
          state = L2_FRAME_REC; //If we send ACK, we expect DATA
        }

      }
      else {
        Serial.println("timeout reached");
        state = L2_RETRANSMIT;
      }
      // ---
      break;

    case L2_DATA_SEND:
      Serial.println("[State] L2_DATA_SEND");
      // +++ add code here
      tx.frame_to = sh.get_address();
      tx.frame_from = sh.getMyAddress();
      tx.frame_type = FRAME_TYPE_DATA;
      tx.frame_seqnum = tx_seqnum;
      tx.frame_payload = LED_PAYLOAD;
      
      
      //Computing 
      {
      byte tx_type_seq = (tx.frame_type << 4) | tx.frame_seqnum;
      byte tx_from_to  = (tx.frame_from << 4) | tx.frame_to;
      const byte tx_frameData[3] = { tx_from_to, tx_type_seq, tx.frame_payload };
      tx.frame_crc = crc8(tx_frameData, 3);
      }
      
    
      
      tx.frame_generation();
      state = L1_SEND;
      // ---
      break;

    case L2_RETRANSMIT:
      Serial.println("[State] L2_RETRANSMIT");
      // +++ add code here

      state = L1_SEND;
      // ---
      break;

    case L2_FRAME_REC:
      Serial.println("[State] L2_FRAME_REC");
      // +++ add code here
      rx.frame = recFrame; //Puts the recieved frame recFrame into rx.frame object
      rx.frame_decompose(); //Decomposes the frame (32bit) into it's parts
      
      {
      byte rx_type_seq = (rx.frame_type << 4) | rx.frame_seqnum;
      byte rx_from_to  = (rx.frame_from << 4) | rx.frame_to;
      const byte rx_frameData[3] = { rx_from_to, rx_type_seq, rx.frame_payload };
      int calculated_crc = crc8(rx_frameData, 3);
      }

        if (rx.frame_to != sh.getMyAddress()) {
          Serial.print("Frame not for me. My address: ");
          Serial.print(sh.getMyAddress());
          Serial.print(", frame to: ");
          Serial.println(rx.frame_to);
    
        // Silently drop frame by going back to listening
        state = L1_RECEIVE;
        break;
        }

      //Checking if CRC is ok 
      if (calculated_crc != rx.frame_crc){
        Serial.println("CRC does not match, frame corrupted");
        state = L1_RECEIVE;
        break;
      }
      
        /*LAB4 changes - if recieved frame is of type data -> send ack
        * If recieved frame is of type data but wrong expected seq-number -> do not not change expected seqnumber and send ack packet
        */
        if (rx.frame_type == FRAME_TYPE_DATA){
          if (rx.frame_seqnum == expected_seqnum){
            Serial.println("Correct data frame recieved");

            //If we should act on data, it should be done here

            expected_seqnum = (expected_seqnum + 1) % 16;
          }
          else{
            Serial.println("Duplicate data frame recieved");
          }

          state = L2_ACK_SEND;
        }
        else{
          Serial.println("Non-data frame recieved");
          state = L1_RECEIVE;
        }

      break;


    case L2_ACK_SEND:
      Serial.println("[State] L2_ACK_SEND");
      // +++ add code here
      tx.frame_to = rx.frame_from;
      tx.frame_from = sh.getMyAddress();
      tx.frame_type = FRAME_TYPE_ACK;
      tx.frame_seqnum = rx.frame_seqnum;
      tx.frame_payload = 0;
      //tx.frame_crc = crc8_from_frame(tx.frame);
      tx.frame_generation();


      
      state = L1_SEND;
      // ---
      break;

    case L2_ACK_REC:
      Serial.println("[State] L2_ACK_REC");
      // +++ add code here
      rx.frame = recFrame;
      rx.frame_decompose();

      if (rx.frame_to != sh.getMyAddress()){
        Serial.println("ACK not for me");
        state = L1_RECEIVE;
        break;
      }

      if(rx.frame_type == FRAME_TYPE_ACK && rx.frame_seqnum == tx_seqnum){
        Serial.println("ACK recieved and matches seqnum");
        tx_seqnum = (tx_seqnum + 1) % 16;
        state = APP_PRODUCE;
      }
      else{
        Serial.println("Wrong or missing ACK");
        state = L2_RETRANSMIT;
      }
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
      digitalWrite(DEB_1, HIGH); //Turns on debug led 1
      Serial.println("SFD found");
      // decoding frame
      for (int i = 0; i <= 31; i++) {
        foundBit = sh.sampleRecCh(PIN_RX);
        recFrame = (recFrame << 1) | foundBit;
        //Debug
        if (foundBit == 1){
          digitalWrite(DEB_2, HIGH);
        }
        else{
          digitalWrite(DEB_2,LOW);
        }
        delay(T_S);  // 100ms per bit
      }
      digitalWrite(DEB_2, HIGH); //Turns on debug led 2
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

bool detect_byte(byte wantedByte, int timeout){
  long time = millis();
  bool synched = false;
  Serial.print("Searching for byte: ");
  Serial.println(wantedByte);
  while (synched == false){
    byte foundByte = 0;
    for (int i = 7; i >= 0; i--) {
      int correctBit = (wantedByte >> i) & 1;
      int foundBit = sh.sampleRecCh(PIN_RX);
      if (correctBit != foundBit){
        break;
      }
      foundByte = (foundByte << 1) | foundBit;
      if (i == 0){
        synched = true;
        Serial.print("Found byte: ");
        Serial.println(foundByte);
      }
      delay(T_S);  // 100ms per bit
    }
    if (millis()-time >= timeout){
      Serial.println("timeout reached");
      return false;
    }
  }
	return true;
}

byte crc8(const byte *data, int len) {
  const byte generator = 0xA7;
  byte crc = 0;
  for (int i = 0; i < len; i++) {
    byte b = data[i];
    for (int bit = 7; bit >= 0; bit--) {
      bool inBit = (b >> bit) & 1;
      crc ^= (inBit << 7);
      if (crc & 0x80)
        crc = (crc << 1) ^ generator;
      else
        crc <<= 1;
    }
  }
  return crc;
}