
#include <KonnektingDevice.h>
#include "kdevice_KNX_EnOcean_Gateway.h"
#include "Timer.h"
#include "EEPROM.h"
#include "Enocean.h"
#include <Wire.h>


// ################################################
// ### DEBUG CONFIGURATION
// ################################################
// I2C-MODE
//#define I2COFF // comment this line to disable I2C mode
// DEBUG-MODE
//#define KDEBUG // comment this line to disable DEBUG mode


#ifdef KDEBUG
#include <DebugUtil.h>

// Get correct serial port for debugging
#ifdef __AVR_ATmega32U4__
// Leonardo/Micro/ProMicro use the USB serial port
#define DEBUGSERIAL Serial
#elif __SAMD21G18A__
// Zero use
#define DEBUGSERIAL SerialUSB
#elif ESP8266
// ESP8266 use the 2nd serial port with TX only
#define DEBUGSERIAL Serial1
#else
// All other, (ATmega328P f.i.) use software serial
#include <SoftwareSerial.h>
SoftwareSerial softserial(11, 10); // RX, TX
#define DEBUGSERIAL softserial
#endif
// end of debugging defs
#endif


// ################################################
// ### KONNEKTING Configuration
// ################################################
#ifdef __AVR_ATmega328P__
#define KNX_SERIAL Serial // Nano/ProMini etc. use Serial
#elif ESP8266
#define KNX_SERIAL Serial // ESP8266 use Serial
#else
#define KNX_SERIAL Serial1 // Leonardo/Micro/Zero etc. use Serial1
#endif

// ################################################
// ### IO Configuration
// ################################################
#define PROG_LED_PIN    A2
#define PROG_BUTTON_PIN A1
#define LED_YELLOW      25




// ################################################
// ### define Parameter
// ################################################
#define delayReleaseMSG 100

uint8_t lui8_BaseID_p[4];
uint8_t lui8_SendeID_p[10][4];

uint32_t Taster_0_19_ID[20];
uint8_t  Taster_array_0_19_ID[20][4];
uint32_t Aktor_0_9_ID[10];
uint8_t  Aktor_array_0_9_ID[10][4];

boolean Taster_0_19_State[21];
uint8_t Taster_0_19_Comand[21];
unsigned long     Taster_0_19_time[21] = {0};

boolean ButtonIsPressed[60][2] = {0};

// ################################################
// ### Statische Variablen Botschaften
// ################################################
static PACKET_SERIAL_TYPE m_Pkt_st;
byte readBuf;
byte headerBuf[5];
uint8_t u8datBuf[DATBUF_SZ];



// ################################################
// ### Funktion
// ################################################

//****************** Save Paramter **************************************************
void SAVE_State() {
  digitalWrite(LED_YELLOW, HIGH);
}

