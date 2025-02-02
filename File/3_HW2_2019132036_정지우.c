/////////////////////////////////////////////////////////////
// HW2: STEP Motor �� DC Motor ���� ����̺��ȣ �߻�
// ������: 2019132036 ������
// �ֿ� ���� �� ���� ���
// -SW3 �Է��� TIM5_CH2 Original Counting mode�� ī���� �Ͽ� 
//    TIM1_CH3 OCmode�� ī���� ���� �ι�� �޽� ��� 
// -TIM4�� �̿��Ͽ� TIM5�� CNT���� ���÷��̷� ���
// -JoytStick PUSH �Է��� TIM8_CH1 Original Counting mode��
//   ī���� �Ͽ� TIM14_CH1 PWMmode�� ī���� ���� 10�踦 
//   Duty Ratio�� �ϴ� PWM�� ��� 
// -TIM3�� �̿��Ͽ� TIM8�� CNT���� ���÷��̷� ���
/////////////////////////////////////////////////////////////
#include "stm32f4xx.h"
#include "GLCD.h"

void TIMER4_Init(void);
void TIMER5_Count_Init(void);
void TIMER1_OC_Init(void);

void TIMER8_Count_Init(void);
void TIMER3_Init(void);
void TIMER14_PWM_Init(void);

void DisplayInitScreen(void);

void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);

UINT16 INT_COUNT;

int main(void)
{
	LCD_Init();	
	DelayMS(10);

   	DisplayInitScreen();	// LCD �ʱ�ȭ��

    TIMER5_Count_Init();  // TIM5  Original Count mode init
	TIMER4_Init();	         // TIM4 Up Counting mode Init                  
    TIMER1_OC_Init();     // TIM1 OC mode init
    
    TIMER8_Count_Init();    // TIM8 Original Count mode init
    TIMER3_Init();              // TIM3 Up Cointing mode init
    TIMER14_PWM_Init();     // TIM14 PWM mode init
    
	while(1) {	}
}

void TIMER5_Count_Init(void) //TIM5_CH2 CC 
{
	// PH11: TIM5_CH2
	// PH11�� ��¼����ϰ� Alternate function(TIM5_CH2)���� ��� ����
	RCC->AHB1ENR   |= (1<<7);   // RCC_AHB1ENR GPIOH Enable
	GPIOH->MODER   |= (2<<2*11);   // GPIOH PH11 Output Alternate function mode               
	GPIOH->OSPEEDR |= (3<<2*11);   // GPIOH PH11 Output speed (100MHz High speed)
	GPIOH ->PUPDR   &= ~(3<<2*11);   // GPIOH PH11 No Pull-up, Pull-down
	GPIOH->AFR[1]  |= (2<<4*(11-8)); // Connect TIM5 pins(PH11) to AF2(TIM5)
   
	// Timerbase Mode
	RCC->APB1ENR   |= (1<<3);// RCC_APB1ENR TIMER5 Enable

	TIM5->PSC = 1-1;   // Prescaler 0
	TIM5->ARR = 5-1;    // 4max
    
	// Setting CR1 
	TIM5->CR1 &= ~(1<<4);	// DIR=0(Up counter)(reset state)
	TIM5->CR1 &= ~(1<<1);	// UDIS=0(Update event Enabled)
	TIM5->CR1 &= ~(1<<2);	// URS=0(Update event source Selection)
	TIM5->CR1 &= ~(1<<3);	// OPM=0(The counter is NOT stopped at update event) (reset state)
	TIM5->CR1 &= ~(3<<8); 	// CKD(Clock division)=00(reset state)
	TIM5->CR1 &= ~(3<<5); 	// CMS(Center-aligned mode Sel)=00 (Edge-aligned mode) (reset state)
        
	// Update(Clear) the Counter
	TIM5->EGR |= (1<<0);    // UG=1, REG's Update (CNT clear) 

    // External Clock Mode 1
	// CCMR1(Capture/Compare Mode Register 1) : Setting the MODE of Ch1 or Ch2
	TIM5->CCMR1 |= (1<<8); 	// CC2S(CC2 channel) = '0b01' : Input 
	TIM5->CCMR1 &= ~(15<<12); // IC2F='0b0000: No Input Filter 
				
	// CCER(Capture/Compare Enable Register) : Enable "Channel 1" 
	TIM5->CCER &= ~(1<<4);	// CC2E=0: Capture Disable
    
	// TI1FP1 NonInverting / Falling Edge  
	TIM5->CCER |= (1<<5);	// CC2P=1
	TIM5->CCER &= ~(1<<7);	// CC2NP=0   

	// SMCR(Slave Mode Control Reg.) : External Clock Enable
	TIM5->SMCR |= (6<<4);	// TS(Trigger Selection)=0b110 :TI2FP2(Filtered Timer Input 2 ��½�ȣ)
	TIM5->SMCR |= (7<<0);	// SMS(Slave Mode Selection)=0b111 : External Clock Mode 1

	TIM5->CR1 |= (1<<0);	// CEN: Enable the Tim5 Counter  	
}

