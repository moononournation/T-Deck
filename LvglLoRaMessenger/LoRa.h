#pragma once

#define RADIO_FREQ 868.0
SX1262 radio = new Module(TDECK_RADIO_CS, TDECK_RADIO_DIO1, TDECK_RADIO_RST, TDECK_RADIO_BUSY);
// #define RADIO_FREQ          433.0
// SX1268 radio = new Module(TDECK_RADIO_CS, TDECK_RADIO_DIO1, TDECK_RADIO_RST, TDECK_RADIO_BUSY);

bool transmissionFlag = true;
bool isEnableInterrupt = true;
int transmissionState;
bool hasRadio = false;
bool hasMsgToSend = false;
String strTextInput;
bool isWaitingMsgSent = false;
uint32_t sendCount = 0;
uint32_t runningMillis = 0;
SemaphoreHandle_t xSemaphore = NULL;

void setFlag(void)
{
  // check if the interrupt is enabled
  if (!isEnableInterrupt)
  {
    return;
  }
  // we got a packet, set the flag
  transmissionFlag = true;
}

bool setupRadio()
{
  // Add mutex to allow multitasking access
  xSemaphore = xSemaphoreCreateBinary();
  assert(xSemaphore);
  xSemaphoreGive(xSemaphore);

  int state = radio.begin(RADIO_FREQ);
  if (state == RADIOLIB_ERR_NONE)
  {
    Serial.println("Start Radio success!");
  }
  else
  {
    Serial.print("Start Radio failed,code:");
    Serial.println(state);
    return false;
  }

  hasRadio = true;

  // set carrier frequency to 868.0 MHz
  if (radio.setFrequency(RADIO_FREQ) == RADIOLIB_ERR_INVALID_FREQUENCY)
  {
    Serial.println(F("Selected frequency is invalid for this module!"));
    return false;
  }

  // set bandwidth to 250 kHz
  if (radio.setBandwidth(250.0) == RADIOLIB_ERR_INVALID_BANDWIDTH)
  {
    Serial.println(F("Selected bandwidth is invalid for this module!"));
    return false;
  }

  // set spreading factor to 10
  if (radio.setSpreadingFactor(10) == RADIOLIB_ERR_INVALID_SPREADING_FACTOR)
  {
    Serial.println(F("Selected spreading factor is invalid for this module!"));
    return false;
  }

  // set coding rate to 6
  if (radio.setCodingRate(6) == RADIOLIB_ERR_INVALID_CODING_RATE)
  {
    Serial.println(F("Selected coding rate is invalid for this module!"));
    return false;
  }

  // set LoRa sync word to 0xAB
  if (radio.setSyncWord(0xAB) != RADIOLIB_ERR_NONE)
  {
    Serial.println(F("Unable to set sync word!"));
    return false;
  }

  // set output power to 10 dBm (accepted range is -17 - 22 dBm)
  if (radio.setOutputPower(17) == RADIOLIB_ERR_INVALID_OUTPUT_POWER)
  {
    Serial.println(F("Selected output power is invalid for this module!"));
    return false;
  }

  // set over current protection limit to 140 mA (accepted range is 45 - 140 mA)
  // NOTE: set value to 0 to disable overcurrent protection
  if (radio.setCurrentLimit(140) == RADIOLIB_ERR_INVALID_CURRENT_LIMIT)
  {
    Serial.println(F("Selected current limit is invalid for this module!"));
    return false;
  }

  // set LoRa preamble length to 15 symbols (accepted range is 0 - 65535)
  if (radio.setPreambleLength(15) == RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH)
  {
    Serial.println(F("Selected preamble length is invalid for this module!"));
    return false;
  }

  // disable CRC
  if (radio.setCRC(false) == RADIOLIB_ERR_INVALID_CRC_CONFIGURATION)
  {
    Serial.println(F("Selected CRC is invalid for this module!"));
    return false;
  }

  // set the function that will be called
  // when new packet is received
  radio.setDio1Action(setFlag);

  if (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE)
  {
    if (hasRadio)
    {
      int state = radio.startReceive();
      if (state == RADIOLIB_ERR_NONE)
      {
        Serial.println(F("success!"));
      }
      else
      {
        Serial.print(F("failed, code "));
        Serial.println(state);
      }
    }
    xSemaphoreGive(xSemaphore);
  }

  return true;
}

