/////////////////////////////////////////////////////////////
// 텀프로젝트명: 이진계산기
// 과제개요: 2bit의 크기를 가지는 피연산자를 6개의 연산자 중 하나를 선택하여
//                  연산한 후 4bit로 출력, 연속모드로 결과값의 연속적으로 +1
// 사용한  하드웨어(기능): GPIO, SW, LED, Fram, EXTI
// 제출일: 2023. 6. 11
// 제출자  클래스: 화요일반
//           학번: 2019132036
//           이름: 정지우
///////////////////////////////////////////////////////////////
#include "stm32f4xx.h"
#include "GLCD.h"
#include "FRAM.h"

void _GPIO_Init(void);
void _EXTI_Init(void);

void DisplayInitScreen(void);
uint16_t KEY_Scan(void);
void BEEP(void);

void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);

void UpdateDisplay(int); // LCD에 표시되는 변수를 LCD에 그리는 함수

uint8_t numA = 0;               // 피연산자 1
uint8_t numB = 0;               // 피연산자 2
uint8_t result;                 // 결과값
const char operatorlist[] = {'+', '-', 'x', '&', '|', '^'}; // 연산자를 정수로 접근하기 위한 리스트
uint8_t operator_ = 0;             // 연산자의 인덱스
int flag = 0;                           // 결과값을 1씩 증가시키는 루틴을 종료시키는 flag

int main(void)
{
  /*시스템 초기화 */
   LCD_Init();                                                          // LCD 모듈 초기화
   _GPIO_Init();                                                        // GPIO 초기화
   _EXTI_Init();                                                          // EXTI 초기화
    Fram_Init();                                                            // FRAM 초기화 H/W 초기화
	Fram_Status_Config();                                         // FRAM 초기화 S/W 초기화
    DelayMS(10);
    
    /* 변수 및 상태 초기화*/
   DisplayInitScreen();                                             // LCD 초기화면
   
   int data = Fram_Read(530);                                   // Fram의 530번지에서 데이터를 불러옴 
   if (data > 5 || data < 0) {Fram_Write(530, 0);}      // Fram에서 불러온 값이 유효한 값이 아니면 Fram 530번지 0으로 초기화
   
   data = Fram_Read(531);                                       // Fram의 531번지에서 데이터를 불러옴
   data &= ~0x80;                                                       // FRAM의 데이터의 7번비트를 0으로 (결과에 음수를 7번비트에 표기함. 하지만 컴퓨터에서는 이를 숫자로 인식하기 때문에 예외처리를 해줌)
   if (data < 0 || data > 15) {Fram_Write(531, 0);}     // Fram에서 불러온 값이 유효한 값이 아니면 Fram 531번지 0으로 초기화
   
   operator_ = Fram_Read(530);      // Fram의 530번지에서 데이터를 불러와 연산자 인덱스 변수에 저장 
   result = Fram_Read(531);             // Fram의 531번지에서 데이터를 불러와 결과에 저장
   
   UpdateDisplay(0);                        // LCD에 초기 변수값 출력
   
   /*Main 동작*/
   while(1)
   {
     switch (KEY_Scan()) {              // SW 입력
     case 0xFB00:                               //  SW2
       numA &= 0x03;                          // numA의 사용하지 않는 비트 0으로 초기화
       numA ^= 0x02;                            // 상위비트 XOR
       BEEP();
       UpdateDisplay(0);                    // LCD 출력
     break;
     case 0xF700:                               // SW3
       numA &= 0x03;                            // numA의 사용하지 않는 비트 0으로 초기화
       numA ^=0x01;                             // 하위비트 XOR
       BEEP();
       UpdateDisplay(0);                        // LCD출력
     break;
     case 0xEF00:                               // SW4
       numB &= 0x03;                            // numB의 사용하지 않는 비트 0으로 초기화
       numB ^=0x02;                             // 상위비트 XOR
       BEEP();
       UpdateDisplay(0);                        //LCD출력
     break;
     case 0xDF00:                               // SW5
       numB &= 0x03;                            // numB의 사용하지 않는 비트 0으로 초기화
       numB ^=0x01;                             // 하위비트 XOR
       BEEP();
       UpdateDisplay(0);                        // LCD출력
     break;
     }
   }
}