void TIMER4_Init(void)
{  
	RCC->APB1ENR 	|= (1<<2);	// 0x04, TIMER4 Enable 

    // Time base Mode
	// Setting CR1 : 0x0000 
	TIM4->CR1 &= ~(1<<4);	// DIR=0(Up counter)(reset state)
	TIM4->CR1 &= ~(1<<1);	// UDIS=0(Update event Enabled)
	TIM4->CR1 &= ~(1<<2);	// URS=0(Update event source Selection)
	TIM4->CR1 &= ~(1<<3);	// OPM=0(The counter is NOT stopped at update event) (reset state)
	TIM4->CR1 &= ~(1<<7);	// ARPE=0(ARR Preload Disable)
	TIM4->CR1 &= ~(3<<8); 	// CKD(Clock division)=00(reset state)
	TIM4->CR1 &= ~(3<<5); 	// CMS(Edge-aligned mode Sel)=00

	// PSC, ARR
	TIM4->PSC = 8400-1;	// Prescaler 84,000,000Hz/8400= 10000Hz (0.1ms)  (1~65536)
	TIM4->ARR = 1000-1;	// Auto reload  :  count�� ����: 0~1000 : 100ms
        
	// Update(Clear) the Counter
	TIM4->EGR |= (1<<0);    // UG=1, REG's Update (CNT clear) 
    
    NVIC->ISER[0] |= (1<<30); // TIMER4 overflow event interrupt
    TIM4->DIER |= (1<<0);        // CC2IE: Enable the Tim4 Update event interrupt

	TIM4->CR1 |= (1<<0);	// CEN: Enable the Tim4 Counter  	
}

void TIMER1_OC_Init(void) //TIM1_CH3
{
	// PE13: TIM1_CH3
	// PE13�� ��¼����ϰ� Alternate function(TIM1_CH3)���� ��� ����
	RCC->AHB1ENR   |= (1<<4);       // RCC_AHB1ENR GPIOE Enable
	GPIOE->MODER   |= (2<<2*13);   // GPIOE PE13 Output Alternate function mode               
	GPIOE->OSPEEDR |= (3<<2*13);   // GPIOE PE13 Output speed (100MHz High speed)
	GPIOE->OTYPER  = 0x00000000;   // GPIOE PE13 Output type push-pull (reset state)
	GPIOE->PUPDR   |= (1<<2*13);        // GPIOE PE13 Pull-up
	GPIOE->AFR[1]  |= (1<<4*(13-8)); // Connect TIM1 pins(PE13) to AF1(TIM1/2)
   
	// Timerbase Mode
	RCC->APB2ENR   |= 0x01;// RCC_APB1ENR TIMER1 Enable

	TIM1->PSC = 8400-1;   // Prescaler 168,000,000Hz/8400= 20kHz (50us)

	TIM1->ARR = 10000-1;    // �ֱ� = 50us * 10000 = 0.5s
    
	TIM1->CR1 &= ~(1<<4);   // DIR: Countermode = Upcounter (reset state)
	TIM1->CR1 &= ~(3<<8);   // CKD: Clock division = 1 (reset state)
	TIM1->CR1 |= (2<<5);    // CMS(Center-aligned mode Sel): Center-aligned mode 2

	TIM1->EGR |= (1<<0);    // UG: Update generation 
    
	// Output/Compare Mode
	TIM1->CCER &= ~(1<<4*(3-1));   // CC3E: OC3 Active 
	TIM1->CCER &= ~(1<<(4*(3-1)+1));  // CC3P: OCPolarity_Active HIGH

	TIM1->CCR3 = 5000;   // TIM1_Pulse

	TIM1->BDTR |= (1<<15);  // main output enable
   
	TIM1->CCMR2 &= ~(3<<8*0); // CC3S(CC3channel):   
	TIM1->CCMR2 &= ~(1<<3); // OC3PE: Output Compare 3 preload disable
	
    TIM1->CCMR2 |= (3<<4);   // OC3M: Output Compare 3 Mode : toggle

	TIM1->CR1 &= ~(1<<7);   // ARPE: Auto reload preload disable
	TIM1->DIER |= (1<<3);   // CC3IE: Enable the Tim1 CC3 interrupt
   
	NVIC->ISER[0] |= (1<<27); // TIM1_CC
	TIM1->CR1 &= ~(1<<0);   // CEN: Disable the Tim1 Counter 
}