void loopRadio()
{
  if (!hasRadio)
  {
    Serial.println("Radio not online!");
    return;
  }
  if (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE)
  {
    digitalWrite(TDECK_SDCARD_CS, HIGH);
    digitalWrite(TDECK_RADIO_CS, HIGH);
    digitalWrite(TDECK_TFT_CS, HIGH);

    if (hasMsgToSend)
    {
      if (isWaitingMsgSent)
      {
        // check if the previous transmission finished
        if (transmissionFlag)
        {
          // disable the interrupt service routine while
          // processing the data
          isEnableInterrupt = false;
          // reset flag
          transmissionFlag = false;

          if (transmissionState == RADIOLIB_ERR_NONE)
          {
            // packet was successfully sent
            Serial.println(F("transmission finished!"));
            // NOTE: when using interrupt-driven transmit method,
            //       it is not possible to automatically measure
            //       transmission data rate using getDataRate()
          }
          else
          {
            Serial.print(F("failed, code "));
            Serial.println(transmissionState);
          }

          append_message(strTextInput);
          resumeTextInput();

          hasMsgToSend = false;
          isWaitingMsgSent = false;

          radio.startReceive();

          // we're ready to send more packets,
          // enable interrupt service routine
          isEnableInterrupt = true;
        }
      }
      else
      {
        Serial.print(F("Transmitting msg: "));
        Serial.println(strTextInput);

        // you can also transmit byte array up to 256 bytes long
        transmissionState = radio.startTransmit(strTextInput.c_str());
        isWaitingMsgSent = true;

        // we're ready to send more packets,
        // enable interrupt service routine
        isEnableInterrupt = true;
      }
    }
    else
    {
      String recv;

      // check if the flag is set
      if (transmissionFlag)
      {
        // disable the interrupt service routine while
        // processing the data
        isEnableInterrupt = false;

        // reset flag
        transmissionFlag = false;

        int state = radio.readData(recv);
        if (state == RADIOLIB_ERR_NONE)
        {
          // packet was successfully received
          Serial.print(F("[RADIO] Received packet!"));

          // print data of the packet
          Serial.print(F(" Data:"));
          Serial.print(recv);

          // print RSSI (Received Signal Strength Indicator)
          Serial.print(F(" RSSI:"));
          Serial.print(radio.getRSSI());
          Serial.print(F(" dBm"));
          // snprintf(dispRecvicerBuff[1], sizeof(dispRecvicerBuff[1]), "RSSI:%.2f dBm", radio.getRSSI());

          // print SNR (Signal-to-Noise Ratio)
          Serial.print(F("  SNR:"));
          Serial.print(radio.getSNR());
          Serial.println(F(" dB"));

          append_message(recv);
        }
        else if (state == RADIOLIB_ERR_CRC_MISMATCH)
        {
          // packet was received, but is malformed
          Serial.println(F("CRC error!"));
        }
        else
        {
          // some other error occurred
          Serial.print(F("failed, code "));
          Serial.println(state);
        }
        // put module back to listen mode
        radio.startReceive();

        // we're ready to receive more packets,
        // enable interrupt service routine
        isEnableInterrupt = true;
      }
    }
    xSemaphoreGive(xSemaphore);

    digitalWrite(TDECK_SDCARD_CS, HIGH);
    digitalWrite(TDECK_RADIO_CS, HIGH);
    digitalWrite(TDECK_TFT_CS, HIGH);
  }
}
