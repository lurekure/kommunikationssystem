////////////////////////////
//
// V21.1 Fixed.ino
//
//
// 2025-05-20 ChatGPT
//
////////////////////////////

//
// Select library
#include <datacommlib.h>

//
// Prototypes
//
void    l1_send(unsigned long l2frame, int framelen);
boolean l1_receive(int timeout);
void    send_byte(byte data);
bool    detect_byte(byte data, int timeout);
long    read_next_bits(long data, int length);
byte    crc8(const byte *data, int len);

//
// Runtime variables
//
int     my_address;
int     sequence;
byte    LED_PAYLOAD;
unsigned long ACK_timeout;
const unsigned long ACK_timeout_threshold = 25000;
int     retransmit_nbr;
int state = NONE;

Shield  sh;
Transmit tx;
Receive  rx;
unsigned long recFrame;

//
// Setup
//
void setup() {
  Serial.begin(9600);
  Serial.println("STARTING SETUP");
  sh.begin();
  sh.setMyAddress(1);
  my_address = sh.getMyAddress();
  sequence = -1;
  retransmit_nbr = 0;
  state = APP_PRODUCE;
}

//
// Main loop / state machine
//
void loop() {
  switch (state) {

    case L1_SEND:
      Serial.println("[State] L1_SEND");
      l1_send(tx.frame, LEN_FRAME);
      ACK_timeout = millis();
      state = L1_RECEIVE;
      break;

    case L1_RECEIVE:
      Serial.println("[State] L1_RECEIVE");
      if (millis() - ACK_timeout > ACK_timeout_threshold) {
        Serial.println("ACK not received in time");
        state = L2_RETRANSMIT;
        break;
      }
      if (!l1_receive(ACK_timeout_threshold)) {
        state = L2_RETRANSMIT;
        break;
      }
      Serial.println("Frame received");
      digitalWrite(DEB_2, HIGH);
      state = L2_FRAME_REC;
      break;

    case L2_DATA_SEND:
      Serial.println("[State] L2_DATA_SEND");
      sequence++;
      tx.frame_to      = sh.get_address();
      tx.frame_from    = my_address;
      tx.frame_type    = FRAME_TYPE_DATA;
      tx.frame_seqnum  = sequence;
      tx.frame_payload = LED_PAYLOAD;

      // --- build the 3‐byte header+payload array, compute CRC8 over it
      {
        byte type_seq = (tx.frame_type << 4) | tx.frame_seqnum;
        byte from_to  = (tx.frame_from << 4) | tx.frame_to;
        const byte frameData[3] = { from_to, type_seq, tx.frame_payload };
        tx.frame_crc = crc8(frameData, 3);
      }
      Serial.print("tx.frame_crc: ");
      Serial.println(tx.frame_crc);
      tx.frame_generation();

      state = L1_SEND;
      break;

    case L2_RETRANSMIT:
      Serial.println("[State] L2_RETRANSMIT");
      if (retransmit_nbr >= 2) {
        Serial.println("Retransmit limit exceeded, resetting");
        state = APP_PRODUCE;
        retransmit_nbr = 0;
        break;
      }
      retransmit_nbr++;
      state = L1_SEND;
      break;

    case L2_FRAME_REC:
      Serial.println("[State] L2_FRAME_REC");
      rx.frame = recFrame;
      rx.frame_decompose();
      state = L2_ACK_REC;
      break;

    case L2_ACK_REC:
      Serial.println("[State] L2_ACK_REC");
      if (millis() - ACK_timeout > ACK_timeout_threshold) {
        Serial.println("ACK not received in time");
        state = L2_RETRANSMIT;
        break;
      }
      if (rx.frame_to     != my_address ||
          rx.frame_type   != FRAME_TYPE_ACK ||
          rx.frame_seqnum != sequence) {
        Serial.println("ACK validation failed, discarding");
        state = L1_RECEIVE;
        break;
      }
      Serial.println("ACK received successfully");
      state = APP_PRODUCE;
      break;

    case APP_PRODUCE:
      Serial.println("[State] APP_PRODUCE");
      LED_PAYLOAD = sh.select_led();
      retransmit_nbr = 0;
      state = L2_DATA_SEND;
      break;

    case APP_ACT:
      Serial.println("[State] APP_ACT");
      Serial.println(sh.select_led());
      break;

    case HALT:
      Serial.println("[State] HALT");
      sh.halt();
      break;

    default:
      Serial.println("UNDEFINED STATE");
      break;
  }
}

//
// Physical send (preamble + SFD + bits)
//
void l1_send(unsigned long frame, int framelen) {
  send_byte(PREAMBLE_SEQ);
  send_byte(SFD_SEQ);
  for (int i = framelen - 1; i >= 0; i--) {
    digitalWrite(PIN_TX, (frame >> i) & 1 ? HIGH : LOW);
    delay(T_S);
  }
}

//
// Physical receive + CRC‐8 check (generator 0xA7)
//
boolean l1_receive(int timeout) {
  const byte generator = 0xA7;
  recFrame = 0;
  byte CRC = 0;

  if (!detect_byte(PREAMBLE_SEQ, timeout)) return false;
  Serial.println("Preamble found");

  if (!detect_byte(SFD_SEQ, timeout)) return false;
  Serial.println("SFD found");
  digitalWrite(DEB_1, HIGH);

  // Read 32 bits (3 data bytes + 1 CRC byte)
  for (int i = 0; i < 32; i++) {
    int bit = sh.sampleRecCh(PIN_RX);
    // inject received bit into MSB
    CRC ^= (bit << 7);
    // shift+conditional XOR
    if (CRC & 0x80)
      CRC = (CRC << 1) ^ generator;
    else
      CRC <<= 1;
    // assemble the raw frame
    recFrame = (recFrame << 1) | bit;
    digitalWrite(DEB_2, bit);
    delay(T_S);
  }

  if (CRC != 0) {
    Serial.println("CRC check failed");
    return false;
  }
  Serial.println("CRC validated, no error");
  return true;
}

//
// Bit-bang one byte
//
void send_byte(byte data) {
  for (int i = 7; i >= 0; i--) {
    digitalWrite(PIN_TX, (data >> i) & 1 ? HIGH : LOW);
    delay(T_S);
  }
}

//
// Byte-synchronizer
//
// bool detect_byte(byte wanted, int timeout) {
//   unsigned long start = millis();
//   Serial.print("Searching for sync byte: 0x");
//   Serial.println(wanted, HEX);
//   while (millis() - start < timeout) {
//     // try reading 8 bits
//     byte candidate = 0;
//     for (int i = 7; i >= 0; i--) {
//       int b = sh.sampleRecCh(PIN_RX);
//       candidate = (candidate << 1) | b;
//       delay(T_S);
//     }
//     if (candidate == wanted) {
//       Serial.print("Found sync byte: 0x");
//       Serial.println(wanted, HEX);
//       return true;
//     }
//   }
//   Serial.println("detect_byte: timeout");
//   return false;
// }
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
      Serial.print(foundBit);
      Serial.print(" ");
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
//
// Compute CRC‐8 (gen=0xA7) over an array of len bytes
//
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

//
// Helper: read next <length> bits from <data> (unused here)
//
long read_next_bits(long data, int length) {
  long result = 0;
  for (int i = length - 1; i >= 0; i--) {
    result = (result << 1) | ((data >> i) & 1);
  }
  return result;
}