//****************** get BaseID   ***************************************************
void getBaseID(uint8_t* fui8_BaseID_p)
{
  PACKET_SERIAL_TYPE lRdBaseIDPkt_st;

  uint8_t lu8SndBuf = u8CO_RD_IDBASE;

  lRdBaseIDPkt_st.u16DataLength  = 0x0001;
  lRdBaseIDPkt_st.u8OptionLength = 0x00;
  lRdBaseIDPkt_st.u8Type  	     = u8RORG_COMMON_COMMAND;
  lRdBaseIDPkt_st.u8DataBuffer   = &lu8SndBuf;

  // Swap data length bytes to little endian

  // uint8_t temp;
  // temp = lRdBaseIDPkt_st.u16DataLength[0];
  // lRdBaseIDPkt_st.u16DataLength[0] = lRdBaseIDPkt_st.u16DataLength[1];
  // lRdBaseIDPkt_st.u16DataLength[1] = temp;

#ifdef KDEBUG
  SerialUSB.println("Sending telegram (read base ID).");
#endif

  if ( OK == uart_sendPacket(&lRdBaseIDPkt_st) )
  {
    u8RetVal = NO_RX_TEL;
#ifdef KDEBUG
    SerialUSB.println("Receiving telegram (read base ID).");
#endif
    while (u8RetVal == NO_RX_TEL)
    {
      u8RetVal = uart_getPacket(&m_Pkt_st, (uint16_t) DATBUF_SZ);
    }

    switch (u8RetVal)
    {
      case OK:
        {
#ifdef KDEBUG
          SerialUSB.println("Data: ");
          for (int i = 0; i < m_Pkt_st.u16DataLength + (uint16_t)m_Pkt_st.u8OptionLength; i++)
          {

            SerialUSB.print(m_Pkt_st.u8DataBuffer[i], HEX);
            SerialUSB.print(" ");

          }
          SerialUSB.println("");
#endif

          switch (m_Pkt_st.u8Type)
          {
            case u8RESPONSE:
              {
#ifdef KDEBUG
                SerialUSB.println("Received Response.");
#endif
                // todo define number of base id bytes
                for (int i = 0; i < 4; i++)
                {
                  memcpy((void*) & (fui8_BaseID_p[i]), (void*) & (m_Pkt_st.u8DataBuffer[i + 1]), 1);
                }

              }
              break;
            default:
              {
#ifdef KDEBUG
                SerialUSB.print("Wrong packet type. Expected reponse. Received: ");
                SerialUSB.println(m_Pkt_st.u8Type, HEX);
#endif
              }
          }
        }
        break;
      default:
        {
#ifdef KDEBUG
          SerialUSB.println("Error receiving base ID");
#endif
        }
    }
  }
}


