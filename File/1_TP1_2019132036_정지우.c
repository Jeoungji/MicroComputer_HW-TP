/////////////////////////////////////////////////////////////
// 텀프로젝트명: 엘레베이터
// 과제개요: 두개의 엘레베이터를 시각화하여 탑승층에 
//              따라 두 엘레베이터 중 가까운 엘레베이터가 
//              선택되어 목표층까지 이동하여 전원이 종료
//              되어도 이전 엘레베이터의 층수를 유지하는 
//             프로그램 작성
// 사용한  하드웨어(기능): GPIO, SW, LED, Fram, EXTI
// 제출일: 2023. 6. 6
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

void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);

typedef struct Elevator{                        // 엘레베이터의 정보를 가지고 있는 구조체
  char num;                                             // 엘레베이터의 좌우정보
  int location;                                             // 엘레베이터 LCD출력 x좌표 저장
  int pressent;                                         // 현재층
}Elevator;

void Move(Elevator*, int);                          // 엘레베이터 움직이기 함수
void UpdateDisplay(Elevator*, char);     // 디스플레이 업데이트
void BEEPS(int);                                    // 횟수를 입력받아 횟수만큼 부저를 울림

int _mode = 0;              // 동작모드 : 0:층선택모드, 1:실행모드
int _select = 1;                // 선택된 엘레베이터 : 0:left, 1:Right
int _target = 1;                // 목표층
int _user_floor = 1;            // 탑승할 층

int main(void)
{
  /*시스템 초기화*/
   LCD_Init();                       // LCD 모듈 초기화
   _GPIO_Init();                    // GPIO 초기화
   _EXTI_Init();                    // EXTI 초기화
    Fram_Init();                    // FRAM 초기화 H/W 초기화
	Fram_Status_Config();   // FRAM 초기화 S/W 초기화
    DelayMS(10);
    
    /*사용하는 Fram주소에 유효한 값이 없으면 1로 초기화*/
    if (Fram_Read(2023) < 1 || Fram_Read(2023) > 6) {Fram_Write(2023, 1);}
    if (Fram_Read(2024) < 1 || Fram_Read(2024) > 6) {Fram_Write(2024, 1);}
    
    /*Fram에서 엘레베이터 이전 층수를 받아 Elevator초기화*/
   Elevator LElevator = {'L', 25, Fram_Read(2023)};
   Elevator RElevator = {'R', 115, Fram_Read(2024)};
  Elevator ele[] = {LElevator, RElevator};              // 엘레베이터를 관리하기 편하도록 배열에 저장
  
  DisplayInitScreen();              // LCD 초기화면
  UpdateDisplay(ele, 'S');          // 엘레베이터 S모드로 표기하고 오른쪽 엘레베이터 그리기
  _select = 0;                          // 왼쪽 엘레베이터 선택
  UpdateDisplay(ele, 'S');          // 엘레베이터모드 S로하고 왼쪽 엘레베이터 그리기

  GPIOG->ODR &= ~0x00FF;   // 초기값: LED0~7 Off
  GPIOG->ODR |= 0x0080;     // 7LED ON
   while(1) {
     if (_mode == 0) {                              // 층선택 모드
      switch (KEY_Scan()) {
      case 0xFD00:                                  // SW1 down
         GPIOG->ODR &= ~0x0004;             // LED3 OFF
         GPIOG->ODR |= 0x0002;                  // LED2 ON
         _user_floor++;                                 // 탑승층 증가
          if (_user_floor > 6) _user_floor =1;      // 탑승층이 6보다 크면 1로 초기화
          UpdateDisplay(ele, 'S');                      // 디스플레이 업데이트
          BEEPS(1);                                     // 부저 1회
       break;
      case 0xFB00:                                  // SW2 down
         GPIOG->ODR &= ~0x0002;             // LED2 OFF
         GPIOG->ODR |= 0x0004;                  // LED3 ON
         _target++;                                     // 목표층 1증가
          if (_target > 6) _target = 1;                 // 목표층이 6보다  크면 1로 초기화
          UpdateDisplay(ele, 'S');                      //디스플레이 업데이트
          BEEPS(1);                                     // 부저 1회
      break;
      }
     } else if (_mode == 1) {                       // 실행모드
       int Ldiffer = abs(ele[0].pressent-_user_floor);  // 탑승층과 왼쪽 엘레베이터 차이 계산
       int Rdiffer = abs(ele[1].pressent-_user_floor);  // 탑승층과 오른쪽 엘레베이터 차이 계산
       if (Rdiffer >= Ldiffer) _select = 0;                     // 좌측 엘레베이터가 더 가깝거나 같으면 좌측 엘레베이터 선택
       else _select = 1;                                            // 우측엘레베이터가 더 가까우면 우측엘레베이터 선택
       DelayMS(500);                                                // 0.5초 딜레이
       Move(ele, _user_floor);                                  // 선택된 엘레베이터를 탑슥층으로 이동
       UpdateDisplay(ele, 'S');                                     // 엘레베이터를 stop모드로 하고 디스플레이  업데이트
       DelayMS(500);                                                // 0.5초 딜레이
       Move(ele, _target);                                          // 엘레베이터 목표층으로 이동
       
       Fram_Write(2023, ele[0].pressent);                   // 좌측 엘레베이터의 현재층을 Fram 2023번지에 저장
       Fram_Write(2024, ele[1].pressent);                   // 우측 엘레베이터의 현재틍을 Fram 2024번지에 저장
       BEEPS(3);                                                    // 부저 3회
       GPIOG->ODR &= ~0x01;                                 // LED1 OFF
       GPIOG->ODR |= 0x80;                                      // LED7 ON
       _mode = 0;                                                   // 층선택 모드로 변경
     }
  }
}

