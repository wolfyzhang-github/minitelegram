/******************************************************************************
 *                      迷你网络电报机本地驱动程序，基于Arduino
 *  
 *  By:      wolfyzhang
 *  Date:    2022.02.11
 *  For:     cshanshi
 *  Links:   https://habr.com/en/post/407057/
 *           https://github.com/shihaipeng03/MiniTelegraph
 *           https://github.com/YegorAlexandrow/miniTelegraph
 *           https://www.hackster.io/Yegor_A/mini-telegraph-0a6b83
 * 
 *****************************************************************************/

/******************************************************************************
 *  Description：
 * 
 *      Arduino从串口读到字符串数据，之后将字符串通过三个电机书写出来。
 *      三个电机为执行机构，步进电机负责走纸，控笔舵机负责笔尖的水平划动（笔划），控纸
 *  舵机负责纸带书写区域的起落。通过控笔和控纸舵机的合理配合，可以成功模拟点阵字符的针
 *  式打印效果。
 * 
 ******************************************************************************/

#include <Arduino.h>                                // Arduino头文件引入
#include <Servo.h>                                  // 舵机库头文件引入

#include "Stepper_28BYJ_48.h"                       // 步进电机库头文件引入  
#include "chars.h"                                  // 点阵字库头文件引入




#define PEN_PIN          2                          // 控笔舵机接口定义
#define PAPER_PIN        3                          // 控纸舵机接口定义

#define PAPER_UP_DELAY   80                         // 抬纸后停留时间  
#define PAPER_DOWN_DELAY 80                         // 落纸后停留时间
                                                    // 此处应配合好控笔舵机动作的时间，延时过小会导致控笔舵机的点、划动作无法完成

#define PAPER_UP         89                         // 控纸舵机提升后到达的位置，注意让笔刚好贴到纸上，不能太高
#define PAPER_DOWN       102                        // 控纸舵机下降后到达的位置，注意让翻板刚好放平，不能太低

#define PEN_MIN          55                         // 控笔舵机到达的顶部位置，对应参数为笔尖在纸带上触达区域的最高点
#define PEN_MAX          105                        // 控笔舵机到达的底部位置，对应参数为笔尖在纸带上触达区域的最低点

#define PEN_STEPS        12                         // 字符的高度，数值越小，字符越高，且必须小于PEN_MAX-PEN_MIN
#define BASE_LINE        8                          // 字符的基线位置，过大会超出纸的高度

#define PAPER_STEPS      9                          // 走纸幅度（步进电机旋转）的步长，数值越大则纸带行进速度越快，字符越扁

#define PEN_STEP (PEN_MAX - PEN_MIN) / PEN_STEPS    // 控笔舵机每步的距离

#define PEN_DELAY PEN_STEP*5                        // 每步的延时




Servo servoPen;                                     // 控笔舵机
Servo servoPaper;                                   // 控纸舵机
Stepper_28BYJ_48 stepper(4,5,6,7);                  // 步进电机（4、5、6、7端口）




void printDot(int m) {                              // 写单个点函数, 参数m为条件判断的参量
  int pos = 0;                                      // 初始化位置变量

  if (m) {                                          // m为真时，控纸舵机位置提升并保持，画点
    pos = 1;
    servoPaper.write(PAPER_UP);
    delay(PAPER_UP_DELAY);
  } else {                                          // m为假时，控纸舵机位置下降并保持，不画点
    pos = 0;
    servoPaper.write(PAPER_DOWN);
    delay(PAPER_DOWN_DELAY);
  }  
}


void printLine(int b) {                             // 画连续线函数，参量b为线长
  servoPen.write(PEN_MAX - (BASE_LINE-2)*PEN_STEP); // 控笔舵机到初始位置

  if(b != 0) {                                      // 如果b不为0，则开始画线
    servoPen.write(PEN_MAX - (BASE_LINE-2)*PEN_STEP); // 控笔舵机到初始位置
    delay(PEN_DELAY*20);
    for (int j = 0; b != 0; j++) {                  // b不为0则循环执行
      printDot(b&1); 
      b >>= 1;
      servoPen.write(PEN_MAX - (BASE_LINE+j)*PEN_STEP);
      delay(PEN_DELAY);
      }
    printDot(0);                                    // 画点函数状态置零
  }

  stepper.step(PAPER_STEPS);                        // 走纸幅度
}


void printChar(char c) {                            // 写某一字符函数
  int n = 0;

  Serial.println(c);
  for (int i = 0; i < 8; i++) {
    if(chars[c][i] != 0) {
      printLine(chars[c][i]);
      n++;
    }
    else stepper.step(PAPER_STEPS);                 // 走纸幅度
  }
}


void printString(char* str) {                       // 写某一字符串函数  

  while(*str != '\0') {
    printChar(*str);
    str+=1;
  }
}



char target[] = "begin";

void setup() {                                      // 初始化
  Serial.begin(115200);                             // 串口波特率初始化为115200
  Serial.setTimeout(10000);
  pinMode(PAPER_PIN, OUTPUT);                       // 控纸舵机接口状态初始化为OUTPUT
  pinMode(PEN_PIN, OUTPUT);                         // 控笔舵机接口状态初始化为OUTPUT

  servoPaper.attach(PAPER_PIN);                     // 控纸舵机初始化接口
  servoPaper.write(PAPER_DOWN);                     // 控纸舵机位置初始化为最低点

  servoPen.attach(PEN_PIN);                         // 控笔舵机初始化接口
  servoPen.write((PEN_MIN + PEN_MAX) / 2);          // 控笔舵机位置初始化为纸带中间位置

  while (true) {
    if (Serial.find(target)) {
      break;
    } else
     delay(1000);
  }

  printChar("   ");
  delay(3000);                                      // 延时3s
  Serial.println("miniTelegram Ready!");

}


int str, isCompleted;

void loop() {

  if(Serial.available() > 0) {
    servoPaper.attach(PAPER_PIN);
    servoPaper.write(PAPER_DOWN);
    servoPen.attach(PEN_PIN);

    while(Serial.available() > 0) {                 // 写串口接收到的字符
      str = Serial.read();
      isCompleted = 0;
      if(str >= 31) {                               // ASCII表中31以上的字符才打印
        Serial.print("Now printing: ");
        printChar((char)str);
      }
    }
    isCompleted = 1;
  }

  if(isCompleted) {
    Serial.println("Completed!");
    isCompleted = 0;
  }

  servoPaper.detach();
  servoPen.detach();                                // 放松舵机，避免持续工作过热
  Serial.write(1);
  delay(1500);
}