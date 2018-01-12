int incomingByte = 0;   // for incoming serial data
boolean textend = false;
int array[25];

//#define KDEBUG

#define DATBUF_SZ 100

//! uart_getPacket state machine states.
typedef enum
{
  //! Waiting for the synchronisation byte 0x55
  GET_SYNC_STATE = 0,
  //! Copying the 4 after sync byte: raw data length (2 bytes), optional data length (1), type (1).
  GET_HEADER_STATE,
  //! Checking the header CRC8 checksum. Resynchronisation test is also done here
  CHECK_CRC8H_STATE,
  //! Copying the data and optional data bytes to the paquet buffer
  GET_DATA_STATE,
  //! Checking the info CRC8 checksum.
  CHECK_CRC8D_STATE,
} STATES_GET_PACKET;

//! State machine counter
STATES_GET_PACKET u8State = GET_SYNC_STATE;

uint8_t u8CRC = 0x00;
uint8_t u8RxByte = 0x00;
uint8_t u8RetVal = 0x00;

//! Packet structure (ESP3)
typedef struct
{
  // Amount of raw data bytes to be received. The most significant byte is sent/received first
  uint16_t u16DataLength;
  // Amount of optional data bytes to be received
  uint8_t u8OptionLength;
  // Packe type code
  uint8_t u8Type;
  // Data buffer: raw data + optional bytes
  uint8_t *u8DataBuffer;
} PACKET_SERIAL_TYPE;

typedef struct
{
  uint8_t SA : 1; // 0: No 2nd action; 1: Valid second action
  uint8_t R2 : 3; // Rocker second action.
  uint8_t EB : 1; // 0: Released; 1: Pressed
  uint8_t R1 : 3; // Rocker first action. See
} RPS_TEL_DATA_TYPE;

typedef struct
{
  uint8_t type  : 3; // NOT USED
  uint8_t dummy : 5; //
} VLD_TEL_MEASUREMENT_TYPE;

/*
 * 	Packet type
 */

#define u8RADIO_ERP1 0x01
#define u8RESPONSE   0x02
 
/*
 * 	EEP type
 */

// Receive

#define u8RORG_RPS 0xF6
#define u8RORG_VLD 0xD2


// Send
#define u8RORG_COMMON_COMMAND 0x05
// COMANDS

#define u8CO_RD_IDBASE 0x08

// enum RPS_BUTTON
// {
// ChannelA1 = 0,
// ChannelA0 = 1,
// ChannelB1 = 2,
// ChannelB0 = 3
// };

// RPS_BUTTON RpsBtn;

#define RPS_BUTTON_CHANNEL_AI 0
#define RPS_BUTTON_CHANNEL_AO 1
#define RPS_BUTTON_CHANNEL_BI 2
#define RPS_BUTTON_CHANNEL_BO 3

#define RPS_BUTTON_2NDACT_NO    0
#define RPS_BUTTON_2NDACT_VALID 1

#define VLD_CMD_ID_01 0x01
#define VLD_CMD_ID_02 0x02
#define VLD_CMD_ID_03 0x03
#define VLD_CMD_ID_04 0x04
#define VLD_CMD_ID_05 0x05
#define VLD_CMD_ID_06 0x06

typedef struct
{
  uint16_t outputValue    : 7; // in %
  uint16_t LC         : 1; //
  uint16_t IOChannel    : 5; //
  uint16_t EL         : 2; //
  uint16_t OC         : 1; // NOT USED
} VLD_D2_01_TELEGRAM_CMD_04_ACTRESP_TYPE;

typedef struct
{
  VLD_D2_01_TELEGRAM_CMD_04_ACTRESP_TYPE u16VldTelActResp;
  uint8_t  u8SenderId_p[4];
  uint8_t  u8Status;
} VLD_D2_01_TELEGRAM_CMD_04_TYPE;

typedef struct
{
  RPS_TEL_DATA_TYPE  u8RpsTelData;
  uint8_t  u8SenderId_p[4];
  uint8_t  u8Status;
} RPS_TELEGRAM_TYPE;



