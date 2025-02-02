/////////////////////////////////////////////////////////////
// 과제명: 엘레베이터
// 과제개요: SW로 층수를 입력받아 해당 층으로 이동을 LED로 표현하며, SW7으로 긴급정지를 한 후 5초 후에 제개한다.
// 사용한  하드웨어(기능): GPIO, SW, LED, EXTI, GLCD
// 제출일: 2023. 5. 30
// 제출자  클래스: 화요일반
//           학번: 2019132036
//           이름: 정지우
///////////////////////////////////////////////////////////////
#include "stm32f4xx.h"
#include "GLCD.h"

void _GPIO_Init(void);
void _EXTI_Init(void);
uint16_t KEY_Scan(void);
void BEEP(void);
void DisplayInitScreen(void);
void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);

void UpdateDisplay(int CurFlow, int DesFlow);   // 디스플레이에 층에 대한 정보 표기 // 179
void UpdateLED(int CurFlow);                            // 현재층의 LED를 키는 함수// 199
int TargetFloor(void);                                          // 목표층을 0~6번 스위치를 통해 입력받아 반환하는 함수// 205
void BEEPS(int);                                                // 부저를 여러번 울리는 함수


int main(void)
{
	_GPIO_Init();		// GPIO (LED & SW) 초기화
    _EXTI_Init();       // 인터럽트 초기화
	LCD_Init();		    // LCD 모듈 초기화
	DelayMS(100);       // 안정화

	GPIOG->ODR |= 0x0001;	// LED 초기값: LED0~7 Off
	DisplayInitScreen();	// LCD 초기화면
    int DesFlow = -1;       // 목표층 -1로 초기화
    int CurFlow = 0;        // 현재층 0으로 초기화
    
	while(1)
	{
		DesFlow = TargetFloor();            // 스위치로 목표층을 입력받음 (입력이 없으면 -1)
        if (DesFlow == -1) continue;        // 입력 받지 않았다면 처음으로
        if (DesFlow == CurFlow) continue;   // 목표층과 현재층이 같으면 동작안함
        
        UpdateDisplay(CurFlow, DesFlow);    // 입력된 목표층을 업데이트
        DelayMS(1000);
        
        if (DesFlow > CurFlow) {                // 목표층이 현재층보다 높은 경우
          for (;CurFlow != DesFlow; CurFlow++){ // 목표층과 현재층이 같을 때까지 현재층을 1증가시키며 반복
            UpdateDisplay(CurFlow, DesFlow);    // 디스플레이의 층 정보 업데이트
            UpdateLED(CurFlow);                 // LED의 층 정보 업데이트
            DelayMS(1000);                      // 1초 대기
            }
        } else {                                // 목표층이 현재층보다 낮은 경우
          for (;CurFlow != DesFlow; CurFlow--){ // 목표층과 현재층이 같을 때까지 현재층을 1감소시키며 반복
            UpdateDisplay(CurFlow, DesFlow);    // 디스플레이의 층 정보 업데이트
            UpdateLED(CurFlow);                 // LED의 층 정보 업데이트
            DelayMS(1000);                      // 1초 대기
          }
        }
        DesFlow = -1;                           // 목표층을 -1로 초기화
        UpdateDisplay(CurFlow, DesFlow);        // 디스플레이의 층 정보 업데이트
        UpdateLED(CurFlow);                     // LED의 층 정보 업데이트
        
        BEEPS(3);                                       // 부저 3회
	}
}

