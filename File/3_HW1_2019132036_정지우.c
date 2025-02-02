/////////////////////////////////////////////////////////////
// HW1: 세계 디지털 시계 제작
// 제출자 : 2019132036 정지우
// 주요 내용 및 구현 방법
// - 세계 주요 도시의 시계를 표시
// - 출력되는 각 도시의 시계를 변경 가능
// -  지역 시간(인천)       : TIM2의 Down-Counting mode(100msec) 이용
// -  지역 시간 (시카고)   : TIM6의 UP-Counting mode(100msec) 이용
// -  지역 시간 (런던)      : TIM9의 Up-Counting, OC-Compare mode(100msec) 이용
// -  지역 시간 reset       : EXTI15를 이용하여 선택된 도시가 인천인 경우에만 인천을 00:00으로 초기화, 인천을 제외한 각 도시는 인천과의 시차를 고려하여 초기화
//- 지역 시간 증가 속도 변경 : EXTI14를 이용하여 선택된 도시가 인천인 경우에만 시계의 속도를 100ms에서 10ms 혹은 10ms에서 100ms로 변경
//                                          TIM2, TIM6, TIM9의 ARR을 변경하여 Timer의 인터럽트 주기를 변경
/////////////////////////////////////////////////////////////
#include "stm32f4xx.h"
#include "GLCD.h"

#define SW0_PUSH        0xFE00  //PH8
#define SW1_PUSH        0xFD00  //PH9
#define SW2_PUSH        0xFB00  //PH10
#define SW3_PUSH        0xF700  //PH11
#define SW4_PUSH        0xEF00  //PH12
#define SW5_PUSH        0xDF00  //PH13
#define SW6_PUSH        0xBF00  //PH14
#define SW7_PUSH        0x7F00  //PH15

typedef struct Time{ // City Time struct
  uint8_t hour;
  uint8_t min;
} Time;

void _GPIO_Init(void);
void _EXTI_Init(void);
uint16_t KEY_Scan(void);
void TIMER2_Init(void);
void TIMER9_OC_Init(void);
void TIMER6_Init(void);

void DisplayInitScreen(void);
void DisplayICN(void);  // Incheon clock dispaly
void DisplayORD(void);  // Chicago clock dispaly
void DisplayLHR(void);  // London clock dispaly

void TimeReset(void); // Initializing the Time of Cities Based on Incheon

void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);
void BEEP(void);

Time ICN = {23,50}; // Set Incheon time to 23::50
Time ORD = {9,50};  // Set Chicago time to 9::50
Time LHR = {15,50}; // Set London time to 15::50
char city = 'I'; // Initialize the city to be displayed to Incheon
uint8_t period = 0; // 0:100ms 1: 10ms

int main(void)
{
	_GPIO_Init();
	_EXTI_Init();
	LCD_Init();	
	DelayMS(10);
    
	//BEEP();
   	DisplayInitScreen();	// LCD 초기화면
	GPIOG->ODR &= 0xFF00;// 초기값: LED0~7 Off
    
	TIMER2_Init();	    // TIM2 Init (Down Counting mode)                  
    TIMER6_Init();      // TIM6 Init (UP Counting mode)
    TIMER9_OC_Init();   // TIM9 Init (Output Compare mode)
    
	while(1)
	{
		// SW0~2 Change City
		switch(KEY_Scan())
		{
			case SW0_PUSH  : 	//SW0
                city = 'I';     // Display 'ICN'
 			break;

			case SW1_PUSH  : 	//SW1
                city = 'O';     // Display 'ORD'
			break;

            case SW2_PUSH  : 	//SW2
                city = 'L';     // DIsplay 'LHR'
			break;	
        }
	}
}