void TIMER8_Count_Init(void) //TIM8_CH1 CC 
{
	// PI5: TIM8_CH1
	// PI5�� ��¼����ϰ� Alternate function(TIM8_CH1)���� ��� ����
      //Joy Stick SW(PORT I) ����
	RCC->AHB1ENR	    |=      (1<<8);	// RCC_AHB1ENR GPIOI Enable
	GPIOI->MODER	    |=      (2<<(2*5));	// GPIOI PI5 : Alternative function mode (reset state)
    GPIOI->OSPEEDR    |=      (3<<2*5);   // GPIOI PI5 Output speed (100MHz High speed)
	GPIOI->PUPDR	    &=   ~(3<<2*5);	// GPIOI PI5 : Floating input (No Pull-up, pull-down) (reset state)
    GPIOI->AFR[0]        |=      (3<<4*(5)); // Connect TIM8 pins(PI5) to AF3(TIM8)

	// Timerbase Mode
	RCC->APB2ENR   |= (1<<1);// RCC_APB2ENR TIMER8 Enable

	TIM8->PSC = 1-1;   // Prescaler 0
	TIM8->ARR = 8-1;    // 7max
    
	// Setting CR1 : 0x0000 
	TIM8->CR1 &= ~(1<<4);	// DIR=0(Up counter)(reset state)
	TIM8->CR1 &= ~(1<<1);	// UDIS=0(Update event Enabled)
	TIM8->CR1 &= ~(1<<2);	// URS=0(Update event source Selection)
	TIM8->CR1 &= ~(1<<3);	// OPM=0(The counter is NOT stopped at update event) (reset state)
	TIM8->CR1 &= ~(1<<7);	// ARPE=0(ARR Preload Disable)
	TIM8->CR1 &= ~(3<<8); 	// CKD(Clock division)=00(reset state)
	TIM8->CR1 |= (3<<5); 	// CMS(Center-aligned mode Sel)=11 (Center-aligned mode3)
        
	// Update(Clear) the Counter
	TIM8->EGR |=    (1<<0);    // UG=1, REG's Update (CNT clear) 

    // External Clock Mode 1
	// CCMR1(Capture/Compare Mode Register 1) : Setting the MODE of Ch1
	TIM8->CCMR1 |=    (1<<0); 	// CC1S(CC1 channel) = '0b01' : Input  TI1
	TIM8->CCMR1 &= ~(15<<4); // IC1F='0b0000: No Input Filter 
				
	// CCER(Capture/Compare Enable Register) : Enable "Channel 1" 
	TIM8->CCER &= ~(1<<0);	// CC1E=0: Capture Disable
	// TI1FP1 NonInverting / Rising Edge  
	TIM8->CCER &= ~(1<<1);	// CC1P=0
	TIM8->CCER &= ~(1<<3);	// CC1NP=0   

	// SMCR(Slave Mode Control Reg.) : External Clock Enable
	TIM8->SMCR |= (5<<4);	// TS(Trigger Selection)=0b101 :TI1FP1
	TIM8->SMCR |= (7<<0);	// SMS(Slave Mode Selection)=0b111 : External Clock Mode 1

	TIM8->CR1 |= (1<<0);	// CEN: Enable the Tim4 Counter  	
}