uint8_t u8CRC8Table[256] = {
  0x00, 0x07, 0x0e, 0x09, 0x1c, 0x1b, 0x12, 0x15,
  0x38, 0x3f, 0x36, 0x31, 0x24, 0x23, 0x2a, 0x2d,
  0x70, 0x77, 0x7e, 0x79, 0x6c, 0x6b, 0x62, 0x65,
  0x48, 0x4f, 0x46, 0x41, 0x54, 0x53, 0x5a, 0x5d,
  0xe0, 0xe7, 0xee, 0xe9, 0xfc, 0xfb, 0xf2, 0xf5,
  0xd8, 0xdf, 0xd6, 0xd1, 0xc4, 0xc3, 0xca, 0xcd,
  0x90, 0x97, 0x9e, 0x99, 0x8c, 0x8b, 0x82, 0x85,
  0xa8, 0xaf, 0xa6, 0xa1, 0xb4, 0xb3, 0xba, 0xbd,
  0xc7, 0xc0, 0xc9, 0xce, 0xdb, 0xdc, 0xd5, 0xd2,
  0xff, 0xf8, 0xf1, 0xf6, 0xe3, 0xe4, 0xed, 0xea,
  0xb7, 0xb0, 0xb9, 0xbe, 0xab, 0xac, 0xa5, 0xa2,
  0x8f, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9d, 0x9a,
  0x27, 0x20, 0x29, 0x2e, 0x3b, 0x3c, 0x35, 0x32,
  0x1f, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0d, 0x0a,
  0x57, 0x50, 0x59, 0x5e, 0x4b, 0x4c, 0x45, 0x42,
  0x6f, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7d, 0x7a,
  0x89, 0x8e, 0x87, 0x80, 0x95, 0x92, 0x9b, 0x9c,
  0xb1, 0xb6, 0xbf, 0xb8, 0xad, 0xaa, 0xa3, 0xa4,
  0xf9, 0xfe, 0xf7, 0xf0, 0xe5, 0xe2, 0xeb, 0xec,
  0xc1, 0xc6, 0xcf, 0xc8, 0xdd, 0xda, 0xd3, 0xd4,
  0x69, 0x6e, 0x67, 0x60, 0x75, 0x72, 0x7b, 0x7c,
  0x51, 0x56, 0x5f, 0x58, 0x4d, 0x4a, 0x43, 0x44,
  0x19, 0x1e, 0x17, 0x10, 0x05, 0x02, 0x0b, 0x0c,
  0x21, 0x26, 0x2f, 0x28, 0x3d, 0x3a, 0x33, 0x34,
  0x4e, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5c, 0x5b,
  0x76, 0x71, 0x78, 0x7f, 0x6A, 0x6d, 0x64, 0x63,
  0x3e, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2c, 0x2b,
  0x06, 0x01, 0x08, 0x0f, 0x1a, 0x1d, 0x14, 0x13,
  0xae, 0xa9, 0xa0, 0xa7, 0xb2, 0xb5, 0xbc, 0xbb,
  0x96, 0x91, 0x98, 0x9f, 0x8a, 0x8D, 0x84, 0x83,
  0xde, 0xd9, 0xd0, 0xd7, 0xc2, 0xc5, 0xcc, 0xcb,
  0xe6, 0xe1, 0xe8, 0xef, 0xfa, 0xfd, 0xf4, 0xf3
};

/*
   Example usage of crc8
*/
/*
   for (i = 0 ; i < u16DataSize ; i++)
   u8CRC = proc_crc8(u8CRC, u8Data[i]);
   printf("CRC8 = %02X\n", u8CRC);
*/
#define proc_crc8(u8CRC, u8Data) (u8CRC8Table[u8CRC ^ u8Data])
#define ErrMsg(x) printf("ERROR:", %x)

#define HeaderSz 4;

#define SER_SYNCH_CODE 0x55
#define SER_HEADER_NR_BYTES 4

#define RETURN_TYPE uint8_t

#define OK                 0
#define OUT_OF_RANGE      21
#define NOT_VALID_CHKSUM   7
#define NO_RX_TEL          6
#define NEW_RX_BYTE        3

