#include <ESP8266WiFi.h>
#include <FirebaseArduino.h>

#define SERIAL_PRINT  1

#define PRINT(...)    if (SERIAL_PRINT) {Serial.print(__VA_ARGS__);}
#define PRINTLN(...)  if (SERIAL_PRINT) {Serial.println(__VA_ARGS__);}
#define PRINTF(...)   if (SERIAL_PRINT) {Serial.printf(__VA_ARGS__);}


#define LED_1 4 
#define LED_2 12 
#define LED_3 14 

#define BUTTON_1 13
#define BUTTON_2 4
#define BUTTON_3 5

#define LED_ON LOW 
#define LED_OFF HIGH 

#define CFG_CMD_NEW_STATE             1
#define CFG_CMD_GET_LENGTH            2
#define CFG_CMD_GET_OPCODE            3
#define CFG_CMD_GET_DATA              4
#define CFG_CMD_CRC_CHECK             5
#define CFG_CMD_WAITING_SATRT_CODE    6

#define UART_PACKET_LEN_MAX           50

byte parserPacketstate = CFG_CMD_WAITING_SATRT_CODE;
byte parserPacketcrc = 0;
byte parserPacketlen = 0;
byte parserPacketcnt = 0;
byte parserPacketopcode;
byte uart_data_parser[UART_PACKET_LEN_MAX];

#define FIREBASE_CHECKTIME          1000
#define DEVICE_NUM_MAX              20
#define MAC_ADDR_LEN                12

#define FIREBASE_HOST               "luanvan2018-4a262.firebaseio.com"
#define FIREBASE_AUTH               "w6SGS2UcoAs7a7gltHDzhFxZUfdD3GnMPINcHDcv"

#define WIFI_SSID                   "IKY.HOME"
#define WIFI_PASSWORD               "iky12345"

typedef struct
 {
     char mac[MAC_ADDR_LEN];
     unsigned char status;
     bool enable;
 }  device_type;

char mac_null[MAC_ADDR_LEN];

device_type device_list_firebase[DEVICE_NUM_MAX];
device_type device_list_local[DEVICE_NUM_MAX];

int device_num = 0;
unsigned char uart_data_send[UART_PACKET_LEN_MAX];
char User_UID[40];

bool first_loop = true;

void setupFirebase() 
{
  int cnt;
  sprintf(User_UID,"rZYioIc50zM8sj3qUdhrj5Ec0Oc2/status"); //for test
  memset(mac_null,'0',sizeof(mac_null));
  for(cnt = 0;cnt < DEVICE_NUM_MAX;cnt++)
  {
    memcpy(device_list_local[cnt].mac,mac_null,MAC_ADDR_LEN);
    device_list_local[cnt].enable = false;
  }
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
}

void setupWifi() {

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {

    PRINTLN("connecting...");
    delay(500);
  }
  
  PRINTLN("\r\nI 'm connected and my IP address: ");
  PRINTLN(WiFi.localIP());
}


void setup() 
{
  Serial.begin(115200);

  setupWifi();
  setupFirebase();

  setupPinsMode();
}