//****************** Read EnOcean Message *******************************************
void getEnOceanMSG(uint8_t u8RetVal, PACKET_SERIAL_TYPE* f_Pkt_st)
{
  switch (u8RetVal)
  {
    case OK:
      {
        //SerialUSB.println("Successfully received packet");
#ifdef KDEBUG
        SerialUSB.println("Data: ");
        for (int i = 0; i < f_Pkt_st->u16DataLength + (uint16_t)f_Pkt_st->u8OptionLength; i++)
        {

          SerialUSB.print(f_Pkt_st->u8DataBuffer[i], HEX);
          SerialUSB.print(" ");

        }
        SerialUSB.println("");
#endif

        switch (f_Pkt_st->u8Type)
        {
          case u8RADIO_ERP1:
            {
              switch (f_Pkt_st->u8DataBuffer[0])
              {
                // Taster
                case u8RORG_RPS:
                  {
                    RPS_TELEGRAM_TYPE* lRpsTlg_p = (RPS_TELEGRAM_TYPE*) & (f_Pkt_st->u8DataBuffer[1]);
#ifdef KDEBUG
                    SerialUSB.println("Received RPS telegram.");
#endif

                    // Abfrage der 10 Taster als Input)
                    for (int n = 0; n < 10; n++)
                    {
                      if (lRpsTlg_p->u8SenderId_p[0] == Taster_array_0_19_ID[n][0])
                      {
                        if (lRpsTlg_p->u8SenderId_p[1] == Taster_array_0_19_ID[n][1])
                        {
                          if (lRpsTlg_p->u8SenderId_p[2] == Taster_array_0_19_ID[n][2])
                          {
                            if (lRpsTlg_p->u8SenderId_p[3] == Taster_array_0_19_ID[n][3])
                            {
#ifdef KDEBUG
                              SerialUSB.print("deteckt: Taster_");
                              SerialUSB.println(n);
#endif
                              // Check welcher Taster gedrückt ist

                              if (RPS_BUTTON_2NDACT_VALID == lRpsTlg_p->u8RpsTelData.SA)
                              {
                                switch (lRpsTlg_p->u8RpsTelData.R2)
                                {
                                  case RPS_BUTTON_CHANNEL_AO:
#ifdef KDEBUG
                                    SerialUSB.println("2nd action: Button DO");
#endif
                                    break;
                                  case RPS_BUTTON_CHANNEL_AI:
#ifdef KDEBUG
                                    SerialUSB.println("2nd action: Button DI");
#endif
                                    break;
                                  case RPS_BUTTON_CHANNEL_BO:
#ifdef KDEBUG
                                    SerialUSB.println("2nd action: Button CO");
#endif
                                    if (1 == lRpsTlg_p->u8RpsTelData.EB) //Pressed
                                    {
                                      ButtonIsPressed[n*6+ 2][0] = true;
                                      ButtonIsPressed[n*6+ 2][1] = 0;
                                      Knx.write((n * 6) + 2, 0);
                                    }
                                    break;
                                  case RPS_BUTTON_CHANNEL_BI:
#ifdef KDEBUG
                                    SerialUSB.println("2nd action: Button CI");
#endif
                                    if (1 == lRpsTlg_p->u8RpsTelData.EB) //Pressed
                                    {
                                      ButtonIsPressed[n*6+2][0] = true;
                                      ButtonIsPressed[n*6+2][1] = 1;
                                      Knx.write((n * 6) + 2, 1);
                                    }
                                    break;
                                }
                              }
                              else
                              {
                                switch (lRpsTlg_p->u8RpsTelData.R1)
                                {
                                  case RPS_BUTTON_CHANNEL_AO:
#ifdef KDEBUG
                                    SerialUSB.println("Button AO");
#endif
                                    if (1 == lRpsTlg_p->u8RpsTelData.EB) //ob pressed ist
                                    {
                                      ButtonIsPressed[n*6][0] = true;
                                      ButtonIsPressed[n*6][1] = 0;
                                      Knx.write(n * 6, 0);
                                    }
                                    break;
                                  case RPS_BUTTON_CHANNEL_AI:

                                    if (0 == lRpsTlg_p->u8RpsTelData.EB)
                                    {
#ifdef KDEBUG
                                      SerialUSB.println("Switch released");
#endif
                                      for (int i = 0; i < 60 ; i++)
                                      {
                                       if(ButtonIsPressed[i][0]==true)
                                           {
                                            ButtonIsPressed[i][0]=false;
                                            bool state = ButtonIsPressed[i][1];
                                            Knx.write(i+3,state);
                                           }
                                      }


                                    }
                                    else if (1 == lRpsTlg_p->u8RpsTelData.EB) //ob pressed ist
                                    {
#ifdef KDEBUG
                                      SerialUSB.println("Button AI");
#endif
                                      ButtonIsPressed[n*6][0] = true;
                                      ButtonIsPressed[n*6][1] = 1;
                                      Knx.write(n * 6, 1);
                                    }
                                    break;
                                  case RPS_BUTTON_CHANNEL_BO:
#ifdef KDEBUG
                                    SerialUSB.println("Button BO");
#endif
                                    if (1 == lRpsTlg_p->u8RpsTelData.EB) //ob pressed ist
                                    { 
                                      ButtonIsPressed[n*6+ 1][0] = true;
                                      ButtonIsPressed[n*6+ 1][1] = 0;
                                      Knx.write((n * 6) + 1, 0);
                                    }
                                    break;
                                  case RPS_BUTTON_CHANNEL_BI:
#ifdef KDEBUG
                                    SerialUSB.println("Button BI");
#endif
                                    if (1 == lRpsTlg_p->u8RpsTelData.EB) //ob pressed ist
                                    {
                                      ButtonIsPressed[n*6+ 1][0] = true;
                                      ButtonIsPressed[n*6+ 1][1] = 1;
                                      Knx.write((n * 6) + 1, 1);
                                    }
                                    break;
                                } //ENDE Switch
                              }//Ende Else 2nd Action
                            } //ENDE ID3
                          } //ENDE ID2
                        } //ENDE ID1
                      } // ENDE ID0
                    } // ENDE FOR Schleife für 10 Taster Input



                    /*
                      SerialUSB.print("Sender ID: ");
                      for (int i = 0; i < 4; i++)
                      {
                      SerialUSB.print(lRpsTlg_p->u8SenderId_p[i], HEX);
                      //SerialUSB.print(Taster_array_0_19_ID[0][i], HEX);
                      }
                      SerialUSB.println("");
                    */
                    /*
                      if (0 == lRpsTlg_p->u8RpsTelData.EB)
                      {
                      SerialUSB.println("Switch released");
                      }
                      else if (1 == lRpsTlg_p->u8RpsTelData.EB)
                      {
                      SerialUSB.println("Switch pressed");
                      }
                      else
                      {
                      SerialUSB.println("ERROR in RPS Telegram Data (EB)");
                      }*/
                    /*
                      switch (lRpsTlg_p->u8RpsTelData.R1)
                      {
                      case RPS_BUTTON_CHANNEL_AO:
                        SerialUSB.println("Button AO");
                        break;
                      case RPS_BUTTON_CHANNEL_AI:
                        SerialUSB.println("Button AI");
                        break;
                      case RPS_BUTTON_CHANNEL_BO:
                        SerialUSB.println("Button BO");
                        break;
                      case RPS_BUTTON_CHANNEL_BI:
                        SerialUSB.println("Button BI");
                        break;
                      }*/
                    /*
                      if (RPS_BUTTON_2NDACT_VALID == lRpsTlg_p->u8RpsTelData.SA)
                      {
                        switch (lRpsTlg_p->u8RpsTelData.R2)
                        {
                          case RPS_BUTTON_CHANNEL_AO:
                            SerialUSB.println("2nd action: Button DO");
                            break;
                          case RPS_BUTTON_CHANNEL_AI:
                            SerialUSB.println("2nd action: Button DI");
                            break;
                          case RPS_BUTTON_CHANNEL_BO:
                            SerialUSB.println("2nd action: Button CO");
                            break;
                          case RPS_BUTTON_CHANNEL_BI:
                            SerialUSB.println("2nd action: Button CI");
                            break;
                        }
                      }*/
                  }
                  break;
                case u8RORG_VLD:
                  {
#ifdef KDEBUG
                    SerialUSB.println("Received VLD telegram.");
#endif
                    if (f_Pkt_st->u8DataBuffer[1] == VLD_CMD_ID_04)
                    {
                      uint8_t temp;
                      temp = f_Pkt_st->u8DataBuffer[2];
                      f_Pkt_st->u8DataBuffer[2] = f_Pkt_st->u8DataBuffer[3];
                      f_Pkt_st->u8DataBuffer[3] = temp;
                      VLD_D2_01_TELEGRAM_CMD_04_TYPE* ActStatResp = (VLD_D2_01_TELEGRAM_CMD_04_TYPE*) & (f_Pkt_st->u8DataBuffer[2]);


                      // Abfrage der 10 Aktoren
                      for (int n = 0; n < 10; n++)
                      {
                        if (ActStatResp->u8SenderId_p[0] == Aktor_array_0_9_ID[n][0])
                        {
                          if (ActStatResp->u8SenderId_p[1] == Aktor_array_0_9_ID[n][1])
                          {
                            if (ActStatResp->u8SenderId_p[2] == Aktor_array_0_9_ID[n][2])
                            {
                              if (ActStatResp->u8SenderId_p[3] == Aktor_array_0_9_ID[n][3])
                              {
#ifdef KDEBUG
                                SerialUSB.print("detect: Aktor_");
                                SerialUSB.println(n);
#endif
                                if (ActStatResp->u16VldTelActResp.IOChannel == 0) // Abfrage ob CH1
                                {
                                  if (ActStatResp->u16VldTelActResp.outputValue == 0) // Abfrage ob CH1 = OFF
                                  {
                                    Knx.write(70 + (n * 2), 0);
                                  }
                                  else if (ActStatResp->u16VldTelActResp.outputValue > 0)  // Abfrage ob CH1 = ON
                                  {
                                    Knx.write(70 + (n * 2), 1);
                                  }
                                }
                                else if (ActStatResp->u16VldTelActResp.IOChannel == 1) // Abfrage ob CH2
                                {
                                  if (ActStatResp->u16VldTelActResp.outputValue == 0) // Abfrage ob CH2 = OFF
                                  {
                                    Knx.write(71 + (n * 2), 0);
                                  }
                                  else if (ActStatResp->u16VldTelActResp.outputValue > 0)  // Abfrage ob CH2 = ON
                                  {
                                    Knx.write(71 + (n * 2), 1);
                                  }
                                }


                              } //ENDE ID3
                            } //ENDE ID2
                          } //ENDE ID1
                        } // ENDE ID0
                      } // ENDE FOR Schleife für 10 Taster Input

                      /*
                        SerialUSB.print("Channel ");
                        SerialUSB.println(ActStatResp->u16VldTelActResp.IOChannel, HEX);
                        SerialUSB.print("Output value: ");
                        SerialUSB.print(ActStatResp->u16VldTelActResp.outputValue, DEC);
                        SerialUSB.println("%.");
                      */
                      /*
                        SerialUSB.print("Sender ID: ");
                        for (int i = 0; i < 4; i++)
                        {
                        SerialUSB.print(ActStatResp->u8SenderId_p[i], HEX);
                        //SerialUSB.print(Aktor_array_0_9_ID[0][i], HEX);
                        }
                        SerialUSB.println("");
                      */
                    }
                    else
                    {
#ifdef KDEBUG
                      SerialUSB.println("Command ID not implemented yet.");
#endif
                    }
                  }
                  break;
                default:
                  {
#ifdef KDEBUG
                    SerialUSB.println("Unknown EnOcean equipment.");
#endif
                  }
              }
            }
            break;
          default:
            break;
        }


        // Schaltaktor
      }
      break;
      //      default:
      //        SerialUSB.print("Error receiving packet: ");
      //        SerialUSB.println(u8RetVal, HEX);
      //
      //        SerialUSB.print("State: ");
      //        SerialUSB.println(u8State, HEX);
      //
      //        SerialUSB.print("f_Pkt_st->u16DataLength: ");
      //        SerialUSB.println(f_Pkt_st.u16DataLength, HEX);

  }


}


