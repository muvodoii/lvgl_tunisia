#include<Arduino.h>
#include <lvgl.h>
#include <LovyanGFX.hpp>
// #include <Adafruit_Sensor.h>
#include "FT6236.h"
#include "ui.h"
// #include "DHT.h"

#include <EEPROM.h>
#include <WiFi.h>
#include <WiFiUdp.h>
const int i2c_touch_addr = TOUCH_I2C_ADD;
#define EEPROM_SIZE 512  // Kích thước EEPROM cần sử dụng
#define TEXT_ADDRESS 0   // Địa chỉ bắt đầu lưu text trong EEPROM

#define LCD_BL 46

#define DHTPIN 40     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
static char current_password[10] = "";
static uint8_t password_len = 0;
// Initialize DHT20 sensor

#define SDA_FT6236 38
#define SCL_FT6236 39
//FT6236 ts = FT6236();

const char* ssid = "Note";
const char* password = "123456789";

// Thông số UDP
const char* UDP_ADDRESS = "255.255.255.255"; // Địa chỉ broadcast hoặc địa chỉ cụ thể
const int UDP_PORT = 4210;

// Khởi tạo đối tượng UDP
WiFiUDP udp;
class LGFX : public lgfx::LGFX_Device
{
    lgfx::Panel_ILI9488 _panel_instance;
    lgfx::Bus_Parallel16 _bus_instance;
  public:
    LGFX(void)
    {
      {
        auto cfg = _bus_instance.config();
        cfg.port = 0;
        cfg.freq_write = 80000000;
        cfg.pin_wr = 18;
        cfg.pin_rd = 48;
        cfg.pin_rs = 45;

        cfg.pin_d0 = 47;
        cfg.pin_d1 = 21;
        cfg.pin_d2 = 14;
        cfg.pin_d3 = 13;
        cfg.pin_d4 = 12;
        cfg.pin_d5 = 11;
        cfg.pin_d6 = 10;
        cfg.pin_d7 = 9;
        cfg.pin_d8 = 3;
        cfg.pin_d9 = 8;
        cfg.pin_d10 = 16;
        cfg.pin_d11 = 15;
        cfg.pin_d12 = 7;
        cfg.pin_d13 = 6;
        cfg.pin_d14 = 5;
        cfg.pin_d15 = 4;
        _bus_instance.config(cfg);
        _panel_instance.setBus(&_bus_instance);
      }

      {
        auto cfg = _panel_instance.config();

        cfg.pin_cs = -1;
        cfg.pin_rst = -1;
        cfg.pin_busy = -1;
        cfg.memory_width = 320;
        cfg.memory_height = 480;
        cfg.panel_width = 320;
        cfg.panel_height = 480;
        cfg.offset_x = 0;
        cfg.offset_y = 0;
        cfg.offset_rotation = 2;
        cfg.dummy_read_pixel = 8;
        cfg.dummy_read_bits = 1;
        cfg.readable = true;
        cfg.invert = false;
        cfg.rgb_order = false;
        cfg.dlen_16bit = true;
        cfg.bus_shared = true;
        _panel_instance.config(cfg);
      }
      setPanel(&_panel_instance);
    }
};

LGFX tft;
/*Change to your screen resolution*/
static const uint16_t screenWidth  = 480;
static const uint16_t screenHeight = 320;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[ screenWidth * screenHeight / 5 ];
int led;

const char *correct_password = "1234";
/* Display flushing */
void my_disp_flush( lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p )
{
  uint32_t w = ( area->x2 - area->x1 + 1 );
  uint32_t h = ( area->y2 - area->y1 + 1 );

  tft.startWrite();
  tft.setAddrWindow( area->x1, area->y1, w, h );
  tft.writePixels((lgfx::rgb565_t *)&color_p->full, w * h);
  tft.endWrite();

  lv_disp_flush_ready( disp );
}