/* GLCD 초기화면 설정 함수 */
void DisplayInitScreen(void)
{
  LCD_Clear(RGB_WHITE);                          // 화면 클리어
  LCD_SetBackColor(RGB_WHITE);          // 글자배경색 : WHITE
  LCD_SetTextColor(RGB_BLACK);           // 글자색 : Black
  
  /*연산기호 상자 출력*/
  LCD_SetBrushColor(GET_RGB(255, 0, 255));  // 분홍색 브러쉬 선택
  LCD_SetPenColor(RGB_BLACK);                   // 검정펜 선택
  LCD_DrawFillRect(70, 53, 20, 25);                     // 연산기호 상자 출력
  LCD_DrawRectangle(70, 53, 20, 25);
  
  /*메인 상자 출력*/
  LCD_SetPenColor(RGB_GREEN);                   // 초록펜 선택
  LCD_DrawRectangle(54, 15, 50, 100);
  
  /*피연산자 종류 상자 출력*/
  LCD_DrawRectangle(5, 31, 20, 20);             // A피연산자 표기 상자 출력
  LCD_DrawRectangle(5, 81, 20, 20);             // B피연산자 표기 상자 출력
  LCD_DrawRectangle(137, 55, 20, 20);           // 결과값 표시 상자 출력
  LCD_SetFont(&Gulim10);                                // 글자크기 10으로
  
  LCD_DrawChar(11, 33, 'A');            // A출력
  LCD_DrawChar(11, 83, 'B');            // B출력
  LCD_DrawChar(142, 57, 'C');           // C출력
  
  /*선 출력*/
  LCD_SetPenColor(RGB_BLACK);                   // 겸정펜 선택
  LCD_DrawHorLine(105, 22, 14);                     // A MSB 선 출력
  LCD_DrawHorLine(40, 32, 14);                      // A LSB 선 출력
  LCD_DrawHorLine(40, 52, 14);          
  LCD_DrawHorLine(40, 82, 14);
  LCD_DrawHorLine(40, 102, 14);
  for (int i = 0; i < 4; i++) LCD_DrawHorLine(105, 47 + 20 * i, 14);
  
  /*피연산자 및 결과 상자 출력*/
  LCD_SetBrushColor(RGB_YELLOW);            // 노란색 펜 선택
  for (int i = 0; i < 51; i = i+50) {                       // 두 피연산자 상자 출력
    LCD_DrawFillRect(28, 24 + i, 15, 15);
    LCD_DrawFillRect(28, 44 + i, 15, 15);
    LCD_DrawRectangle(28, 24 + i, 15, 15);
    LCD_DrawRectangle(28, 44 + i, 15, 15);
  }
  for (int i = 0; i  < 4; i++) {                            // 결과값 상자 출력
    LCD_DrawFillRect(119, 40 + 20 * i, 15, 15);
    LCD_DrawRectangle(119, 40 + 20 * i, 15, 15);
  }
  LCD_DrawFillRect(70, 97, 20, 15);                 //  연속모드 상자 출력
  LCD_DrawRectangle(70, 97, 20, 15);
  
  LCD_DrawFillRect(119, 15, 15, 15);                // 결과 부호 상자 출력
  LCD_DrawRectangle(119, 15, 15, 15);
  LCD_SetFont(&Gulim8);                                             // 폰트 : 굴림 8
  LCD_SetBackColor(RGB_YELLOW);                             // 글자 배경색 노란색
  LCD_DrawChar(72, 99, '+');                            // 연속모드의 +출력
}