/* GPIO (GPIOG(LED), GPIOH(Switch), GPIOF(Buzzer) 초기 설정	*/
void _GPIO_Init(void)
{
	// LED (GPIO G) 설정 : Output mode
	RCC->AHB1ENR	|=  0x00000040;	// RCC_AHB1ENR : GPIOG(bit#6) Enable							
	GPIOG->MODER 	|=  0x00005555;	// GPIOG 0~7 : Output mode (0b01)						
	GPIOG->OTYPER	&= ~0x00FF;	// GPIOG 0~7 : Push-pull  (GP8~15:reset state)	
	GPIOG->OSPEEDR 	|=  0x00005555;	// GPIOG 0~7 : Output speed 25MHZ Medium speed 
   
	// SW (GPIO H) 설정 : Input mode 
	RCC->AHB1ENR    |=  0x00000080;	// RCC_AHB1ENR : GPIOH(bit#7) Enable							
	//GPIOH->MODER 	&= ~0xFFFF0000;	// GPIOH 8~15 : Input mode (reset state)				
	GPIOH->PUPDR 	&= ~0xFFFF0000;	// GPIOH 8~15 : Floating input (No Pull-up, pull-down) :reset state

	// Buzzer (GPIO F) 설정 : Output mode
	RCC->AHB1ENR	|=  0x00000020;	// RCC_AHB1ENR : GPIOF(bit#5) Enable							
	GPIOF->MODER 	|=  0x00040000;	// GPIOF 9 : Output mode (0b01)						
	GPIOF->OTYPER 	&= ~0x0200;	// GPIOF 9 : Push-pull  	
	GPIOF->OSPEEDR 	|=  0x00040000;	// GPIOF 9 : Output speed 25MHZ Medium speed 
}	

void _EXTI_Init(void) {
    RCC->AHB1ENR    |= 0x00000080;  // RCC_AHB1ENR : GPIOH enable
    RCC->APB2ENR    |= 0x00004000;  // Enable System Configuration Controller Clock
    
    GPIOH->MODER    &= ~0xC000000;  // GPIOH 15 : Input Mode
    SYSCFG->EXTICR[3]   |= 0x7000;  // EXTI15에 대한 소스 입력은 GPIOH로 설정, EXTI15 <- PH15
    
    EXTI->FTSR  |= 0x008000;        // EXTI15 : Falling Trigger Enable
    EXTI->IMR   |= 0x008000;        // EXTI15 : 인터럽트 mask 설정
    
    NVIC->ISER[1]   |= 0x0100;      // Enable 'Global Interrupt EXTI15'
}

void EXTI15_10_IRQHandler() {   // 10~10에 인터럽트 발생
    if (EXTI->PR & 0x8000) {    // SW7 눌림
      EXTI->PR |= 0x8000;       // 핸들러가 다시 작동하지 않도록 초기화
      EXTI->IMR   &= ~0x008000;     // mask를 다시 씌어 연속으로 발생된 인터럽트 제거
      GPIOG->ODR |= 0x0080;     // 7번 LED ON
      BEEPS(2);                    //  부저 2회
      DelayMS(5000);            // 5초 대기
      BEEPS(2);                     // 부저 2회
      GPIOG->ODR &= ~0x0080;    // 7번 LED OFF
      EXTI->IMR   |= 0x008000;      // 다시 mask를 씌움
    }
}

/* Switch가 입력되었는지를 여부와 어떤 switch가 입력되었는지의 정보를 return하는 함수  */ 
uint8_t key_flag = 0;
uint16_t KEY_Scan(void)	// input key SW0 - SW7 
{ 
	uint16_t key;
	key = GPIOH->IDR & 0xFF00;	// any key pressed ?
	if(key == 0xFF00)		// if no key, check key off
	{	if(key_flag == 0)
			return key;
		else
		{	DelayMS(10);
			key_flag = 0;
			return key;
		}
	}
	else				// if key input, check continuous key
	{	if(key_flag != 0)	// if continuous key, treat as no key input
			return 0xFF00;
		else			// if new key,delay for debounce
		{	key_flag = 1;
			DelayMS(10);
 			return key;
		}
	}
}

/* Buzzer: Beep for 30 ms */
void BEEP(void)			
{ 	
	GPIOF->ODR |=  0x0200;	// PF9 'H' Buzzer on
	DelayMS(30);		// Delay 30 ms
	GPIOF->ODR &= ~0x0200;	// PF9 'L' Buzzer off
}

void DisplayInitScreen(void)
{
	LCD_Clear(RGB_YELLOW);		        // 화면 클리어
	LCD_SetFont(&Gulim8);		        // 폰트 : 굴림 8
	LCD_SetBackColor(RGB_YELLOW);	    // 글자배경색 : 노란색
	LCD_SetTextColor(RGB_BLACK);	    // 글자색 : Black
	LCD_DisplayText(0,0,"MECHA Elevator(JJW)");	// Title

	LCD_SetBackColor(RGB_YELLOW);	    //글자배경색 : Yellow
          
	LCD_DisplayText(1,0,"Cur FL: ");    // 둘째줄에 "Cur FL : "출력
	LCD_DisplayText(2,0,"Des FL: ");    // 셋째줄에 "Des FL : "출력
    
    LCD_SetTextColor(RGB_RED);	        // 글자색 : Red
    LCD_DisplayChar(1,8, '0');          // 현재 위치를 초기위치인 0으로 출력
	LCD_DisplayChar(2,8, '-');          // 목표 위치를 초기값인 -로 출력
}