void my_touchpad_read( lv_indev_drv_t * indev_driver, lv_indev_data_t * data )
{
  int pos[2] = {0, 0};

  ft6236_pos(pos);
  if (pos[0] > 0 && pos[1] > 0)
  {
    data->state = LV_INDEV_STATE_PR;
   data->point.x = tft.width()-pos[1];
   data->point.y = pos[0];
    // data->point.y = pos[1];
    // data->point.x = pos[0];
    Serial.printf("x-%d,y-%d\n", data->point.x, data->point.y);
  }
  else {
    data->state = LV_INDEV_STATE_REL;
  }
}
void add_number_to_password(char num) {
    if(password_len < 9) {
        current_password[password_len++] = num;
        current_password[password_len] = '\0';
        lv_textarea_set_text(ui_TextAreaS1, current_password);
    }
}

void touch_init()
{
  // I2C init
  Wire.begin(SDA_FT6236, SCL_FT6236);
  byte error, address;

  Wire.beginTransmission(i2c_touch_addr);
  error = Wire.endTransmission();

  if (error == 0)
  {
    Serial.print("I2C device found at address 0x");
    Serial.print(i2c_touch_addr, HEX);
    Serial.println("  !");
  }
  else if (error == 4)
  {
    Serial.print("Unknown error at address 0x");
    Serial.println(i2c_touch_addr, HEX);
  }
}

void setupUDP() {
    udp.begin(UDP_PORT);
    Serial.println("UDP initialized");
}
void setup()
{
  Serial.begin( 9600 ); /* prepare for possible serial debug */
  
WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  //IO口引脚

setupUDP();
  tft.begin();          /* TFT init */
  tft.setRotation( 1); /* Landscape orientation, flipped */
EEPROM.begin(EEPROM_SIZE); // Khởi tạo EEPROM


  pinMode(LCD_BL, OUTPUT);
  digitalWrite(LCD_BL, HIGH);

  touch_init();

  lv_init();
  lv_disp_draw_buf_init( &draw_buf, buf, NULL, screenWidth * screenHeight / 5 );

  /*Initialize the display*/
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init( &disp_drv );
  /*Change the following line to your display resolution*/
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register( &disp_drv );

  /*Initialize the (dummy) input device driver*/
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init( &indev_drv );
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register( &indev_drv );

//  lv_obj_add_event_cb(ui_Button1, btn1_event_cb,LV_EVENT_ALL,NULL);

  ui_init();

  
}
void ui_event_Button8(lv_event_t * e) {
//     lv_event_code_t event_code = lv_event_get_code(e);
//     lv_obj_t * target = lv_event_get_target(e);
    
//     if(event_code == LV_EVENT_RELEASED) {
//         //const char* input_text = lv_textarea_get_text(ui_Text3);
//       //  int text_length = strlen(input_text);
        
//         // Lưu độ dài của text vào EEPROM
//         EEPROM.write(TEXT_ADDRESS, text_length);
        
//         // Lưu text vào EEPROM
//         for(int i = 0; i < text_length; i++) {
//             EEPROM.write(TEXT_ADDRESS + 1 + i, input_text[i]);
//         }
        
//         EEPROM.commit(); // Cần thiết cho ESP32/ESP8266
        
//         // In ra Serial để debug
//         Serial.println("Saved text to EEPROM:");
//         Serial.println(input_text);
        
//         // Chuyển màn hình (nếu cần)
//        _ui_screen_change(&ui_Screen2, LV_SCR_LOAD_ANIM_FADE_ON, 0, 0, &ui_Screen2_screen_init);
//     }
// }
}


void readFromEEPROM() {
    int saved_length = EEPROM.read(TEXT_ADDRESS);
    
    if(saved_length > 0 && saved_length < EEPROM_SIZE) {
        char saved_text[EEPROM_SIZE];
        
        for(int i = 0; i < saved_length; i++) {
            saved_text[i] = EEPROM.read(TEXT_ADDRESS + 1 + i);
        }
        saved_text[saved_length] = '\0';
        
     //   lv_textarea_set_text(ui_Text3, saved_text);
        Serial.println("Read from EEPROM:");
        Serial.println(saved_text);
    } else {
        Serial.println("No data in EEPROM or invalid length");
    }
}
void loop()
{
  char DHT_buffer[6];
  
  
  
  if(led == 1)
  digitalWrite(19, HIGH);
  if(led == 0)
  digitalWrite(19, LOW);
//Serial.println("hello world ");
  lv_timer_handler(); /* let the GUI do its work */
  delay( 5 );
}