//************************* get Status Actors *************************************
void getStatusActors(uint8_t* fui8_BaseID_p)
{
  PACKET_SERIAL_TYPE f_TestPacket_st;
  uint8_t f_TestBuf_p[15];
  f_TestPacket_st.u16DataLength = 0x0008;
  f_TestPacket_st.u8OptionLength = 0x07;
  f_TestPacket_st.u8Type = u8RADIO_ERP1;
  f_TestPacket_st.u8DataBuffer = &f_TestBuf_p[0];

  f_TestBuf_p[0] = u8RORG_VLD;
  f_TestBuf_p[1] = VLD_CMD_ID_03;
  f_TestBuf_p[2] = 0x1E;            //all outputs channels supported by the device

  for (int n = 0; n < 10; n++)
  {
    for (int i = 0; i < 4; i++)
    {
      f_TestBuf_p[i + 3] = fui8_BaseID_p[i];
    }
    f_TestBuf_p[7] = 0x00;   //Status
    f_TestBuf_p[8] = 0x00;   //TelSubnet


    for (int i = 0; i < 4; i++)
    {
      f_TestBuf_p[i + 9] = Aktor_array_0_9_ID[n][i];
    }
    f_TestBuf_p[13] = 0xFF;   //Status
    f_TestBuf_p[14] = 0x00;   //uncrypted


    uart_sendPacket(&f_TestPacket_st);
  }
}