/* GLCD 초기화면 설정 함수 */
void DisplayInitScreen(void)
{
   LCD_Clear(RGB_WHITE);      // 화면 클리어
   LCD_SetFont(&Gulim8);      // 폰트 : 굴림 8
   LCD_SetBackColor(RGB_WHITE);   // 글자배경색 : Green
   LCD_SetTextColor(RGB_BLACK);   // 글자색 : Black
   LCD_DisplayText(0,0,"MC-Elevator(JJW)");     // Title
  LCD_DisplayChar(6, 9, '>');                           //화살표 표시
  
  LCD_SetTextColor(RGB_GREEN);      // 초록색 글자
  for (int i = 1; i < 7; i++) {                 // 좌측 층 번호 출력
    char num = (7-i) + 0x30;                // 숫자를 char로 바꿔가며 출력
    LCD_DisplayChar(i+1,13, num);
  } 
  LCD_SetTextColor(RGB_BLUE);       // 파란색 글자
  LCD_DisplayText(4, 8, " -E");         // 선택된 엘레베이터가 표기될 글자 표기
  for (int i = 1; i < 7; i++) {                 // 우측 층표기
    char num = (7-i) + 0x30;                // 숫자를 char로 바꿔가며 출력
    LCD_DisplayChar(i+1,5, num);
  }
}

/*엘레베이터 LCD업데이트*/
void UpdateDisplay(Elevator* ele, char direction)
{
   LCD_SetFont(&Gulim8);                        // 폰트 : 굴림 8
   LCD_SetBackColor(RGB_WHITE);             // 글자배경색 : Green
   LCD_SetTextColor(RGB_RED);                   // 글자색 : Black
  if (_mode == 0) LCD_DisplayText(2, 9, "FL");  // mode가 0일때 FL표기
  else if (_mode == 1) LCD_DisplayText(2, 9, "EX"); // mode가 1일때 EX표기
  else LCD_DisplayText(2, 9, "??");                         // mode에 이상한 값이 저장될 시 ??를 띄움
  LCD_DisplayChar(6, 8, _user_floor + 0x30);            // 탑승층 출력
  LCD_DisplayChar(6, 10, _target + 0x30);               // 목표층 출력
   LCD_DisplayChar(5,9,direction);                          // 엘레베이터 이동방향 출력
  LCD_SetTextColor(RGB_BLUE);                               // 글자색 파란색
  LCD_DisplayChar(4, 8, ele[_select].num);              // 선택된 엘레베이터가 우측인지 좌측인지 표기
  
  LCD_SetBrushColor(RGB_WHITE);                         // 브러쉬를 흰색
  LCD_DrawFillRect(ele[_select].location, 10, 11, 100); //선택된 엘레베이터를 흰색사각형을 그려 덮음

  if (_select == 0) LCD_SetBrushColor(RGB_BLUE);    // 좌측 엘레베이터면 파란색 브러쉬
  else LCD_SetBrushColor(RGB_GREEN);                // 우측 엘레베이터면 초록색 브러쉬

  // 엘레베이터에 저장된 x좌표와 엘레베이터의 현재층을 토대로 선택된 엘레베이터 그리기
  LCD_DrawFillRect(ele[_select].location, 14 + 13*(7-ele[_select].pressent) ,10, 13*(ele[_select].pressent));
}