void TIMER3_Init(void)
{  
	RCC->APB1ENR 	|= (1<<1);	// 0x04, TIMER3 Enable 

    // Time base Mode
	// Setting CR1 : 0x0000 
	TIM3->CR1 &= ~(1<<4);	// DIR=0(Up counter)(reset state)
	TIM3->CR1 &= ~(1<<1);	// UDIS=0(Update event Enabled)
	TIM3->CR1 &= ~(1<<2);	// URS=0(Update event source Selection)
	TIM3->CR1 &= ~(1<<3);	// OPM=0(The counter is NOT stopped at update event) (reset state)
	TIM3->CR1 &= ~(1<<7);	// ARPE=0(ARR Preload Disable)
	TIM3->CR1 &= ~(3<<8); 	// CKD(Clock division)=00(reset state)
	TIM3->CR1 &= ~(3<<5); 	// CMS(Edge-aligned mode Sel)=00

	// PSC, ARR
	TIM3->PSC =  8400-1;	// Prescaler 84,000,000Hz/8400= 10000Hz (0.1ms)  (1~65536)
	TIM3->ARR = 200-1;	// Auto reload  :  count�� ����: 0~1000 : 20ms
        
	// Update(Clear) the Counter
	TIM3->EGR |= (1<<0);    // UG=1, REG's Update (CNT clear) 
    
    NVIC->ISER[0] |= (1<<29); // TIM3_CC
    TIM3->DIER |= (1<<0);       // TIM3_Update Event Interrupt

	TIM3->CR1 |= (1<<0);	// CEN: Enable the Tim3 Counter  	
}

void TIMER14_PWM_Init(void) //TIM14_CH1 PWM 
{
	// PF9: TIM14_CH1
	// PF9�� ��¼����ϰ� Alternate function(TIM14_CH1)���� ��� ����
	RCC->AHB1ENR   |= 0x00000020;   // RCC_AHB1ENR GPIOF Enable
	GPIOF->MODER   |= (2<<2*9);   // GPIOF PF9 Output Alternate function mode               
	GPIOF->OSPEEDR |= (3<<2*9);   // GPIOF PF9 Output speed (100MHz High speed)
	GPIOF->OTYPER  &= ~(1<<9);   // GPIOF PF9 Output type push-pull (reset state)
	GPIOF->AFR[1]  |= (9<<4*(9-8)); // Connect TIM14 pins(PF9) to AF9

	// Timerbase Mode
	RCC->APB1ENR   |= (1<<8);// RCC_APB2ENR TIMER14 Enable

	TIM14->PSC = 420-1;   // Prescaler 168,000,000Hz/420= 400kHz (2.5us)

	TIM14->ARR = 160-1;    // �ֱ� = 2.5us * 160 = 400us
    
    TIM14->CCR1 =160;   // TIM14_Pulse
    
    // Define the corresponding pin by 'Output'  
	// CCER(Capture/Compare Enable Register) : Enable "Channel 1" 
	TIM14->CCER	|= (1<<0);	// CC1E=1: OC1(TIM14_CH1) Active(Capture/Compare 1 output enable)
	TIM14->CCER	&= ~(1<<1);	// CC1P=0: CC1 output Polarity High (OC1���� �������� ���)
    
	// 'Mode' Selection : Output mode, PWM 1
	// CCMR1(Capture/Compare Mode Register 1) : Setting the MODE of Ch1
	TIM14->CCMR1 &= ~(3<<0); 	// CC1S(CC1 channel)='0b00' : Output 
    TIM14->CCMR1 |=    (1<<2); 	// CC1FE =1: Fastmode Enable 
	TIM14->CCMR1 |=    (1<<3); 	// OC1PE=1: Output Compare 1 preload Enable

	TIM14->CCMR1 |=    (7<<4);	// OC1M: Output compare 1 mode: PWM 2 mode
	TIM14->CCMR1 |=    (1<<7);	// OC1CE: Output compare 1 Clear enable

	// CR1 : Up counting & Counter TIM5 enable
	TIM14->CR1 	&= ~(1<<4);	// DIR: Countermode = Upcounter (reset state)
	TIM14->CR1 	&= ~(3<<8);	// CKD: Clock division = 1 (reset state)
	TIM14->CR1 	&= ~(3<<5); 	// CMS(Center-aligned mode Sel): No(reset state)
	TIM14->CR1	    |=    (1<<7);	// ARPE: Auto-reload preload enable
    
	TIM14->CR1	    |=    (1<<0);	// CEN: Counter TIM14 enable
}