//************************* get Status Actors *************************************
void send_RPS_Taster(uint8_t* fui8_BaseID_p, boolean state, boolean pressed)
{
  PACKET_SERIAL_TYPE l_TestPacket_st;
  uint8_t l_TestBuf_p[7];
  l_TestPacket_st.u16DataLength = 0x0007;
  l_TestPacket_st.u8OptionLength = 0x00;
  l_TestPacket_st.u8Type = u8RADIO_ERP1;
  l_TestPacket_st.u8DataBuffer = &l_TestBuf_p[0];

  l_TestBuf_p[0] = u8RORG_RPS;

  if (state == true)
  {
    if (pressed == true)
    {
      l_TestBuf_p[1] = 0x10;
      l_TestBuf_p[6] = 0x30;
    }
    else
    {
      l_TestBuf_p[1] = 0x00;
      l_TestBuf_p[6] = 0x20;
    }
  }
  else
  {
    if (pressed == true)
    {
      l_TestBuf_p[1] = 0x30;
      l_TestBuf_p[6] = 0x30;
    }
    else
    {
      l_TestBuf_p[1] = 0x00;
      l_TestBuf_p[6] = 0x20;
    }
  }

  for (int i = 0; i < 4; i++)
  {
    l_TestBuf_p[i + 2] = fui8_BaseID_p[i];
  }



  uart_sendPacket(&l_TestPacket_st);
}