/* GPIO (GPIOG(LED), GPIOH(Switch), GPIOF(Buzzer)) 초기 설정   */
void _GPIO_Init(void)
{
   // LED (GPIO G) 설정 : Output mode
   RCC->AHB1ENR   |=  0x00000040;   // RCC_AHB1ENR : GPIOG(bit#6) Enable                     
   GPIOG->MODER    |=  0x00005555;   // GPIOG 0~7 : Output mode (0b01)                  
   GPIOG->OTYPER   &= ~0x00FF;   // GPIOG 0~7 : Push-pull  (GP8~15:reset state)   
   GPIOG->OSPEEDR    |=  0x00005555;   // GPIOG 0~7 : Output speed 25MHZ Medium speed 
   
   // SW (GPIO H) 설정 : Input mode 
   RCC->AHB1ENR    |=  0x00000080;   // RCC_AHB1ENR : GPIOH(bit#7) Enable                     
   GPIOH->MODER    &= ~0x003C0000;   // GPIOH 1,2 : Input mode (reset state)            
   GPIOH->PUPDR    &= ~0x003C0000;   // GPIOH 1,2 : Floating input (No Pull-up, pull-down) :reset state

   // Buzzer (GPIO F) 설정 : Output mode
   RCC->AHB1ENR   |=  0x00000020;   // RCC_AHB1ENR : GPIOF(bit#5) Enable                     
   GPIOF->MODER    |=  0x00040000;   // GPIOF 9 : Output mode (0b01)                  
   GPIOF->OTYPER    &= ~0x0200;   // GPIOF 9 : Push-pull     
   GPIOF->OSPEEDR    |=  0x00004000;   // GPIOF 9 : Output speed 25MHZ Medium speed 
}   

/* EXTI (EXTI8(GPIOH.8, SW0), EXTI9(GPIOH.9, SW1)) 초기 설정  */
void _EXTI_Init(void)
{
   RCC->AHB1ENR    |= 0x00000080;   // RCC_AHB1ENR GPIOH Enable
   RCC->APB2ENR    |= 0x00004000;   // Enable System Configuration Controller Clock
   
   GPIOH->MODER    &= ~0xC0030000;   // GPIOH PIN8,PIN15 Input mode (reset state)             
   
   SYSCFG->EXTICR[2] |= 0x0007;   // EXTI8에 대한 소스 입력은 GPIOH로 설정
               // EXTI8 <- PH8,
               // EXTICR3(EXTICR[2])를 이용 
               // reset value: 0x0000   
   SYSCFG->EXTICR[3] |= 0x7000;//15

   EXTI->FTSR |= 0x008100;      // EXTI8: Falling Trigger Enable 
   EXTI->IMR  |= 0x008100;      // EXTI8,9 인터럽트 mask (Interrupt Enable) 설정
      
   NVIC->ISER[0] |= (1<<23);   // Enable 'Global Interrupt EXTI8,9' == (1<<23)
  NVIC->ISER[1] |= (1<<(40-32));  // 40 == (1<<(40-32))
}