void TIM3_IRQHandler(void) 
{
  TIM3->SR &= ~(1<<0);	// Interrupt flag Clear
    int num = TIM8->CNT % 10;
    LCD_DisplayChar(5,9,( num + 0x30));  // Joystick ���� Ƚ�� ���
    
    UINT16 Duty = (10-num) * 10; // ���� Ƚ�� Duty Ratio�� ��ȯ , PWM mode 2�̹Ƿ� Duty Ratio�� �������� ����
    
    TIM14->CCR1 = (UINT16)(160 * Duty / 100);   // Duty Ratio�� ���� TIM14 CCR1 ����
}

void TIM1_CC_IRQHandler(void)      //RESET: 0
{
	if ((TIM1->SR & 0x08) != RESET)	// CC3 interrupt flag 
	{
		TIM1->SR &= ~0x08;	// CC3 Interrupt Claer

		INT_COUNT++; // TIM1 interrupt�� �߻��� ������ INT_COUNT�� 1�����Ͽ� �� �� ����ߴ��� ����
		if (INT_COUNT >= TIM5->CNT*2 )  // ��� �޽��� ���� �Էµ� ���� 2�� ��ŭ ���
		{
 			TIM1->CCER &= ~(1<<4*(3-1));// CC3E Disable 
			TIM1->CR1 &= ~(1<<0); // TIM1 Disable
			INT_COUNT= 0; // �ʱ�ȭ
		}
	}
}

int last_CNT = 0; // ����  ī��Ʈ�� ����
void TIM4_IRQHandler(void)      //RESET: 0
{
    TIM4->SR &= ~(1<<0);	// Interrupt flag Clear
    
    int num = TIM5->CNT % 10;
    LCD_DisplayChar(3,9,( num + 0x30));  // switch3 ���� Ƚ�� ǥ��
    
    if (TIM5->CNT != 0)     // ���� Ƚ���� 0�� ��� ���� �޽��� �߻����� ����
      if (last_CNT != TIM5->CNT) { // �������� ��ȭ�� ����� ���
        TIM1->CCER |= (1<<4*(3-1));// CC3E Disable 
        TIM1->CR1 |= (1<<0); // TIM1 Disable
        last_CNT = TIM5->CNT; // �������� ���簪���� �ʱ�ȭ
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

void DisplayInitScreen(void)
{
	LCD_Clear(RGB_YELLOW);		// ȭ�� Ŭ����
	LCD_SetFont(&Gulim8);		// ��Ʈ : ���� 8
	LCD_SetBackColor(RGB_BLACK);	// ���ڹ��� : Black
	LCD_SetTextColor(RGB_WHITE);	// ���ڻ� : White
    
    LCD_DrawFillRect(0,0,120,25);              // Title space
	LCD_DisplayText(0,0,"Motor Control");  // Title
	LCD_DisplayText(1,0,"2019132036 JJW");  // Name
    
    LCD_SetBackColor(RGB_YELLOW);	// ���ڹ��� : Yellow
	LCD_SetTextColor(RGB_BLUE);	// ���ڻ� : Blue
	LCD_DisplayText(2,0,"Step Motor");  // Title
    LCD_DisplayText(4,0,"DC Motor");  // Title
    
    LCD_SetTextColor(RGB_BLACK);	// ���ڻ� : Black
	LCD_DisplayText(3,0," Postion:");  // Title
    LCD_DisplayText(5,0," Torque:");  // Title
    
    LCD_SetTextColor(RGB_RED);	// ���ڻ� : Red
}