// #################################################################################
// ### SETUP
// #################################################################################

void setup() {


  //****************** Init IO *******************************************************
  pinMode(LED_YELLOW, OUTPUT);
  digitalWrite(LED_YELLOW, HIGH);
  Wire.begin();


  //****************** Init Debug Interface ********************************************
#ifdef KDEBUG
  // Start debug serial with 9600 bauds
  DEBUGSERIAL.begin(115200);
#if defined(__AVR_ATmega32U4__) || defined(__SAMD21G18A__)
  // wait for serial port to connect. Needed for Leonardo/Micro/ProMicro/Zero only
  while (!DEBUGSERIAL)
#endif
    // make debug serial port known to debug class
    // Means: KONNEKTING will sue the same serial port for console debugging
    Debug.setPrintStream(&DEBUGSERIAL);
  Debug.print(F("KONNEKTING DemoSketch\n"));
#endif

  //****************** Init Debug KONNEKTING ********************************************
  Konnekting.setMemoryReadFunc(&readEeprom);
  Konnekting.setMemoryWriteFunc(&writeEeprom);
  Konnekting.setMemoryUpdateFunc(&updateEeprom);

  // Initialize KNX enabled Arduino Board

  Konnekting.init(KNX_SERIAL,
                  PROG_BUTTON_PIN,
                  PROG_LED_PIN,
                  MANUFACTURER_ID,
                  DEVICE_ID,
                  REVISION);


  //****************** Read Parameter ***************************************************
#ifdef KDEBUG
  Debug.println("** READ Parameter ******************");
#endif
  //static uint8_t abfrage_Interval_val_CH1 = ((uint8_t) Konnekting.getUINT8Param(0));  // abfrage_Interval

  for (int i = 0; i < 20; i++)
  {
    Taster_0_19_ID[i] = ((uint32_t) Konnekting.getUINT32Param(i));
    Taster_array_0_19_ID[i][0] = Taster_0_19_ID[i] >> 24;
    Taster_array_0_19_ID[i][1] = Taster_0_19_ID[i] >> 16;
    Taster_array_0_19_ID[i][2] = Taster_0_19_ID[i] >>  8;
    Taster_array_0_19_ID[i][3] = Taster_0_19_ID[i] ;

  }
  for (int i = 0; i < 10; i++)
  {
    Aktor_0_9_ID[i] = ((uint32_t) Konnekting.getUINT32Param((i * 2) + 20));
    Aktor_array_0_9_ID[i][0] = Aktor_0_9_ID[i] >> 24;
    Aktor_array_0_9_ID[i][1] = Aktor_0_9_ID[i] >> 16;
    Aktor_array_0_9_ID[i][2] = Aktor_0_9_ID[i] >> 8;
    Aktor_array_0_9_ID[i][3] = Aktor_0_9_ID[i] ;
  }
#ifdef KDEBUG
  Debug.println("ID0: 0x%08x", Taster_0_19_ID[0]);
#endif

  //****************** Init Timer *******************************************************
  setTimer();
  setTimer_ms(10);

  //****************** Init Enocean Gateway Interface************************************
  Serial.begin(57600);                       // Change to Serial wenn original Platine
  m_Pkt_st.u8DataBuffer = &u8datBuf[0];

  //****************** Read EnOcean Gateway Base ID  ************************************
  // communicates via Enocean UART channel

  
  getBaseID(&lui8_BaseID_p[0]);
  for (int i = 0; i < 10; i++)
  {
    lui8_SendeID_p[i][0] = lui8_BaseID_p[0];
    lui8_SendeID_p[i][1] = lui8_BaseID_p[1];
    lui8_SendeID_p[i][2] = lui8_BaseID_p[2];
    lui8_SendeID_p[i][3] = lui8_BaseID_p[3] + i;
  }

#ifdef KDEBUG
  Debug.print("BASEID: ");

  for (int i = 0; i < 4; i++)
  {
    SerialUSB.print(lui8_BaseID_p[i], HEX);
  }
  SerialUSB.println("");
#endif

  //****************** get Status Actors  ***********************************************
  //delay(2000);
  //  getStatusActors(&lui8_BaseID_p[0]);

  //  PACKET_SERIAL_TYPE l_TestPacket_st;
  //  uint8_t l_TestBuf_p[7];
  //  l_TestPacket_st.u16DataLength = 0x0007;
  //  l_TestPacket_st.u8OptionLength = 0x00;
  //  l_TestPacket_st.u8Type = u8RADIO_ERP1;
  //  l_TestPacket_st.u8DataBuffer = &l_TestBuf_p[0];
  //
  //  l_TestBuf_p[0] = u8RORG_RPS;
  //  l_TestBuf_p[1] = 0x70;
  //
  //    for(int i = 0; i < 4; i++)
  //  {
  //    l_TestBuf_p[i+2] = lui8_BaseID_p[i];
  //  }
  //
  //  l_TestBuf_p[6] = 0x30;
  //
  //  uart_sendPacket(&l_TestPacket_st);




  //  send_RPS_Taster(&lui8_BaseID_p[0], true, true);


#ifdef KDEBUG
  Debug.println("FINISH Setup");
#endif
  digitalWrite(LED_YELLOW, LOW);
}