void ui_event_Button12(lv_event_t * e) {
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    
    if(event_code == LV_EVENT_RELEASED) {
        // Đọc độ dài của text từ EEPROM
        int saved_length = EEPROM.read(TEXT_ADDRESS);
        
        if(saved_length > 0 && saved_length < EEPROM_SIZE) {
            char saved_text[EEPROM_SIZE];
            
            // Đọc text từ EEPROM
            for(int i = 0; i < saved_length; i++) {
                saved_text[i] = EEPROM.read(TEXT_ADDRESS + 1 + i);
            }
            saved_text[saved_length] = '\0';
            
            // Hiển thị text lên textarea
         //   lv_textarea_set_text(ui_Text3, saved_text);
            
            // Gửi dữ liệu qua UDP
            udp.beginPacket(UDP_ADDRESS, UDP_PORT);
            udp.write((uint8_t*)saved_text, saved_length);
            udp.endPacket();
            
            // In thông tin ra Serial để debug
            Serial.println("Read from EEPROM and sent via UDP:");
            Serial.println(saved_text);
        } else {
            Serial.println("No data in EEPROM or invalid length");
        }
    }
}
void ui_event_Button22(lv_event_t * e)
{
  lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    
    if(event_code == LV_EVENT_RELEASED) {
        // Đọc độ dài của text từ EEPROM
        int saved_length = EEPROM.read(TEXT_ADDRESS);
        
        if(saved_length > 0 && saved_length < EEPROM_SIZE) {
            char saved_text[EEPROM_SIZE];
            
            // Đọc text từ EEPROM
            for(int i = 0; i < saved_length; i++) {
                saved_text[i] = EEPROM.read(TEXT_ADDRESS + 1 + i);
            }
            saved_text[saved_length] = '\0';
            
            // Hiển thị text lên textarea
    //        lv_textarea_set_text(ui_Text3, saved_text);
            
            // Gửi dữ liệu qua UDP
            udp.beginPacket(UDP_ADDRESS, UDP_PORT);
            udp.write((uint8_t*)saved_text, saved_length);
            udp.endPacket();
            
            // In thông tin ra Serial để debug
            Serial.println("Read from EEPROM and sent via UDP:");
            Serial.println(saved_text);
        } else {
            Serial.println("No data in EEPROM or invalid length");
        }
    }
}
void ui_event_Button32(lv_event_t * e)
{
 lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    
    if(event_code == LV_EVENT_RELEASED) {
        // Đọc độ dài của text từ EEPROM
        int saved_length = EEPROM.read(TEXT_ADDRESS);
        
        if(saved_length > 0 && saved_length < EEPROM_SIZE) {
            char saved_text[EEPROM_SIZE];
            
            // Đọc text từ EEPROM
            for(int i = 0; i < saved_length; i++) {
                saved_text[i] = EEPROM.read(TEXT_ADDRESS + 1 + i);
            }
            saved_text[saved_length] = '\0';
            
            // Hiển thị text lên textarea
          //  lv_textarea_set_text(ui_Text3, saved_text);
            
            // Gửi dữ liệu qua UDP
            udp.beginPacket(UDP_ADDRESS, UDP_PORT);
            udp.write((uint8_t*)saved_text, saved_length);
            udp.endPacket();
            
            // In thông tin ra Serial để debug
            Serial.println("Read from EEPROM and sent via UDP:");
            Serial.println(saved_text);
        } else {
            Serial.println("No data in EEPROM or invalid length");
        }
    }
}
void ui_event_Button42(lv_event_t * e)
{
  lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    
    if(event_code == LV_EVENT_RELEASED) {
        // Đọc độ dài của text từ EEPROM
        int saved_length = EEPROM.read(TEXT_ADDRESS);
        
        if(saved_length > 0 && saved_length < EEPROM_SIZE) {
            char saved_text[EEPROM_SIZE];
            
            // Đọc text từ EEPROM
            for(int i = 0; i < saved_length; i++) {
                saved_text[i] = EEPROM.read(TEXT_ADDRESS + 1 + i);
            }
            saved_text[saved_length] = '\0';
            
            // Hiển thị text lên textarea
     //       lv_textarea_set_text(ui_Text3, saved_text);
            
            // Gửi dữ liệu qua UDP
            udp.beginPacket(UDP_ADDRESS, UDP_PORT);
            udp.write((uint8_t*)saved_text, saved_length);
            udp.endPacket();
            
            // In thông tin ra Serial để debug
            Serial.println("Read from EEPROM and sent via UDP:");
            Serial.println(saved_text);
        } else {
            Serial.println("No data in EEPROM or invalid length");
        }
    }
}
void connect_to_wifi(const char *ssid, const char *password)
{
   // lv_label_set_text(ui_Label_Status, "Đang kết nối WiFi...");
    WiFi.begin(ssid, password);

    int timeout = 15;  // Thời gian chờ kết nối (15 giây)
    while (WiFi.status() != WL_CONNECTED && timeout > 0)
    {
        delay(1000);
        timeout--;
        LV_LOG_USER("Đang kết nối... %d giây còn lại", timeout);
    }

    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println("wifi connected! ");
        LV_LOG_USER("Kết nối thành công! IP: %s", WiFi.localIP().toString().c_str());
     //   lv_label_set_text(ui_Label_Status, "Kết nối thành công!");
    }
    else
    {
        LV_LOG_USER("Kết nối thất bại!");
       // lv_label_set_text(ui_Label_Status, "Kết nối thất bại! Kiểm tra lại SSID/Password.");
    }
}
void ui_event_SaveWiFi(lv_event_t *e)
{
//   lv_event_code_t event_code = lv_event_get_code(e);
//   lv_obj_t *target = lv_event_get_target(e);
//   if (event_code == LV_EVENT_RELEASED)
//   {
//    // const char *wifi_ssid = lv_textarea_get_text(ui_TextArea3);
//    // const char *wifi_pwd = lv_textarea_get_text(ui_TextArea2);
//  if (wifi_ssid && wifi_pwd && strlen(wifi_ssid) > 0)
//         {
//             LV_LOG_USER("Bắt đầu kết nối WiFi...");
//             connect_to_wifi(wifi_ssid, wifi_pwd);
//             Serial.println("wifi connected! ");
//         }
//         else
//         {
//           Serial.println("Error pwd ");
//             //lv_label_set_text(ui_Label_Status, "SSID hoặc Password không hợp lệ!");
//         }
//   }

}