void TIMER2_Init(void)
{
// Time base 설정
	RCC->APB1ENR |= (1<<0);	// 0x01, RCC_APB1ENR TIMER2 Enable

	// Setting CR1 : 0x0000 
	TIM2->CR1 |= (1<<4);	// DIR=1(Down counter)(reset state)
	TIM2->CR1 &= ~(1<<1);	// UDIS=0(Update event Enabled): By one of following events
                            //  Counter Overflow/Underflow, 
                            //  Setting the UG bit Set,
                            //  Update Generation through the slave mode controller 
                            // UDIS=1 : Only Update event Enabled by  Counter Overflow/Underflow,
	TIM2->CR1 &= ~(1<<2);	// URS=0(Update event source Selection): one of following events
                            //	Counter Overflow/Underflow, 
                            // Setting the UG bit Set,
                            //	Update Generation through the slave mode controller 
                            // URS=1 : Only Update Interrupt generated  By  Counter Overflow/Underflow,
	TIM2->CR1 &= ~(1<<3);	// OPM=0(The counter is NOT stopped at update event) (reset state)
	TIM2->CR1 |= (1<<7);	// ARPE=1(ARR is buffered): ARR Preload Enalbe
    TIM2->CR1 &= ~(3<<8); 	// CKD(Clock division)=00(reset state)
	TIM2->CR1 &= ~(3<<5); 	// CMS(Center-aligned mode Sel)=00 : Edge-aligned mode(reset state)

	// Setting the Period
	TIM2->PSC = 42;	// Prescaler=42, 84MHz/42 = 2MHz (0.5us)
	TIM2->ARR = 200000;	// Auto reload  : 0.5us * 200K = 100ms(period) : 인터럽트주기나 출력신호의 주기 결정

	// Update(Clear) the Counter
	TIM2->EGR |= (1<<0);    // UG: Update generation    

	TIM2->DIER |= (1<<0);	// UIE: Enable Tim2 Update interrupt

	NVIC->ISER[0] 	|= (1<<28);	// Enable Timer2 global Interrupt on NVIC

	TIM2->CR1 |= (1<<0);	// CEN: Enable the Tim2 Counter  					
}

void TIMER6_Init(void)
{
// Time base 설정
	RCC->APB1ENR |= (1<<4);	// 0x10, RCC_APB1ENR TIMER6 Enable

	// Setting CR1 : 0x0000 
	TIM6->CR1 &= ~(1<<4);	// DIR=0(Up counter)(reset state)
	TIM6->CR1 &= ~(1<<1);	// UDIS=0(Update event Enabled): By one of following events
                            //  Counter Overflow/Underflow, 
                            //  Setting the UG bit Set,
                            //  Update Generation through the slave mode controller 
                            // UDIS=1 : Only Update event Enabled by  Counter Overflow/Underflow,
	TIM6->CR1 &= ~(1<<2);	// URS=0(Update event source Selection): one of following events
                            //	Counter Overflow/Underflow, 
                            // Setting the UG bit Set,
                            //	Update Generation through the slave mode controller 
                            // URS=1 : Only Update Interrupt generated  By  Counter Overflow/Underflow,
	TIM6->CR1 &= ~(1<<3);	// OPM=0(The counter is NOT stopped at update event) (reset state)
	TIM6->CR1 |= (1<<7);	// ARPE=1(ARR is buffered): ARR Preload Enalbe
    TIM6->CR1 &= ~(3<<8); 	// CKD(Clock division)=00(reset state)
	TIM6->CR1 &= ~(3<<5); 	// CMS(Center-aligned mode Sel)=00 : Edge-aligned mode(reset state)

	// Setting the Period
	TIM6->PSC = 1680-1;	// Prescaler=1680, 84MHz/1680 = 50KHz (20us)
	TIM6->ARR = 5000-1;	// Auto reload  : 20us * 5K = 100ms(period) : 인터럽트주기나 출력신호의 주기 결정

	// Update(Clear) the Counter
	TIM6->EGR |= (1<<0);    // UG: Update generation    

	TIM6->DIER |= (1<<0);	// UIE: Enable Tim6 Update interrupt

	NVIC->ISER[1] 	|= (1<<(54-32));	// Enable Timer6 global Interrupt on NVIC

	TIM6->CR1 |= (1<<0);	// CEN: Enable the Tim6 Counter  					
}