void UpdateDisplay(int mode) {
  LCD_SetFont(&Gulim10);                                // 폰트 : 굴림 10
  LCD_SetBackColor(GET_RGB(255, 0, 255));   //글자배경색 분홍색
  LCD_DrawChar(76, 56, operatorlist[operator_]); // 연산자 출력
  
  LCD_SetFont(&Gulim8);                                             // 폰트 : 굴림 8
  LCD_SetBackColor(RGB_YELLOW);                             // 글자 배경색 노란색
  LCD_DrawChar(32, 26, ((numA & 0x02)>>1) + 0x30);      // numA의 상위비트 출력
  LCD_DrawChar(32, 46, ((numA & 0x01)>>0) + 0x30);      // numA의 하위비트 출력
  LCD_DrawChar(32, 76, ((numB & 0x02)>>1) + 0x30);      // numB의 상위비트 출력
  LCD_DrawChar(32, 96, ((numB & 0x01)>>0) + 0x30);      // numB의 하위비트 출력
  char sig;                                                                     //  부호를 출력할 변수 생성
  if ((result & 0x80)>>7 == 1) sig = '-';                               // 결과의 최상위 비트에 1이 있으면 sig에 -저장
  else sig =  '+';                                                               // 결과의 최상위 비트에 0이 있으면 sig에 +저장
  LCD_DrawChar(123, 17, sig);                                      // 결과의 부호 출력
  LCD_DrawChar(123, 42, ((result & 0x08)>>3) + 0x30);   // 결과의 상위비트  출력
  LCD_DrawChar(123, 62, ((result & 0x04)>>2) + 0x30);   // 결과의 상위에서 2번째 비트 출력
  LCD_DrawChar(123, 82, ((result & 0x02)>>1) + 0x30);   // 결과의 상위에서 3번째 비트 출력
  LCD_DrawChar(123, 102, ((result & 0x01)>>0) + 0x30);  // 결과의 하위비트 출력
  
  LCD_DrawChar(80, 99, mode + 0x30);                            // 연속모드 더하는 숫자 출력
}

/* GPIO (GPIOG(LED), GPIOH(Switch), GPIOF(Buzzer)) 초기 설정   */
void _GPIO_Init(void)
{
   // LED (GPIO G) 설정 : Output mode
   RCC->AHB1ENR   |=  0x00000040;           // RCC_AHB1ENR : GPIOG(bit#6) Enable                     
   GPIOG->MODER    |=  0x00005555;          // GPIOG 0~7 : Output mode (0b01)                  
   GPIOG->OTYPER   &= ~0x00FF;              // GPIOG 0~7 : Push-pull  (GP8~15:reset state)   
   GPIOG->OSPEEDR    |=  0x00005555;        // GPIOG 0~7 : Output speed 25MHZ Medium speed 
   
   // SW (GPIO H) 설정 : Input mode 
   RCC->AHB1ENR    |=  0x00000080;          // RCC_AHB1ENR : GPIOH(bit#7) Enable                     
   GPIOH->MODER    &= ~0x003C0000;          // GPIOH 1,2 : Input mode (reset state)            
   GPIOH->PUPDR    &= ~0x003C0000;          // GPIOH 1,2 : Floating input (No Pull-up, pull-down) :reset state

   // Buzzer (GPIO F) 설정 : Output mode
   RCC->AHB1ENR   |=  0x00000020;           // RCC_AHB1ENR : GPIOF(bit#5) Enable                     
   GPIOF->MODER    |=  0x00040000;          // GPIOF 9 : Output mode (0b01)                  
   GPIOF->OTYPER    &= ~0x0200;             // GPIOF 9 : Push-pull     
   GPIOF->OSPEEDR    |=  0x00004000;      // GPIOF 9 : Output speed 25MHZ Medium speed 
}   