void getData() 
{
  int index = -1;
  bool isUpdate = false;
  int cnt = 0;
  int cnt1;
  int cnt2;  
  const char * mac;
  FirebaseObject object = Firebase.get(User_UID);

  if(Firebase.error())
  {
    PRINTLN("Firebase error\r\n");
    return;
  }
  
  JsonObject& obj = object.getJsonVariant();
  device_num = obj.size();
  
  //printObjectKeys(obj);
  //return;
  PRINTLN("device_num");
  PRINTLN(device_num);

  if(device_num == 0)
  {
    return;
  }
  
  for (JsonObject::iterator it=obj.begin(); it!=obj.end(); ++it)
  {       
       memcpy(device_list_firebase[cnt].mac,it->key,MAC_ADDR_LEN);       
      if (it->value.is<JsonObject&>())
      {
        device_list_firebase[cnt].status = it->value["status"];
      }
      else
      {
        device_list_firebase[cnt].status = it->value;
      }
      //PRINTLN(device_list_firebase[cnt].mac);
      //PRINTLN(device_list_firebase[cnt].status);
      cnt ++;
  }
  
  /*
   * Kiểm tra trạng thái 
   *  - Kiểm tra MAC đã có trong bảng routing 
   *    + nếu chưa có thì thêm mới và ko update trạng thái (nếu thêm thất bại thì bỏ qua)
   *    + nếu đã có rồi thì kiểm tra trạng thái -> trạng thái thái đổi -> update
   */  
  if(first_loop) 
  {
    first_loop = false;
    memcpy(device_list_local,device_list_firebase,sizeof(device_list_local));
  }
  else
  {
    for(cnt = 0; cnt < device_num; cnt ++)
    {
      isUpdate = false;
      index = routing_check_mac(device_list_firebase[cnt].mac);
      if(index > -1)
      {
        if(device_list_local[index].status != device_list_firebase[cnt].status)
        {
          device_list_local[index].status = device_list_firebase[cnt].status;
          isUpdate = true;         
        }
        device_list_local[index].enable = true;
      }
      else
      {
        index = routing_add_mac(device_list_firebase[cnt].mac);
        if(index > -1) 
        {
          device_list_local[index].status = device_list_firebase[cnt].status;
          device_list_local[index].enable = true;
        }
      }

      if(isUpdate)
      {
        digitalWrite(LED_1, HIGH);
        //PRINTLN("\r\nUPDATE\r\n")
        //$CA$09$00$04$be$98$09$f4$df$f3$0a$01$01$31
        Serial.write(0xCA);
        Serial.write(MAC_ADDR_LEN + 1); //MAC_ADDR_LEN + len(status)
        Serial.write(0); //Len
        Serial.write(4); //Opcode
        //
        memcpy(uart_data_send,device_list_firebase[cnt].mac,MAC_ADDR_LEN);
        uart_data_send[MAC_ADDR_LEN] = device_list_firebase[cnt].status;
        uart_data_send[MAC_ADDR_LEN + 1] = CfgCalcCheckSum(uart_data_send,MAC_ADDR_LEN + 1);
        //
        Serial.write(uart_data_send,MAC_ADDR_LEN + 1 + 1);
      }
      else
      {
        digitalWrite(LED_1, LOW);
      }
    }

    // Xóa MAC không có trên firebase dựa vào field enable
    for(cnt=0;cnt<DEVICE_NUM_MAX;cnt++)
    {
      if(device_list_local[cnt].enable == false)
      {
        memcpy(device_list_local[cnt].mac,mac_null,MAC_ADDR_LEN);
      }
    }
    // Khởi tạo lại giá trị của field enable cho lần kiểm tra tiếp theo
    for(cnt=0;cnt<DEVICE_NUM_MAX;cnt++)
    {
      device_list_local[cnt].enable = false;
    }
  }
}
void loop() 
{
  static long firebase_checktime;  
  uint8_t c;

  if (WiFi.status() != WL_CONNECTED)
  {
      setupWifi();
  }
  while (Serial.available() > 0) 
  {
    c = Serial.read();
    UartParserPacket(c);    
  }  
  if (millis() < firebase_checktime) 
  {
    // we wrapped around, lets just try again
    firebase_checktime = millis();
  }
  if ((firebase_checktime + FIREBASE_CHECKTIME) < millis()) 
  {
    PRINTLN("\r\nCHECK DATA\r\n")
    getData();
    //enough time has passed to debounce
    firebase_checktime = millis();    
  }
  if (Firebase.failed()) {
      return;
  }
}

void setupPinsMode() 
{
  pinMode(LED_1, OUTPUT);
}