void TIMER9_OC_Init(void)
{
// Time base 설정
	RCC->APB2ENR |= (1<<16);	// 0x04, RCC_APB1ENR TIMER4 Enable

	// Setting CR1 : 0x0000 
	TIM9->CR1 &= ~(1<<4);	// DIR=0(Up counter)(reset state)
	TIM9->CR1 &= ~(1<<1);	// UDIS=0(Update event Enabled): By one of following events
                            //  Counter Overflow/Underflow, 
                            //  Setting the UG bit Set,
                            //  Update Generation through the slave mode controller 
                            // UDIS=1 : Only Update event Enabled by  Counter Overflow/Underflow,
	TIM9->CR1 &= ~(1<<2);	// URS=0(Update event source Selection): one of following events
                            //	Counter Overflow/Underflow, 
                            // Setting the UG bit Set,
                            //	Update Generation through the slave mode controller 
                            // URS=1 : Only Update Interrupt generated  By  Counter Overflow/Underflow,
	TIM9->CR1 &= ~(1<<3);	// OPM=0(The counter is NOT stopped at update event) (reset state)
	TIM9->CR1 |= (1<<7);	// ARPE=1(ARR is buffered): ARR Preload Enalbe 
	TIM9->CR1 &= ~(3<<8); 	// CKD(Clock division)=00(reset state)
	TIM9->CR1 &= ~(3<<5); 	// CMS(Center-aligned mode Sel)=00 : Edge-aligned mode(reset state)

	// Setting the Period
	TIM9->PSC = 16800-1;	// Prescaler=16800, 168MHz/16800 = 10KHz (0.1ms)
	TIM9->ARR = 1000-1;	// Auto reload  : 0.1ms * 1K = 100ms(period) : 인터럽트주기나 출력신호의 주기 결정

	// Update(Clear) the Counter
	TIM9->EGR |= (1<<0);    // UG: Update generation    

    // Output Compare 설정
	// CCMR1(Capture/Compare Mode Register 1) : Setting the MODE of Ch1 or Ch2
	TIM9->CCMR1 &= ~(3<<0); // CC1S(CC1 channel) = '0b00' : Output 
	TIM9->CCMR1 &= ~(1<<2); // OC1FE=0: Output Compare 1 Fast disable 
	TIM9->CCMR1 &= ~(1<<3); // OC1PE=0: Output Compare 1 preload disable(CCR1에 언제든지 새로운 값을 loading 가능) 
	TIM9->CCMR1 |= (3<<4);	// OC1M=0b011 (Output Compare 1 Mode : toggle)
				// OC1REF toggles when CNT = CCR1
				
	// CCER(Capture/Compare Enable Register) : Enable "Channel 1" 
	TIM9->CCER &= ~(1<<0);	// CC1E=1: CC1 channel Output Enable
				// OC1(TIM9_CH1) Active: 해당핀(100번)을 통해 신호출력
	TIM9->CCER &= ~(1<<1);	// CC1P=0: CC1 channel Output Polarity (OCPolarity_High : OC1으로 반전없이 출력)  

	// CC1I(CC 인터럽트) 인터럽트 발생시각 또는 신호변화(토글)시기 결정: 신호의 위상(phase) 결정
	// 인터럽트 발생시간(10000 펄스)의 10%(1000) 시각에서 compare match 발생
	TIM9->CCR1 = 50;	// TIM9 CCR1 TIM9_Pulse

	TIM9->DIER |= (1<<0);	// UIE: Enable Tim9 Update interrupt
	TIM9->DIER |= (1<<1);	// CC1IE: Enable the Tim9 CC1 interrupt

	NVIC->ISER[0] 	|= (1<<24);	// Enable Timer9 global Interrupt on NVIC

	TIM9->CR1 |= (1<<0);	// CEN: Enable the Tim9 Counter  					
}