/* EXTI (EXTI8(GPIOH.8, SW0), EXTI9(GPIOH.9, SW1)) 초기 설정  */
void _EXTI_Init(void)
{
   RCC->AHB1ENR    |= 0x00000180;   // RCC_AHB1ENR GPIOH,GPIOI Enable
   RCC->APB2ENR    |= 0x00004000;   // Enable System Configuration Controller Clock
   
   GPIOH->MODER    &= ~0x300C0000;   // GPIOH PIN9,PIN14 Input mode (reset state)             
   GPIOI->MODER &= ~0x00033000;         // GPIOI PIN6, pin8 Inpput mode

  SYSCFG->EXTICR[1] |= 0x0800;      // EXTI6에 대한 소스 입력은 GPIOI로 설정
   SYSCFG->EXTICR[2] |= 0x0078;   // EXTI9에 대한 소스 입력은 GPIOH로 설정, EXTI8에 대한 소스 입력은 GPIOI로 설정
   SYSCFG->EXTICR[3] |= 0x0700;     // EXTI14에 대한 소스 입력은 GPIOH로 설정

   EXTI->FTSR |= 0x004340;              // EXTI6,8,9,14: Falling Trigger Enable 
   EXTI->IMR  |= 0x004340;              // EXTI8,9 인터럽트 mask (Interrupt Enable) 설정
      
   NVIC->ISER[0] |= (1<<23);            // Enable 'Global Interrupt EXTI6,8' == (1<<23)
  NVIC->ISER[1] |= (1<<(40-32));        // 40 == (1<<(40-32))
  
  NVIC->IP[23] = 0xF0;                      // EXTI6 우선 순위 최하위로
  NVIC->IP[40] = 0xE0;                      // EXTI14 우선 순위 최하에서 두번째로
}

/* EXTI5~9 인터럽트 핸들러(ISR: Interrupt Service Routine) */
void EXTI9_5_IRQHandler(void)   // EXTI6, EXTI8, EXIT9 Handler
{      
   if(EXTI->PR & 0x0040)                    // EXTI6 Interrupt Pending(발생) 여부 //J_UP
   {
      EXTI->PR |= 0x0040;                   // Pending bit Clear
      EXTI->IMR &= ~0x4000;             // 마스크를 씌워 연속으로 발생한 인터럽트  해제
      flag = 1;                                     // 연속모드 flag를 1로하여 ON
      GPIOG->ODR |= 0x80;               // 7LED ON
      EXTI->IMR |= 0x4000;                  // EXTI6 마스크 해제
      result &= ~0x80;
      while(flag) {                                 // 연속 모드
        result++;                                   //결과 1증가
        if (result & 0x10) result = 0;      // 결과의 하위비트로부터 5번째 비트가 1이되면 결과를 0으로 초기화
        Fram_Write(531, result);            // Fram 531번지에 결과 저장
        UpdateDisplay(1);                   // 디스플레이에 연속모드 숫자를 1로 바꾸고 LCD업데이트
        BEEP();
        DelayMS(500);
      }
      UpdateDisplay(0);                   // 디스플레이에 연속모드 숫자를 0로 바꾸고 LCD업데이트
      BEEP();
      DelayMS(500);
      BEEP();
      DelayMS(500);
      BEEP();
      GPIOG->ODR &= ~0x80;    // 7LED OFF
   } 
  else if (EXTI->PR & 0x0100)       //EXTI8 Interrupt Pending(발생) 여부 //J_RIGHT
  {
    EXTI->PR |= 0x0100;                 // Pending bit Clear
    EXTI->IMR &= ~0x0100;// 마스크를 씌워 연속으로 발생한 인터럽트  해제
    operator_++;                                //연산자 인덱스 1증가
    if (operator_ > 5) operator_ = 0;   // 연산자 인덱스가 5보다 크면 0으로  초기화
    Fram_Write(530, operator_);         // 연산자 인덱스 Fram530번지에 저장
    UpdateDisplay(0);                       // 디스플레이 업데이트
    BEEP();
    EXTI->IMR |= 0x0100;// EXTI8 마스크 해제
  }
  else if (EXTI->PR & 0x0200)       //EXTI9 Interrupt Pending(발생) 여부 // SW1
  {
    EXTI->PR |= 0x0200;                 // Pending bit Clear
    EXTI->IMR &= ~0x0200;               // 마스크를 씌워 연속으로 발생한 인터럽트  해제
    BEEP();
    result = 0;                                     // 결과를 0으로 초기화 (부호 및 데이터 초기화)
    switch (operator_) {                    // 연산자 인덱스에 따라 연산 분류
    case 0:                                         // 인덱스 0 , +
      result = numA + numB;                 //numA와 numB를 더해 결과에 저장
      break;
    case 1:                                         // 인덱스 1 , -                      
      result = abs(numA - numB);        // 결과에 numA와 numB의 차이 저장
      if ((numA- numB) < 0)               // 빼기를 하면 음수이면 결과 변수의 7번 비트를 1로 함.
        result |= 0x80;
      break;
    case 2:                                         // 인덱스 2 , *
      result = numA * numB;                 // numA와 numB를 곱해 결과에 저장
      break;    
    case 3:                                         // 인덱스 3 , &
      result = numA & numB;                 // numA와 numB를 and연산하여 결과에 저장
      break;
    case 4:                                         // 인덱스 4 , |
      result = numA | numB;                 // numA와 numB or 연산하여 결과에 저장
      break;
    case 5:                                         //인덱스 5 , ^
      result = numA ^ numB;                 // numA와 numB를 XOR 연산하여 결과에 저장
      break;
    }
    Fram_Write(531, result);                // Fram 531번지에 결과 저장
    UpdateDisplay(0);                           // 디스플레이 업데이트
    EXTI->IMR |= 0x0200;                    // EXTI9 마스크 해제
  }
}