RETURN_TYPE uart_sendPacket(PACKET_SERIAL_TYPE *pPacket)
{
  uint16_t i;
  uint8_t u8CRC;
  uint8_t *u8Raw = (uint8_t*)pPacket;
  // When both length fields are 0, then this telegram is not allowed.
  if ((pPacket->u16DataLength || pPacket->u8OptionLength) == 0)
  {
    return OUT_OF_RANGE;
  }
  
  uint16_t lui16_PacketLength = pPacket->u16DataLength;
  uint8_t temp;
  temp = u8Raw[0];
  u8Raw[0] = u8Raw[1];
  u8Raw[1] = temp;
  
#ifdef KDEBUG
  SerialUSB.println("Debug 0");
#endif
  // Sync
  while (Serial.write(0x55) != 1);
#ifdef KDEBUG
  SerialUSB.println("Debug 1");
#endif
  // Header
  Serial.write((uint8_t*)pPacket, 4);
#ifdef KDEBUG
  SerialUSB.println("Debug 2");
#endif
  // Header CRC
  u8CRC = 0;
  u8CRC = proc_crc8(u8CRC, ((uint8_t*)pPacket)[0]);
  u8CRC = proc_crc8(u8CRC, ((uint8_t*)pPacket)[1]);
  u8CRC = proc_crc8(u8CRC, ((uint8_t*)pPacket)[2]);
  u8CRC = proc_crc8(u8CRC, ((uint8_t*)pPacket)[3]);
  while (Serial.write(u8CRC) != 1);
#ifdef KDEBUG
  SerialUSB.println("Debug 3");
#endif
  // Data
  u8CRC = 0;
  for (i = 0 ; i < lui16_PacketLength + pPacket->u8OptionLength ; i++)
  {
    u8CRC = proc_crc8(u8CRC, pPacket->u8DataBuffer[i]);
    while (Serial.write(pPacket->u8DataBuffer[i]) != 1);
  }
  
#ifdef KDEBUG
  SerialUSB.println("Debug 4");
#endif
  // Data CRC
  while (Serial.write(u8CRC) != 1);
  
#ifdef KDEBUG
  SerialUSB.println("Debug 5");
#endif
  return OK;
}