void TIM2_IRQHandler(void)      //RESET: 0
{
    TIM2->SR &= ~(1<<0);	// Interrupt flag Clear
    
    ICN.min++;// 분 증가
    if (ICN.hour == 23) { // 시간이 23시 인 경우
      if (ICN.min > 59) { // 분이 59초과 이면
        ICN.hour = 0; // 시간 초기화
        ICN.min = 0; // 분 초기화
      }
    } else { // 시간이 23시가 아닌 경우
      if (ICN.min > 59) { // 분이 59초과 이면
      ICN.min = 0; // 분 초기화
      ICN.hour++; // 시간 증가
      }
    }
    if (city == 'I')    // 선택된 도시가 인천이면 시간 출력
      DisplayICN();
}

void TIM6_DAC_IRQHandler(void)
{
  TIM6->SR &= ~(1<<0);	// Interrupt flag Clear
  
    ORD.min++;                  //  분 증가
    if (ORD.hour == 23) {   //  시간아 23시인 경우
      if (ORD.min > 59) {   //  분이  59초과 이면
        ORD.hour = 0;       // 시간 초기화
        ORD.min = 0;        // 분 초기화
      }
    } else {                //  시간이 23시가 아닌 경우
      if (ORD.min > 59) {   // 분이 59초과인 경우
      ORD.min = 0; // 분 초기화
      ORD.hour++; // 시간 증가
      }
    }
    if (city == 'O')    // 선택된 도시가 시카고 인경우 시간 출력
      DisplayORD();
}

void TIM1_BRK_TIM9_IRQHandler(void)      //RESET: 0
{
	if ((TIM9->SR & 0x01) != RESET)	// Update interrupt flag
		TIM9->SR &= ~(1<<0);	// Update Interrupt Claer
    
	if((TIM9->SR & 0x02) != RESET)	// Capture/Compare 1 interrupt flag
	{
		TIM9->SR &= ~(1<<1);	// CC 1 Interrupt Claer
        
        LHR.min++; // 분 증가
        if (LHR.hour == 23) {// 시간이 23시인 경우
          if (LHR.min > 59) { // 분이 59 초과이면
            LHR.hour = 0;       // 시간 초기화
            LHR.min = 0;    // 분 초기화
          }
        }
        else {              // 시간이 23시가 아닌 경우
          if (LHR.min > 59) {   // 분이 59 초과인 경우
            LHR.min = 0;    // 분 초기화
            LHR.hour++; // 시간 증가
          }   
        }
        if (city == 'L') // 선택된 도시가 런던인 경우 시간 출력
          DisplayLHR();
	}
}

void _GPIO_Init(void)
{
	// LED (GPIO G) 설정
    RCC->AHB1ENR	|=  0x00000040;	// RCC_AHB1ENR : GPIOG(bit#6) Enable							
	GPIOG->MODER 	|=  0x00005555;	// GPIOG 0~7 : Output mode (0b01)						
	GPIOG->OTYPER	&= ~0x00FF;	// GPIOG 0~7 : Push-pull  (GP8~15:reset state)	
 	GPIOG->OSPEEDR 	|=  0x00005555;	// GPIOG 0~7 : Output speed 25MHZ Medium speed 
    
	// SW (GPIO H) 설정 
	RCC->AHB1ENR    |=  0x00000080;	// RCC_AHB1ENR : GPIOH(bit#7) Enable							
	GPIOH->MODER 	&= ~0xFFFF0000;	// GPIOH 8~15 : Input mode (reset state)				
	GPIOH->PUPDR 	&= ~0xFFFF0000;	// GPIOH 8~15 : Floating input (No Pull-up, pull-down) :reset state

	// Buzzer (GPIO F) 설정 
    RCC->AHB1ENR	|=  0x00000020; // RCC_AHB1ENR : GPIOF(bit#5) Enable							
	GPIOF->MODER 	|=  0x00040000;	// GPIOF 9 : Output mode (0b01)						
	GPIOF->OTYPER 	&= ~0x0200;	// GPIOF 9 : Push-pull  	
 	GPIOF->OSPEEDR 	|=  0x00040000;	// GPIOF 9 : Output speed 25MHZ Medium speed 
}	

