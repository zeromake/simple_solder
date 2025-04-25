#include <fw_hal.h>

#define uchar unsigned char
#define uint unsigned int
#define SET_MAX 300
#define LVD2V0 0x00 //LVD@2.0V
#define LVD2V4 0x01 //LVD@2.4V
#define LVD2V7 0x02 //LVD@2.7V
#define LVD3V0 0x03 //LVD@3.0V

unsigned int DisplayData[3];
unsigned int set_temp = 20;     // 开机初始化温度
unsigned char EC11_Stepping = 5; // 旋转编码器步进
unsigned int Temp_DigDisplay;
unsigned int sw_a;
unsigned char __CODE smgduan[10] = {0xe7, 0x05, 0xe9, 0xad, 0x0f, 0xae, 0xee, 0x85, 0xef, 0xaf};
unsigned int t;

INTERRUPT(PowerLost, EXTI_VectLowVoltDect)
{
  EA = 0; //关闭所有中断
  IAP_WriteData(set_temp>>8 & 0x255);//写高8位数据
  IAP_CmdWrite(0);
  IAP_WriteData(set_temp & 0x255);//写低8位数据
  IAP_CmdWrite(1);

  while((PCON & 0x20) != 0) //复查掉电标志
  {
    PCON &= 0xDF; //清除掉电标志
    NOP();               
    NOP(); //坐等掉电
  }
  IAP_CONTR  = 0x20; //发现是误报，重启单片机，恢复正常工作
}

SBIT(SO, _P3, 4);     // P3.4口与SO相连
SBIT(SCK, _P3, 0);    // P3.0口与SCK相连
SBIT(CS, _P3, 1);     // P3.1口与CS相连
SBIT(LSA, _P3, 5);    // 数码管位选信号
SBIT(LSB, _P3, 6);    // 数码管位选信号
SBIT(LSC, _P3, 7);    // 数码管位选信号
SBIT(EC11_A, _P3, 3); // 旋转编码器A相
SBIT(EC11_B, _P3, 2); // 旋转编码器B相
SBIT(EC11_E, _P5, 4); // 旋转编码器E相
SBIT(sw, _P5, 5);     // 固态继电器控制

// PID参数
float Kp = 14.00;      // 比例控制
float Ki = 90.00;      // 积分控制
float Kd = 210.00;     // 微分控制
float integral = 0.0;  // 积分项
float lastError = 0.0; // 上次误差

void setup(void) // 初始化函数
{
  P1M1 = 0x00; // P1口推挽输出
  P1M0 = 0xff;
  P3M1 = 0x00; // P3口准双向
  P3M0 = 0x00;
  P5M1 = 0x00; // P5.4口准双向，P5.5口推挽输出
  P5M0 = 0x20;

  EA = 1; // 开启中断
  RSTCFG = LVD3V0; //使能 2.4V 时低压中断
  ELVD = 1; //使能 LVD 中断

  uint16_t num = 0;
  IAP_CmdRead(0);
  num = IAP_ReadData(); //读高8位
  num <<= 8;
  IAP_CmdRead(1);
  num |= IAP_ReadData(); //读低8位
  if (num != 0xffff) {
    set_temp = num;
  }
  IAP_CmdErase(0); // 擦除扇区（扇区首地址0x0000）
}

void delay(unsigned int i) // 延时函数
{
  while (i--)
    ;
}

unsigned int MAX6675_ReadReg(void) // 读取6675数据
{
  unsigned char i;
  unsigned int dat;

  i = 0;
  dat = 0;

  SCK = 0;
  CS = 0;

  for (i = 0; i < 16; i++)
  {
    SO = 1;
    SCK = 1;
    dat = dat << 1;
    if (SO == 1)
      dat = dat | 0x01;
    SCK = 0;
  }
  CS = 1;
  dat = dat << 1; // 读出来的数据的D3~D14是温度值
  dat = dat >> 4;
  dat = dat / 4; // 测得的温度单位是0.25，所以要乘以0.25（即除以4）才能得到以度为单位的温度值

  return dat;
}

void DigDisplay(unsigned int temp) // 显示函数
{
  unsigned char i;
  DisplayData[0] = temp / 1 % 10;
  DisplayData[1] = temp / 10 % 10;
  DisplayData[2] = temp / 100 % 10;
  for (i = 0; i < 3; i++)
  {
    switch (i)
    {
    case (0):
      LSA = 0;
      LSB = 1;
      LSC = 1;
      break;
    case (1):
      LSA = 1;
      LSB = 0;
      LSC = 1;
      break;
    case (2):
      LSA = 1;
      LSB = 1;
      LSC = 0;
      break;
    }
    P1 = smgduan[DisplayData[i]];
    delay(200);
    P1 = 0x00;
  }
}

void EC11(void) // 旋转编码器驱动
{
  static __BIT flag = 1;
  if ((EC11_A != EC11_B) && (flag))
  {
    if (EC11_A)
    {
      set_temp = set_temp + (int)EC11_Stepping;
      if (set_temp > SET_MAX) {
        set_temp = 0;
      }
      Temp_DigDisplay = set_temp;
      t = 0;
    }
    else
    {
      set_temp = set_temp - (int)EC11_Stepping;
      if (set_temp > SET_MAX) {
        set_temp = SET_MAX;
      }
      Temp_DigDisplay = set_temp;
      t = 0;
    }
    flag = 0;
    sw = 0;
  }
  if (EC11_A == EC11_B)
  {
    flag = EC11_A;
  }
}

float PID(float currentTemperature, float _set_temp) // PID控制
{
  float error;
  float derivative;
  float output;
  float proportional;

  error = _set_temp - currentTemperature; // 计算误差

  proportional = Kp * error; // 计算比例项

  integral += Ki * error; // 计算积分项

  if (integral < -180)
  {
    integral = -180;
  }
  else if (integral > 180)
  {
    integral = 180;
  }

  derivative = Kd * (error - lastError); // 计算微分项

  output = proportional + integral + derivative; // 计算输出

  lastError = error; // 保留误差

  output = (output < 0.0) ? 0.0 : (output > 1200.0) ? 1200.0
                                                    : output;

  return output;
}

void main(void)
{
  setup(); // 复位
  while (1)
  {
    Temp_DigDisplay = MAX6675_ReadReg();
    // sw_a = PID(Temp_DigDisplay, set_temp);
    if (set_temp > Temp_DigDisplay && 30 < set_temp - Temp_DigDisplay)
    {
      sw_a = 1200;
    }
    else
      sw_a = PID(Temp_DigDisplay, (float)set_temp);

    for (t = 0; t < 1200; t++)
    {
      DigDisplay(Temp_DigDisplay); // 显示温度
      EC11();
      if (t > 1200 - sw_a)
      {
        sw = 1;
      }
    }
    sw = 0;
  }
}
