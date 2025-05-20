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
// byte crc8(const byte *data, int len);
byte crc8();
byte Calculate_CRC(byte byteVal);

//
// Runtime
//

// Runtime variables
int my_address;
int sequence;
byte LED_PAYLOAD;
long ACK_timeout;
const long ACK_timeout_threshhold = 20000;
int retransmit_nbr;

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
  Serial.println("STARTING SETUP");
  Serial.begin(9600);
  sh.begin();
  sh.setMyAddress(1);
  my_address = sh.getMyAddress();
  sequence = 0;
  retransmit_nbr = 0;
  
  

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
      state = L1_RECEIVE;
      ACK_timeout = millis();
      
      // ---
      break;
      
    case L1_RECEIVE:
      Serial.println("[State] L1_RECEIVE");
      if (millis() - ACK_timeout > ACK_timeout_threshhold){
        Serial.println("ACK not recieved within threshhold");
        state = L2_RETRANSMIT;
        break;
      }
      // +++ add code here and to the predefined function boolean l1_receive(int timeout) below
      if (l1_receive(15000) == true){
        Serial.println("frame recieved");
        digitalWrite(DEB_2, HIGH);
      }
      else {
        // Serial.println("timeout reached");
        state = L2_RETRANSMIT;
        break;
      }
      state = L2_FRAME_REC;
      // ---
      break;
      
    case L2_DATA_SEND:
      Serial.println("[State] L2_DATA_SEND");
      // +++ add code here
      sequence ++;
      tx.frame_to = 2;
      tx.frame_from = sh.getMyAddress();
      tx.frame_type = FRAME_TYPE_DATA;
      tx.frame_seqnum = sequence;
      //tx.frame_payload = 12;
      tx.frame_payload = LED_PAYLOAD;
      // tx.frame_crc = Calculate_CRC(LED_PAYLOAD);
      // byte frameData[3] = { tx.frame_to, tx.frame_from, tx.frame_type,
      //   tx.frame_seqnum, tx.frame_payload };
      // byte type_seq = ((tx.frame_type << 4) | tx.frame_seqnum);
      // byte from_to =  ((tx.frame_from << 4) | tx.frame_to);
      // byte frameData[3] = { from_to, type_seq, tx.frame_payload };
      tx.frame_crc = crc8();
      // tx.frame_crc = 0;
      // tx.add_crc(Calculate_CRC(tx.frame_payload));
      Serial.print("tx.frame_crc: ");
      Serial.println(tx.frame_crc);
      tx.frame_generation();
      
      state = L1_SEND;
      // ---
      break;
      
    case L2_RETRANSMIT:
      Serial.println("[State] L2_RETRANSMIT");
      // +++ add code here
      if (retransmit_nbr >= 2){
        Serial.println("Retransmission limit exceeded, resetting");
        state = APP_PRODUCE;
        break;
      }
      state = L1_SEND;
      retransmit_nbr ++;
      // ---
      break;
      
    case L2_FRAME_REC:
      Serial.println("[State] L2_FRAME_REC");
      // +++ add code here
      //Serial.println(read_)
      rx.frame = recFrame;

      rx.frame_decompose();
      // ---
      state = L2_ACK_REC;
      break;
      
      
    case L2_ACK_SEND:
      Serial.println("[State] L2_ACK_SEND");
      // +++ add code here
      
      // ---
      break;
      
    case L2_ACK_REC:
      if (millis() - ACK_timeout > ACK_timeout_threshhold){
        Serial.println("ACK not recieved within threshhold");
        state = L2_RETRANSMIT;
        break;
      }
      Serial.println("[State] L2_ACK_REC");
      if (rx.frame_to != sh.getMyAddress()){
        Serial.println("Wrong adress, discarding frame!");
        state = L1_RECEIVE;
        break;
      }
      if (rx.frame_type != FRAME_TYPE_ACK){
        Serial.println("Data type not ACK, discarding frame!");
        state = L1_RECEIVE;
        break;
        
      }
      if(rx.frame_seqnum != sequence){
        Serial.println("Wrong sequence number, discarding frame!");
        state = L1_RECEIVE;
        break;
      }
      Serial.println("ACK Recieved");
      
      state = APP_PRODUCE;
      // +++ add code here
      
      // ---
      break;
      
    case APP_PRODUCE:
      Serial.println("[State] APP_PRODUCE");
      // +++ add code here
      LED_PAYLOAD = sh.select_led();
      state = L2_DATA_SEND;
      retransmit_nbr = 0;

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