void ui_event_Button01(lv_event_t * e) {
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_RELEASED) {
        add_number_to_password('0');
    }
}

void ui_event_Button11(lv_event_t * e) {
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_RELEASED) {
        add_number_to_password('1');
    }
}

void ui_event_Button21(lv_event_t * e) {
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_RELEASED) {
        add_number_to_password('2');
    }
}

void ui_event_Button31(lv_event_t * e) {
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_RELEASED) {
        add_number_to_password('3');
    }
}

void ui_event_Button41(lv_event_t * e) {
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_RELEASED) {
        add_number_to_password('4');
    }
}

void ui_event_Button51(lv_event_t * e) {
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_RELEASED) {
        add_number_to_password('5');
    }
}

void ui_event_Button61(lv_event_t * e) {
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_RELEASED) {
        add_number_to_password('6');
    }
}

void ui_event_Button71(lv_event_t * e) {
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_RELEASED) {
        add_number_to_password('7');
    }
}

void ui_event_Button81(lv_event_t * e) {
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_RELEASED) {
        add_number_to_password('8');
    }
}

void ui_event_Button91(lv_event_t * e) {
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_RELEASED) {
        add_number_to_password('9');
    }
}

void ui_event_ButtonX(lv_event_t * e) {
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_RELEASED) {
        if(password_len > 0) {
            current_password[--password_len] = '\0';
            lv_textarea_set_text(ui_TextAreaS1, current_password);
        }
    }
}

void ui_event_ButtonCF(lv_event_t * e) {
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_RELEASED) {
        if(strcmp(current_password, "1234") == 0) {
            // Chuyển sang screen2
            lv_scr_load_anim(ui_Screen2, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, false);
            
            // Reset password
            password_len = 0;
            current_password[0] = '\0';
            lv_textarea_set_text(ui_TextAreaS1, "");
        } else {
            // Hiển thị thông báo sai mật khẩu (tùy chọn)
            lv_textarea_set_text(ui_TextAreaS1, "Wrong password!");
            password_len = 0;
            current_password[0] = '\0';
        }
    }
}