// #################################################################################
// ### Loop()
// #################################################################################

void loop() {

  Knx.task();

  if (Konnekting.isReadyForApplication())
  {

    // EnOcean IN -> KNX OUT
    u8RetVal = NO_RX_TEL;
    u8RetVal = uart_getPacket(&m_Pkt_st, (uint16_t) DATBUF_SZ);
    getEnOceanMSG(u8RetVal, &m_Pkt_st);

    // KNX IN -> EnoCcean OUT
    for (int i = 0; i < 10; i++)
    {
      if (Taster_0_19_Comand[11 + i] == true)
      {
        //lui8_SendeID_p[3]= lui8_BaseID_p[3]+i;
        send_RPS_Taster(&lui8_SendeID_p[i][0], Taster_0_19_State[11 + i], true);
        Taster_0_19_time[11 + i] = millis();
        Taster_0_19_Comand[11 + i] = 2;
        break;
      }
    }
    for (int i = 0; i < 10; i++)
    {
      if (Taster_0_19_Comand[11 + i] == 2)
      {
        if ( millis() - Taster_0_19_time[11 + i] >= delayReleaseMSG)
        {
          send_RPS_Taster(&lui8_SendeID_p[i][0], Taster_0_19_State[11 + i], false);
          Taster_0_19_Comand[11 + i] = 0;
          break;
        }
      }
    }


  }//ENDE KONNEKTING APPLIKATION RUNING
}





void TC3_Handler()
{
  static int loop_count = 0;
  static int loop_count2 = 0;
  TcCount16* TC = (TcCount16*) TC3; // get timer struct
  /*if (TC->INTFLAG.bit.OVF == 1) {  // A overflow caused the interrupt
      TC->INTFLAG.bit.OVF = 1;    // writing a one clears the flag ovf flag
    }*/
  if (TC->INTFLAG.bit.MC0 == 1) {  // A compare to cc0 caused the interrupt
    TC->INTFLAG.bit.MC0 = 1;    // writing a one clears the flag ovf flag
  }
  //loop_count++;
  //loop_count2++;
  //if(loop_count==5){Task1 = true; loop_count = 0;}
  //if(loop_count2==abfrage_Interval_CH2 && OneWire_CH2_active == true){isDelay_CH2 = true;loop_count2 = 0;}

}



