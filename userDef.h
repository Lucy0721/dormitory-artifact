#define setFont_L u8g.setFont(u8g_font_courB14)
#define setFont_M u8g.setFont(u8g_font_fixed_v0r)
#define setFont_S u8g.setFont(u8g_font_chikitar)
#define INTERVAL_Time 1000
#define INTERVAL_OLED 1000

#define buzzer_pin 4


int micPin = A0; 
int micValue=0;
bool isRoar=false;

//U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE);//定义OLED连接方式

unsigned long Time_millis = millis();
unsigned long OLEDShowTime = millis();

int timeHH, timeMM, timeSS;
int year,month,day;
String stringDate;
String stringTime;



#define voice 100  //噪声阈值
#define INTERVALOLED 50   //刷新OLED的毫秒数
#define maxNoise 5    //在指定时间发上噪声的次数的阈值
#define micPin A0 //麦克风引脚
#define LEDPIN 6  //led灯引脚
#define buzzerPin 10  //蜂鸣器引脚
#define keyPin 8  //按键引脚


long timer;   //按键定时器
int numNoise=0;   //记录噪音频率
boolean isAlaram=false;   //标记是否开蜂鸣器
boolean add;    //标记蜂鸣器频率增加还是减少
int i = 200;    //蜂鸣器频率初始值
double recodeDB=0.0;  //显示在OLED上的最高分贝数