RETURN_TYPE uart_getPacket(PACKET_SERIAL_TYPE *pPacket, uint16_t u16BufferLength)
{

  //! UART received byte code
  uint8_t u8RxByte;
  //! Checksum calculation
  static uint8_t u8CRC = 0;
  //! Nr. of bytes received
  static uint16_t u16Count = 0;
  //! State machine counter
  //static STATES_GET_PACKET u8State = GET_SYNC_STATE;
  //! Timeout measurement
  static uint8_t u8TickCount = 0;
  // Byte buffer pointing at the paquet address
  uint8_t *u8Raw = (uint8_t*)pPacket;
  // Temporal variable
  uint8_t i;
  // Check for timeout between two bytes
  // TODO
  //if (((uint8)ug32SystemTimer) - u8TickCount > SER_INTERBYTE_TIME_OUT)
  //{
  // Reset state machine to init state
  //u8State = GET_SYNC_STATE;
  //}
  // State machine goes on when a new byte is received

  if (Serial.available() > 0) {

    while (Serial.readBytes(&u8RxByte, 1) == 1)
    {
      // Comment out for debugging
      //SerialUSB.println(u8RxByte, HEX);

      // Tick count of last received byte
      // TODO
      //u8TickCount = (uint8)ug32SystemTimer;
      // State machine to load incoming packet bytes
      switch (u8State)
      {
        // Waiting for packet sync byte 0x55
        case GET_SYNC_STATE:
          if (u8RxByte == SER_SYNCH_CODE)
          {
            u8State = GET_HEADER_STATE;
            u16Count = 0;
            u8CRC = 0;
          }
          break;
        // Read the header bytes
        case GET_HEADER_STATE:
          // Copy received data to buffer
          u8Raw[u16Count++] = u8RxByte;
          u8CRC = proc_crc8(u8CRC, u8RxByte);
          // All header bytes received?
          if (u16Count == SER_HEADER_NR_BYTES)
          {
            // SerialUSB.print("Received all header bytes.\n");
            // SerialUSB.print("pPacket->u16DataLength: ");
            // SerialUSB.println(pPacket->u16DataLength, HEX);
            // SerialUSB.print("u8Raw[1]");
            // SerialUSB.println(u8Raw[1], HEX);
            // SerialUSB.print("u8Raw[2]");
            // SerialUSB.println(u8Raw[2], HEX);
            uint8_t temp;
            temp = u8Raw[0];
            u8Raw[0] = u8Raw[1];
            u8Raw[1] = temp;
            // SerialUSB.print("pPacket->u16DataLength: ");
            // SerialUSB.println(pPacket->u16DataLength, HEX);
            // SerialUSB.print("u8Raw[1]");
            // SerialUSB.println(u8Raw[1], HEX);
            // SerialUSB.print("u8Raw[2]");
            // SerialUSB.println(u8Raw[2], HEX);
            u8State = CHECK_CRC8H_STATE;
          }
          break;
        // Check header checksum & try to resynchonise if error happened
        case CHECK_CRC8H_STATE:
          // Header CRC correct?
          if (u8CRC != u8RxByte)
          {
#ifdef KDEUGB
            SerialUSB.print("CRC check failed.");
#endif
            // No. Check if there is a sync byte (0x55) in the header
            int a = -1;
            for (i = 0 ; i < SER_HEADER_NR_BYTES ; i++)
              if (u8Raw[i] == SER_SYNCH_CODE)
              {
                // indicates the next position to the sync byte found
                a = i + 1;
                break;
              };
            if ((a == -1) && (u8RxByte != SER_SYNCH_CODE))
            {
              // Header and CRC8H does not contain the sync code
              u8State = GET_SYNC_STATE;
              break;
            }
            else if ((a == -1) && (u8RxByte == SER_SYNCH_CODE))
            {
              // Header does not have sync code but CRC8H does.
              // The sync code could be the beginning of a packet
              u8State = GET_HEADER_STATE;
              u16Count = 0;
              u8CRC = 0;
              break;
            }
            // Header has a sync byte. It could be a new telegram.
            // Shift all bytes from the 0x55 code in the buffer.
            // Recalculate CRC8 for those bytes
            u8CRC = 0;
            for (i = 0 ; i < (SER_HEADER_NR_BYTES - a) ; i++)
            {
              u8Raw[i] = u8Raw[a + i];
              u8CRC = proc_crc8(u8CRC, u8Raw[i]);
            }
            u16Count = SER_HEADER_NR_BYTES - a;
            // u16Count = i; // Seems also valid and more intuitive than u16Count -= a;
            // Copy the just received byte to buffer
            u8Raw[u16Count++] = u8RxByte;
            u8CRC = proc_crc8(u8CRC, u8RxByte);
            if (u16Count < SER_HEADER_NR_BYTES)
            {
              u8State = GET_HEADER_STATE;
              break;
            }
            break;
          }
          // CRC8H correct. Length fields values valid?
          if ((pPacket->u16DataLength + pPacket->u8OptionLength) == 0)
          {
            //No. Sync byte received?
            if ((u8RxByte == SER_SYNCH_CODE))
            {
              //yes
              u8State = GET_HEADER_STATE;
              u16Count = 0;
              u8CRC = 0;
              break;
            }
            // Packet with correct CRC8H but wrong length fields.
            u8State = GET_SYNC_STATE;
            return OUT_OF_RANGE;
          }
          // Correct header CRC8. Go to the reception of data.
          u8State = GET_DATA_STATE;
          u16Count = 0;
          u8CRC = 0;
          break;
        // Copy the information bytes
        case GET_DATA_STATE:
          // Copy byte in the packet buffer only if the received bytes have enough room
          if (u16Count < u16BufferLength)
          {
            pPacket->u8DataBuffer[u16Count] = u8RxByte;
            u8CRC = proc_crc8(u8CRC, u8RxByte);
          }
          // When all expected bytes received, go to calculate data checksum
          if ( ++u16Count == (pPacket->u16DataLength + pPacket->u8OptionLength) )
          {
            u8State = CHECK_CRC8D_STATE;
          }

          //SerialUSB.print(u16Count);
          //SerialUSB.println(u16Count, DEC);

          break;
        // Check the data CRC8
        case CHECK_CRC8D_STATE:
          // In all cases the state returns to the first state: waiting for next sync byte
          u8State = GET_SYNC_STATE;
          // Received packet bigger than space to allocate bytes?
          if (u16Count > u16BufferLength) return OUT_OF_RANGE;
          // Enough space to allocate packet. Equals last byte the calculated CRC8?
          if (u8CRC == u8RxByte) return OK; // Correct packet received
          // False CRC8.
          // If the received byte equals sync code, then it could be sync byte for next paquet.
          if ((u8RxByte == SER_SYNCH_CODE))
          {
            u8State = GET_HEADER_STATE;
            u16Count = 0;
            u8CRC = 0;
          }
          return NOT_VALID_CHKSUM;
        default:
          // Yes. Go to the reception of info.
          u8State = GET_SYNC_STATE;
          break;
      }
    }
  }
  return (u8State == GET_SYNC_STATE) ? NO_RX_TEL : NEW_RX_BYTE;
}