/* EXTI10~15 인터럽트 핸들러(ISR: Interrupt Service Routine) */
void EXTI15_10_IRQHandler(void) 
{                              // SW6
  
   if(EXTI->PR & 0x4000)         // EXTI14 Interrupt Pending(발생) 여부?
   {
    EXTI->PR |= 0x4000;         // Pending bit Clear 
    EXTI->IMR &= ~0x0040;       // 마스크를 씌어 연속으로 발생한 인터럽트 제거

    flag = 0;                               // 연속모드 flag를 0 ->종료
    BEEP();
    DelayMS(1000);                  // 1초 대기
    EXTI->IMR |= 0x0040;          // EXTI14 마스크 해제
   }
}

/* Switch가 입력되었는지 여부와 어떤 switch가 입력되었는지의 정보를 return하는 함수  */ 
uint8_t key_flag = 0;
uint16_t KEY_Scan(void)   // input key SW0 - SW7 
{ 
   uint16_t key;
   key = GPIOH->IDR & 0xFF00;   // any key pressed ?
   if(key == 0xFF00)      // if no key, check key off
   {   if(key_flag == 0)
         return key;
      else
      {   DelayMS(10);
         key_flag = 0;
         return key;
      }
   }
   else            // if key input, check continuous key
   {   if(key_flag != 0)   // if continuous key, treat as no key input
         return 0xFF00;
      else         // if new key,delay for debounce
      {   key_flag = 1;
         DelayMS(10);
          return key;
      }
   }
}

/* Buzzer: Beep for 30 ms */
void BEEP(void)         
{    
   GPIOF->ODR |=  0x0200;   // PF9 'H' Buzzer on
   DelayMS(10);      // Delay 30 ms
   GPIOF->ODR &= ~0x0200;   // PF9 'L' Buzzer off
}

void DelayMS(unsigned short wMS)
{
   register unsigned short i;
   for (i=0; i<wMS; i++)
      DelayUS(1000);   // 1000us => 1ms
}

void DelayUS(unsigned short wUS)
{
   volatile int Dly = (int)wUS*17;
   for(; Dly; Dly--);
}