byte UartParserPacket(byte c)
{
  int index = 0;  
  bool isUpdate = false;
  switch(parserPacketstate)
  {
    case CFG_CMD_WAITING_SATRT_CODE:
      if(c == 0xCA)
      {
        
        parserPacketstate = CFG_CMD_GET_LENGTH;
        parserPacketlen = 0;
        parserPacketcrc = 0;
        parserPacketcnt = 0;
      }
      break;
    case CFG_CMD_GET_LENGTH:
      parserPacketlen |= (uint16_t)c<<parserPacketcnt;
      parserPacketcnt += 8;
      if(parserPacketcnt >= 16)
      {
        parserPacketstate = CFG_CMD_GET_OPCODE;
        parserPacketcnt = 0;
      }
      break;
    case CFG_CMD_GET_OPCODE:    
      parserPacketopcode = c;
      parserPacketstate = CFG_CMD_GET_DATA;
      break;
    case CFG_CMD_GET_DATA:
      if((parserPacketcnt >= parserPacketlen) || (parserPacketlen > UART_PACKET_LEN_MAX))
      {
        parserPacketstate = CFG_CMD_WAITING_SATRT_CODE;
      }
      else
      {
        parserPacketcrc += c;
        uart_data_parser[parserPacketcnt]= c;
        parserPacketcnt++;
        if(parserPacketcnt == parserPacketlen)
        {
          parserPacketstate = CFG_CMD_CRC_CHECK;
        }
      }
      break;
    case CFG_CMD_CRC_CHECK:
      parserPacketstate= CFG_CMD_WAITING_SATRT_CODE;
      if(parserPacketcrc  == c)
      { 
        //process here
        //PRINTF("parserPacketopcode %d\r\n", parserPacketopcode);
        switch(parserPacketopcode)
        {
          case 5:
            char mac_addr[MAC_ADDR_LEN + 1];
            memset(mac_addr,0,sizeof(mac_addr));
            memcpy(mac_addr,uart_data_parser,MAC_ADDR_LEN);
            char update_path[100]; 
            index = routing_check_mac(mac_addr);

            //PRINTLN(mac_addr);
            //PRINTLN(uart_data_parser[MAC_ADDR_LEN + 2]); // giá trị
            
            if(index > -1)
            {               
              if(device_list_local[index].status != uart_data_parser[MAC_ADDR_LEN + 2]) //cập nhật trạng thái mới    
              {
                device_list_local[index].status = uart_data_parser[MAC_ADDR_LEN + 2];
                isUpdate = true;
              
                sprintf(update_path,"%s/%s",User_UID,mac_addr);   
                //PRINTLN();
                //PRINTLN(update_path);
                //PRINTLN();
                Firebase.setInt((const char*)update_path, device_list_local[index].status);
                if(Firebase.error())
                {
                  PRINTLN("Firebase error\r\n");                  
                }  
              }
            }
            else
            {
              
            }
            break;

          case 7:
            
            break;
            
          case 8: //wifi
            break;

          case 9: //path
            break;
            
          default:      
            break;
        }
        return 0;
      }  
      //else
      //{
        //PRINTF("parserPacketcrc %x\r\n", parserPacketcrc); 
      //}
      break;
    default:
      parserPacketstate = CFG_CMD_WAITING_SATRT_CODE;
      break;        
  }
  return 0xff;
}

uint8_t CfgCalcCheckSum(uint8_t *buff, uint16_t length)
{
  uint32_t i;
  uint8_t crc = 0;
  for(i = 0;i < length; i++)
  {
    crc += buff[i];
  }
  return crc;
}

void printObjectKeys(JsonObject& obj)
{
  for (JsonObject::iterator it=obj.begin(); it!=obj.end(); ++it)
  {
    PRINTLN(it->key);
    if (it->value.is<JsonObject&>())
    {
      printObjectKeys(it->value);
    }
    else
    {
      PRINTLN(it->value.asString());
    }
  }
}

/*
 * Kiểm tra MAC trong routing
 * char device_mac_addr_routing[DEVICE_NUM_MAX][MAC_ADDR_LEN];
 * char device_mac_addr[DEVICE_NUM_MAX][MAC_ADDR_LEN];
 */
 int routing_check_mac(char *mac)
 {
    int i;
    for(i=0;i<DEVICE_NUM_MAX;i++)
    {
      if(memcmp(device_list_local[i].mac,mac,MAC_ADDR_LEN) == 0) return i;      //trả về vị trí trong danh sách
    }
    return -1;
 }

 int routing_add_mac(char *mac)
 {
    int i;
    for(i=0;i<DEVICE_NUM_MAX;i++)
    {
      if(memcmp(device_list_local[i].mac,mac_null,MAC_ADDR_LEN) == 0) break;
    }
    if( i< DEVICE_NUM_MAX)
    {
      memcpy(device_list_local[i].mac,mac,MAC_ADDR_LEN);
      return i; //trả về vị trí được thêm
    }
    return -1;
 }

 int routing_count_mac(void)
 {
    int i;
    int cnt = 0;
    cnt = 0;
    for(i=0;i<DEVICE_NUM_MAX;i++)
    {
      if(memcmp(device_list_local[i].mac,mac_null,MAC_ADDR_LEN) != 0) cnt++;
    }
    return cnt;
 }






 
