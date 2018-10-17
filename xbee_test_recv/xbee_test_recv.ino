// UART SPEED
const unsigned long UART_SPEED = 19200;

// MESSAGE LENGTH
const unsigned short MESSAGE_LENGTH = 32;
const unsigned short MESSAGE_LEN_WITHOUT_CHECKSUM = MESSAGE_LENGTH - (unsigned short)2;

// MESSAGE HEAD MIDDLE
const String MESSAGE_HEAD = ":FRONT";
const char HEAD_CHAR = MESSAGE_HEAD.charAt(0);
const unsigned char MESSAGE_HEAD_LENGTH = MESSAGE_HEAD.length();
const String MESSAGE_MIDDLE = "MIDDLE";
const char MIDDLE_CHAR = MESSAGE_MIDDLE.charAt(0);

// CYCLE TIME OF THE TASK TO SEND AN INFOMATION
const unsigned int SEND_RESULTS_CYCLE = 30000;
const unsigned int SEND_RESULTS_CYCLE_FIRSTSEND = 4000;
unsigned int sendResultCycle = SEND_RESULTS_CYCLE_FIRSTSEND;

unsigned long timeCounter = 0;

// RECEIVED MESSAGE COUNT
unsigned long receivedMessageCountOK = 0;
unsigned long receivedGoodByteNumber = 0;
unsigned long receivedColonCount = 0;
unsigned long checkSumErrCount = 0;
unsigned long checkOverFlowCount = 0;
unsigned long checkUARTErrCount = 0;
unsigned long sendResultByte = 0;

// RECEIVED MESSAGE COUNT WANTED
unsigned long shouldReceivedMessageCount = 0;
String sendCountInReceivedMessage = "";
const unsigned short SEND_COUNTER_POS_IN_MESSAGE = MESSAGE_HEAD_LENGTH;

// LED
const unsigned char LED_RED = 8;
const unsigned char LED_GREEN = 6;

// MESSAGE
String messageReceived = "";


void onoffLedGreen() {
  static bool LEDONOFF_GREED = false;
  digitalWrite(LED_GREEN, LEDONOFF_GREED);
  LEDONOFF_GREED = !LEDONOFF_GREED;
}

void onoffLedRed() {
  static bool LEDONOFF_RED = true;
  digitalWrite(LED_RED, LEDONOFF_RED);
  LEDONOFF_RED = !LEDONOFF_RED;
}

void setup()
{
  Serial.begin(UART_SPEED);
  pinMode(LED_RED, OUTPUT);
  digitalWrite(LED_RED, false);
  pinMode(LED_GREEN, OUTPUT);
  digitalWrite(LED_GREEN, true);
}

void clearMessage() {
  messageReceived = "";
}

// WHEN RECEIVED SOMETHING
void doReadSerial() {
  char chr = Serial.read();

  if (chr == HEAD_CHAR) {
    timeCounter = millis();
    clearMessage();
    messageReceived += chr;
    receivedColonCount++;
    receivedGoodByteNumber++;
  } else if (chr == (-1)) {
    checkUARTErrCount++;
    onoffLedRed();
  } else if (messageReceived.length() >= MESSAGE_LENGTH) {
    clearMessage();
    checkOverFlowCount++;
    onoffLedRed();
  } else {
    messageReceived += chr;
    receivedGoodByteNumber++;
  }
}