boolean l1_receive(int timeout) {
  const byte generator = 0xA7;
  recFrame = 0;
  long time = millis();
  int foundBit;
  byte CRC = 0;
  bool preambleOK = detect_byte(PREAMBLE_SEQ, timeout);
  if (preambleOK){
    Serial.println("Preamble found");
    bool SFDOK = detect_byte(SFD_SEQ, timeout);
    if (SFDOK){
      Serial.println("SFD found");
      digitalWrite(DEB_1, HIGH);
      // decoding frame
      for (int i = 0; i <= 31; i++) {
        foundBit = sh.sampleRecCh(PIN_RX);
        CRC ^= (foundBit << 7);
        // if (((CRC >> 7) & 1) == 1){
        //   CRC = CRC ^ generator;
        // }
        if (CRC & 0x80)
            CRC = (CRC << 1) ^ generator;
        else
            CRC <<= 1;
        // CRC = (CRC << 1) | foundBit;
        digitalWrite(DEB_2, foundBit);
        recFrame = (recFrame << 1) | foundBit;
        delay(T_S);  // 100ms per bit
      }
    //   for (int i = 0; i < FRAME_BITS; i++) {
    //     int b = sh.sampleRecCh(PIN_RX);
    //     // inject the bit at the top
    //     // now shift one step and conditional XOR
    //     // record the bit in your frame buffer, etc.
    // }
      if (CRC == 0){
        Serial.println("CRC validated, no error");
      }else {
        Serial.println("CRC not validated, error found");
        Serial.println(recFrame);
        return false;
      }
      Serial.println(recFrame);
    }
  }
  else {
    return false;
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

long read_next_bits(long data, int length){
  long result = 0;
  for (int i = length-1; i >= 0; i--) {
    int bits = (data >> i) & 1;
    result = result + bits*pow(2,i);
  }
  return result;
}

//void record_frame(long data, int framelen){
//  for (int i = framelen-1; i >= 0; i--) {
//      int bitVal = (frame >> i) & 1;
//}

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



byte Calculate_CRC(byte byteVal) {
    const byte generator = 0xA7;
    byte crc = byteVal; /* init crc directly with input byte instead of 0, avoid useless 8 bitshifts until input byte is in crc register */

    for (int i = 0; i < 8; i++)
    {
        if ((crc & 0x80) != 0)
        { /* most significant bit set, shift crc register and perform XOR operation, taking not-saved 9th set bit into account */
            crc = (byte)((crc << 1) ^ generator);
        }
        else
        { /* most significant bit not set, go to next bit */
            crc <<= 1;
        }
    }

    Serial.print("Generated CRC: ");
    Serial.println(crc);
    return crc;
}

// byte crc8(const byte *data, int len) {
byte crc8() {
  const byte generator = 0xA7;
  const byte type_seq = ((tx.frame_type << 4) | tx.frame_seqnum);
  const byte from_to =  ((tx.frame_from << 4) | tx.frame_to);
  const byte data[3] = { from_to, type_seq, tx.frame_payload };
  byte crc = 0;
  for (int i = 0; i < 3; i++) {
    byte b = data[i];
    // feed each bit MSB-first
    for (int bit = 7; bit >= 0; bit--) {
      bool inBit = (b >> bit) & 1;
      // inject at top
      crc ^= (inBit << 7);
      // shift & conditionally XOR
      if (crc & 0x80)
        crc = (crc << 1) ^ generator;
      else
        crc <<= 1;
    }
  }
  return crc;
}