void DelayMS(unsigned short wMS)
{
	register unsigned short i;
	for (i=0; i<wMS; i++)
		DelayUS(1000);	// 1000us => 1ms
}

void DelayUS(unsigned short wUS)
{
	volatile int Dly = (int)wUS*17;
	for(; Dly; Dly--);
}

void UpdateDisplay(int CurFlow, int DesFlow) { // 디스플레이에 층에 대한 정보 표기
    if (CurFlow < 0 || CurFlow  > 6) { // 잘못된 현재층이 입력되면 층 대신 ?표기후 함수 종료
      LCD_DisplayChar(1, 8, '?');
      return;
    }
    if (DesFlow < -1 || DesFlow  > 6) { // 잘못된 목표층이 입력되면 층 대신 ?표기후 함수 종료
      LCD_DisplayChar(2, 8, '?');
      return;
    }
    
    char Cflow = CurFlow + 0x30;    // 정수형인 현재층을 문자형으로 변환
    char Dflow;
    if (DesFlow == -1) Dflow = '-'; // 목표층이 -1이면(입력이 없음) 목표층 표시변수에 -저장
    else Dflow = DesFlow + 0x30;    // 목표층이 -1이 아니면(입력됨) 정수형인 목표층을 문자형으로 변환
    
    LCD_SetTextColor(RGB_RED);	    // 글자색 : Red
    LCD_DisplayChar(1,8,Cflow);     // 현재층 Display에 출력
	LCD_DisplayChar(2,8,Dflow);     // 목표층 Display에 출력
}

void UpdateLED(int CurFlow) {               // 현재층의 LED를 키는 함수
    if (CurFlow > 6 || CurFlow < 0) return; // 현재층이 작동시킬 수 있는 LED의 범위를 초과하면 종료
    GPIOG->ODR &= ~0x00FF;                  // 모든 LED OFF
    GPIOG->ODR |= (1<<CurFlow);             // 쉬프트 연산자를 통해 CurFlow위치의 LED ON
}

int TargetFloor(void) {     // 목표층을 0~6번 스위치를 통해 입력받아 반환하는 함수
    int DesFlow;            // 스위치를 통해 입력된 목표층을 저장할 변수
    switch(KEY_Scan())
		{
			case 0xFE00  : 	//SW0 -> 0층
                DesFlow = 0;    // 목표층에 0층 저장
				BEEP();
			break;
			case 0xFD00 : 	//SW1 -> 1층
                DesFlow = 1;    // 목표층에 1층 저장
				BEEP();
			break;
            case 0xFB00 : 	//SW2 -> 2층
				DesFlow = 2;    // 목표층에 2층 저장
                BEEP();
			break;
            case 0xF700 : 	//SW3 -> 3층
				DesFlow = 3;    // 목표층에 3층 저장
                BEEP();
			break;
            case 0xEF00 : 	//SW4 -> 4층
				DesFlow = 4;    // 목표층에 4층 저장
                BEEP();
			break;
            case 0xDF00 : 	//SW5 -> 5층
				DesFlow = 5;    // 목표층에 5층 저장
                BEEP();
			break;
            case 0xBF00 : 	//SW6 -> 6층
				DesFlow = 6;    // 목표층에 6층 저장
                BEEP();
			break;
            default:        // 스위치 미입력
                DesFlow = -1;   // 목표층 변수에 -1저장
		}
    return DesFlow;         // 목표층 반환
}

void BEEPS(int time) {              // 부저를 여러번 울리는 함수
  if (time < 1) return;                 // 부저를 울리는 횟수가 0이하이면 함수 종료
  for (int i = 0; i < time -1; i++) { // 부저를 time값보다 하나 적게 400ms간격으로 울린다
    BEEP();
    DelayMS(400);
  }
  BEEP();                                   // 부저를 한번 울린다. (마지막에 딜레이를 하지 않기 위함)
}