// CYCLELY MESSAGE CHECK
void doCheckMessage() {
  // THIS IS FOR SEND TIMES EXTRACTION
  sendCountInReceivedMessage = "";
  unsigned char middle_position = MESSAGE_LENGTH;

  // CALC CHECKSUM
  unsigned char checksum_calc = 0xfe;
  for (unsigned short checkCounti = 0; checkCounti < MESSAGE_LEN_WITHOUT_CHECKSUM; checkCounti++) {
    char chr = messageReceived.charAt(checkCounti);
    checksum_calc += chr;

    // EXTRACT SEND COUNT
    if ((checkCounti >= SEND_COUNTER_POS_IN_MESSAGE)
        && (checkCounti < middle_position)) {
      if (chr == MIDDLE_CHAR) {
        middle_position = checkCounti;
      } else if ((chr < '0') || (chr > '9')) {
        middle_position = checkCounti;
        sendCountInReceivedMessage = "ERR";
      } else {
        sendCountInReceivedMessage += chr;
      }
    }
  }

  // GET CHECKSUM IN THE MESSAGE
  String checksum_in_message = messageReceived.substring(MESSAGE_LEN_WITHOUT_CHECKSUM, MESSAGE_LENGTH);
  String checksum_calced_str = "";
  if (checksum_calc < 0x10) {
    checksum_calced_str += '0';
  }
  checksum_calced_str += String(checksum_calc, HEX);

  // CHECK IF CHECKSUM OK
  if (checksum_calced_str.equals(checksum_in_message)) {
    onoffLedGreen();
    receivedMessageCountOK++;
  } else {
    checkSumErrCount++;
    onoffLedRed();
  }
}


void sendResult() {
  unsigned long time_now = millis();
  if ((unsigned long)(time_now  - timeCounter) > sendResultCycle) {
    //Serial.print("[" + String(receivedMessageCountOK) + "/" + sendCountInReceivedMessage + "]");
    sendResultCycle = SEND_RESULTS_CYCLE;

    String pre_result = "";
    pre_result += "[";
    pre_result += "Byte:" + String(receivedGoodByteNumber);
    pre_result += "/OK:" + String(receivedMessageCountOK);
    pre_result += "/ERR:" + String(checkSumErrCount);
    pre_result += "/Colon:" + String(receivedColonCount);
    pre_result += "/Flo:" + String(checkOverFlowCount);
    pre_result += "/UART:" + String(checkUARTErrCount);
    pre_result += "/ResByte:";


    {
      String result_without_byteinfo = pre_result + "]";
      unsigned long result_without_byteinfo_length = result_without_byteinfo.length();
      
      unsigned long byteinfo_this_time = (unsigned long)sendResultByte
      + (unsigned long)result_without_byteinfo_length +
      + (unsigned long)(String(sendResultByte).length());
//    
//      Serial.print( "[Last:" + String(sendResultByte) + "/" 
//        + "PRE:" + result_without_byteinfo_length + "/"
//        + "LastNumLen:" + String(String(sendResultByte).length())
//        + "]" + "\n");
//      Serial.flush();

      String try_result_string = result_without_byteinfo +  String(byteinfo_this_time);
      unsigned long try_result_string_length =  try_result_string.length() + sendResultByte;

//      Serial.print( "[Text:" + try_result_string + "/" 
//        + "LEN:" + String(try_result_string_length) + "]"+ "\n");
//      Serial.flush();

      
      while (byteinfo_this_time < try_result_string_length) {
        byteinfo_this_time++;
        try_result_string = result_without_byteinfo +  String(byteinfo_this_time);
        try_result_string_length =  try_result_string.length() + sendResultByte;

//      Serial.print( "[Text:" + try_result_string + "/" 
//        + "LEN:" + try_result_string_length + "]"+ "\n");
//      Serial.flush();

      }

      sendResultByte = byteinfo_this_time;
    }
    
    String result = pre_result;
    result += String(sendResultByte);
    result += "]";
    Serial.print( result );
    timeCounter = time_now;
    digitalWrite(LED_RED, false);
    digitalWrite(LED_GREEN, true);
  }
}


void loop()
{
  unsigned short message_length_now = messageReceived.length() ;
  while (Serial.available() > 0) {
    doReadSerial();
  } 
  if ((message_length_now == MESSAGE_LENGTH) && (messageReceived.charAt(0) == HEAD_CHAR)) {
    sendResultCycle = SEND_RESULTS_CYCLE_FIRSTSEND;
    doCheckMessage();
    clearMessage();
  } else {
    sendResult();
  }
}