void _EXTI_Init(void)
{
    RCC->AHB1ENR 	|= 0x0080;	// RCC_AHB1ENR GPIOH Enable
	RCC->APB2ENR 	|= 0x4000;	// Enable System Configuration Controller Clock
	
	GPIOH->MODER 	&= 0x0000FFFF;	// GPIOH PIN8~PIN15 Input mode (reset state)				 
	
	SYSCFG->EXTICR[3] |= 0x7700; 	// EXTI4,15에 대한 소스 입력은 GPIOH로 설정 (EXTICR3) (reset value: 0x0000)	
	
	EXTI->FTSR |= 0x00C000;		// Falling Trigger Enable  (EXTI14:PH14, EXTI15:PH15)
    EXTI->RTSR &= ~0x00C000;		// Rising Trigger  Enable  (EXTI14:PH14, EXTI15:PH15) 
    EXTI->IMR  |= 0x00C000;  	// EXTI14,15 인터럽트 mask (Interrupt Enable)
		
	NVIC->ISER[1] |= (1<<(40-32));   	// Enable Interrupt EXTI14,15 Vector table Position 참조
}

void EXTI15_10_IRQHandler(void)		// EXTI 10~15 인터럽트 핸들러
{
	if(EXTI->PR & 0x4000) 		// EXTI14 nterrupt Pending?
	{
		EXTI->PR |= 0x4000; 	// Pending bit Clear
        if (city == 'I') {      // 인천일 때만 period변경 가능
          period++;
          if (period > 1) period = 0;
          
          TIM2->CR1 &= ~(1<<0); // Timer2 clock disable
          TIM6->CR1 &= ~(1<<0); // Timer6 clock disable
          TIM9->CR1 &= ~(1<<0); // Timer7 clock disable
          
          if (period == 0) {
            TIM2->ARR = 200000; // Auto reload  : 0.5us * 200K = 100ms(period) : 인터럽트주기나 출력신호의 주기 결정
            TIM6->ARR = 5000-1; // Auto reload  : 20us * 5K = 100ms(period) : 인터럽트주기나 출력신호의 주기 결정
            TIM9->ARR = 1000-1; // Auto reload  : 0.1ms * 1K = 100ms(period) : 인터럽트주기나 출력신호의 주기 결정
            LCD_DisplayText(2,11, "100"); // Display period 100
          }
          else {
            TIM2->ARR = 20000;  // Auto reload  : 0.5us * 20K = 10ms(period) : 인터럽트주기나 출력신호의 주기 결정
            TIM6->ARR = 500-1;  // Auto reload  : 20us * 500 = 10ms(period) : 인터럽트주기나 출력신호의 주기 결정
            TIM9->ARR = 100-1;  // Auto reload  : 0.1ms * 100 = 10ms(period) : 인터럽트주기나 출력신호의 주기 결정
            LCD_DisplayText(2,11, " 10");// Display period 10
          }
          
          TIM2->CR1 |= (1<<0); // Timer2 clock enable
          TIM6->CR1 |= (1<<0); // Timer6 clock enable
          TIM9->CR1 |= (1<<0); // Timer7 clock enable
        }
        
	}
    
	if(EXTI->PR & 0x8000) 	// EXTI15 Interrupt Pending?
	{
        EXTI->PR |= 0x8000; 	// Pending bit Clear
        if (city == 'I') {      // 인천일 때만 reset가능
          TIM2->CR1 &= ~(1<<0); // Timer2 clock disable
          TIM6->CR1 &= ~(1<<0); // Timer6 clock disable
          TIM9->CR1 &= ~(1<<0); // Timer7 clock disable
          
          GPIOG->ODR |= 0x80; // LED7 On
          BEEP();
          DelayMS(3000);
          GPIOG->ODR &= ~0x80;  // LED7 Off
          
          TIM2->CNT = 0;    // Tim2 Clear
          TIM6->CNT = 0;    // Tim6 clear
          TIM9->CNT = 0;    // Tim9 Clear
          
          TimeReset();      // City Time Reset
          DisplayICN();     // Display Incheon time
          
          TIM2->CR1 |= (1<<0); // Timer2 clock enable
          TIM6->CR1 |= (1<<0); // Timer6 clock enable
          TIM9->CR1 |= (1<<0); // Timer7 clock enable
        }
	}
}