/* EXTI5~9 인터럽트 핸들러(ISR: Interrupt Service Routine) */
void EXTI9_5_IRQHandler(void)   // SW0
{      
   if(EXTI->PR & 0x0100)         // EXTI8 Interrupt Pending(발생) 여부? -> 마스크를 씌운다.
   {
      EXTI->PR |= 0x0100;      // Pending bit Clear (clear를 안하면 인터럽트 수행후 다시 인터럽트 발생)
      EXTI->IMR &= ~0x000100;   // EXTI8번에 마스킹을 하여 중복으로 발생된 인터럽트 제거
      GPIOG->ODR &= ~0xFF;      // LED OFF
      GPIOG->ODR |= 0x01;           // 1번 LED ON
      _mode = 1;                            // 모드를 1로 변경
      BEEPS(1);                             // 부저 1회
      EXTI->IMR |= 0x000100;            // EXTI8 마스크 해제
   }
}

/* EXTI10~15 인터럽트 핸들러(ISR: Interrupt Service Routine) */
void EXTI15_10_IRQHandler(void)     //SW7
{      
   if(EXTI->PR & 0x8000)         // EXTI15 Interrupt Pending(발생) 여부? -> 마스크를 씌운다.
   {
     EXTI->PR |= 0x8000;        // Pending bit Clear (clear를 안하면 인터럽트 수행후 다시 인터럽트 발생)
     if (_mode == 1) {                  // 모드 1에서만 작동하도록 함.
      EXTI->IMR &= ~0x8000;         // EXTI15번에 마스킹을 하여 중복으로 발생된 인터럽트 제거
      LCD_SetTextColor(RGB_RED);    // 글자색을 빨간색으로
      LCD_DisplayText(2, 9, "HD");          // HD표기-엘레베이터 중단모드 표기
      GPIOG->ODR |= 0x40;                   // LED6 ON
      GPIOG->ODR &= ~0x01;              // LED1 OFF
      BEEPS(10);                                // 부저 10회 + 딜레이 0.5초 x 9
      DelayMS(500);                             // 딜레이 0.5초  
      GPIOG->ODR &= ~0x40;   // LED6 OFF
      GPIOG->ODR |= 0x01;   // LED1 ON
      LCD_DisplayText(2, 9, "EX");          // EX표기 - 엘레베이터 실행모드 표기
      EXTI->IMR |= 0x8000;                  // EXTI15 마스크 해제
     }
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
         DelayMS(20);
          return key;
      }
   }
}

/* Buzzer: Beep for 30 ms */
void BEEPS(int num)         
{  
  for (int i = 0; i < num; i++) {   // 매개변수로 입력되 횟수만큼 반복
   GPIOF->ODR |=  0x0200;   // PF9 'H' Buzzer on
   DelayMS(30);      // Delay 30 ms
   GPIOF->ODR &= ~0x0200;   // PF9 'L' Buzzer off
   if (i+1 < num) DelayMS(500);     // 마지막횟수를 제외하고 부저를 울린후 0.5초 딜레이
  }
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

/* 엘레베이터를 target에 저장된층으로 이동시키는 함수*/
void Move(Elevator* ele, int target) {          
  while ((ele[_select].pressent) < target) {    // 선택된 엘레베이터의 현재층이 목표층보다 낮은 경우
    UpdateDisplay(ele, 'U');                            // 엘레베이터가 위로 이동중임을 표기
    DelayMS(500);                                       // 0.5초 대기
    ele[_select].pressent++;                        // 엘레베이터 상승
    UpdateDisplay(ele, 'U');                            // 이동된 엘레베이터 표기
  }
  while ((ele[_select].pressent) > target) {   // 선택된 엘레베이터의 현재층이 목표층보다 높은 경우
    UpdateDisplay(ele, 'D');                        // 엘렙에티어가 아래로 이동중임을 표기
    DelayMS(500);                                   //0.5초 대기
    ele[_select].pressent--;                        // 엘레베이터 하강
    UpdateDisplay(ele, 'D');                            // 이동된 엘레베이터 표시
  }
  DelayMS(500);                                         // 0.5초 대기
  UpdateDisplay(ele, 'S');                              // 엘레베이터가 정지되었음을 표기
}