void BEEP(void)			// Beep for 20 ms 
{ 	GPIOF->ODR |= 0x0200;	// PF9 'H' Buzzer on
	DelayMS(20);		// Delay 20 ms
	GPIOF->ODR &= ~0x0200;	// PF9 'L' Buzzer off
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

void DisplayInitScreen(void)
{
	LCD_Clear(RGB_YELLOW);		// 화면 클리어
	LCD_SetFont(&Gulim8);		// 폰트 : 굴림 8
	LCD_SetBackColor(RGB_YELLOW);	// 글자배경색 : Yellow
	LCD_SetTextColor(RGB_BLUE);	// 글자색 : Blue

	LCD_DisplayText(0,0,"JJW 2019132036");  // Name, StudentID

	LCD_DisplayText(2,0,"Int period    ms");
    
    LCD_SetTextColor(RGB_BLACK);	// 글자색 : Black
    LCD_DisplayText(2,11, "100"); // Display period value 100
}

void DisplayICN(void) {
  LCD_DisplayText(1,0,"ICN"); // 인천 표시
  
  LCD_DisplayChar(1,4,0x30+ICN.hour/10);   // 인천 시간의  10의 자리 출력
  LCD_DisplayChar(1,5,0x30+ICN.hour%10);    // 인천 시간의 1의 자리 출력
  LCD_DisplayChar(1,6,':');
  LCD_DisplayChar(1,7,0x30+ICN.min/10);     // 인천 분의 10의 자리 출력
  LCD_DisplayChar(1,8,0x30+ICN.min%10);     // 인천 분의 1의 자리 출력
}

void DisplayORD(void) {
  LCD_DisplayText(1,0,"ORD");           // 시카고 표시
  
  LCD_DisplayChar(1,4,0x30+ORD.hour/10);    // 시카고 시간의 10의 자리 출력
  LCD_DisplayChar(1,5,0x30+ORD.hour%10);    // 시카고 시가느이 1의 자리 출력
  LCD_DisplayChar(1,6,':');
  LCD_DisplayChar(1,7,0x30+ORD.min/10);     // 시키고 분의 10의 자리 출력
  LCD_DisplayChar(1,8,0x30+ORD.min%10);     // 시카고 분의 1의 자리 출력
}

void DisplayLHR(void) {
  LCD_DisplayText(1,0,"LHR");               // 런던 표시
  
  LCD_DisplayChar(1,4,0x30+LHR.hour/10);    // 런던 시간의 10의 자리 출력
  LCD_DisplayChar(1,5,0x30+LHR.hour%10);    // 런던 시간의 1의 자리 출력
  LCD_DisplayChar(1,6,':');
  LCD_DisplayChar(1,7,0x30+LHR.min/10);     // 런던 분의 10의 자리 출력
  LCD_DisplayChar(1,8,0x30+LHR.min%10);     // 런던 분의 1의 자리 출력
}

void TimeReset(void) { // 각 도시의 시간 초기화
    ICN.hour = 0;
    ICN.min = 0;
    ORD.hour = 10;
    ORD.min = 0;
    LHR.hour = 16;
    LHR.min = 0;
}


uint8_t key_flag = 0;
uint16_t KEY_Scan(void)	// input key SW0 - SW7 
{ 
	uint16_t key;
	key = GPIOH->IDR & 0xFF00;	// any key pressed ?
	if(key == 0xFF00)		// if no key, check key off
	{  	if(key_flag == 0)
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
		{   	key_flag = 1;		
			DelayMS(10);
			return key;
		}
	}
}
