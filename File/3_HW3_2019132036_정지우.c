//////////////////////////////////////////////////////////////////////
// HW3: USART 통신을 이용한 센서모니터링
// 제출자: 2019132036 정지우
// 
// 주요 내용 및 구현 방법
// TIM1_CH2에 의해 400ms 주기로 작동하는 ADC1을 이용하여 3개의 온도센서를
// 측정하여 데이터를 LCD에 표시
// PC와 USART로 연결되어 PC에서 MCU로 1, 2, 3을 보낼시 해당 숫자에 해당하는
// 센서의 데이터를 USART이용하여 PC로 전송
// 
// USART1: TX pin: PA9, RX pin: PA10 
// TX: Poliing 방식, RX: Interrupt 방식 
// 9600bps, 9data bit, Odd parity
// 센서 인덱스를 받으면 해당 센서의 현재값을 전송, 
// PC에서 보내온 인덱스를 화면에 표시
// 
// ADC1 : 3개의 온도 센서의 아날로그 입력 측정
// Sensor1 : ADC1_IN1(PA1), 키트 가변저항
// Sensor2 : ADC1_IN8(PB0), 키트 외부 가변저항
// Sensor3 : ADC1_IN16(IN16), MCU내부 온도 센서
// SCAN mode, DMA , TIM_CH2의 event에 동작
// 각 센서의 아날로그 입력을 측정하여, 온도 정보로 변환하여 화면에 출력
//
// TIMER1 : ADC1을 주기적으로 동작
// TIM1_CH2 OCmode를 사용하여 400ms주기로 CCevent를 발생시키고, ADC1을 동작
//////////////////////////////////////////////////////////////////////
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

typedef struct Temperature { // 각센서의 데이터를 저장
  uint16_t* ADC_Val;                   //  ADC value
  double Voltage;                           //  입력된 아날로그 전압
  uint16_t Temp;                          // 온도
}Temperature;

//*하드웨어 초기화 *//
// GPIO초기화
void _GPIO_Init(void);

// ADC 사용을 위한 초기화
void DMAInit(void);
void _ADC_Init(void);
void TIMER1_OC_Init(void);

// USART 초기화
void USART1_Init(void);
void USART_BRR_Configuration(uint32_t USART_BaudRate);


//* 사용자 함수 *//
// 디스플레이 표시  초기화
void DispayTitle(void);

// USART 데이터 전송
void SerialSendChar(uint8_t c);
void SerialSendString(char *s);

uint16_t KEY_Scan(void);
void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);
void BEEP(void);

//* 전역 변수 *//
uint16_t ADC_value[3];                                      // DMA를 이용한 ADC value 저장배열
Temperature Sensor1 = {&ADC_value[0], 0, 0};    // Sensor1 데이터 저장
Temperature Sensor2 = {&ADC_value[1], 0, 0};    // Sensor2 데이터 저장
Temperature Sensor3 = {&ADC_value[2], 0, 0};    // Sensor3 데이터 저장

//uint16_t ADC_Value, Voltage, Temp1;
int send_ch= 0x30;
int No_send_data;
char str[20];


int main(void)
{
	LCD_Init();	// LCD 구동 함수
	DelayMS(1000);	// LCD구동 딜레이
    DispayTitle();	//LCD 초기화면구동 함수
    DelayMS(10);
    
    // 하드웨어 초기화
	_GPIO_Init();
    _ADC_Init();
    DMAInit();
    TIMER1_OC_Init();
	USART1_Init();
    
	GPIOG->ODR &= 0x00;	// LED0~7 Off 
	BEEP();

	while(1) {    }
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

void _ADC_Init(void)
{	
    // ADC1: PA1(pin 41) 키트 가변저항
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;	// (1<<0) ENABLE GPIOA CLK (stm32f4xx.h 참조)
	GPIOA->MODER |= (3<<2*1);		                    // CONFIG GPIOA PIN1(PA1) TO ANALOG IN MODE
    
    // ADC1: PB0(pin 56) 외부 가변저항
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;	// (1<<0) ENABLE GPIOA CLK (stm32f4xx.h 참조)
	GPIOB->MODER |= (3<<2*0);		                    // CONFIG GPIOB PIN1(PB0) TO ANALOG IN MODE
						
	RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;	// (1<<8) ENABLE ADC1 CLK (stm32f4xx.h 참조)

	ADC->CCR &= ~(0X1F<<0);		                        // MULTI[4:0]: ADC_Mode_Independent  Only Use Channel1
	ADC->CCR |=  (1<<16); 		                            // 0x00010000 ADCPRE:ADC_Prescaler_Div4 (ADC MAX Clock 36MHz, 84Mhz(APB2)/4 = 10.5MHz)
       
	ADC1->CR1 &= ~(3<<24);		                            // RES[1:0]=0b00 : 12bit Resolution
    ADC1->CR1 |= (1<<8);		                               // SCAN=1 : ADC_ScanCovMode Enable

	ADC1->CR2 &= ~(1<<1);		// CONT=0: ADC_Continuous ConvMode Disable
    ADC1->CR2 |= (3<<28);		// EXTEN[1:0]=0b11: External trigger rising and falling edges
    ADC1->CR2 |=   (1<<24);     // EXTSEL=1: trigger at Timer1 CC2 event
	
	ADC1->CR2 &= ~(1<<11);		// ALIGN=0: ADC_DataAlign_Right
	ADC1->CR2 &= ~(1<<10);		// EOCS=0: The EOC bit is set at the end of each sequence of regular conversions

	ADC1->SQR1 |= (2<<20);	// L[3:0]=0b0010: ADC Regular channel sequece length 
					// 0b00010 3conversion)
    
    // Sensor 1 설정
	ADC1->SMPR2 |= (0x7<<(3*1));	// ADC1_CH1 Sample TIme_480Cycles (3*Channel_1)
	ADC1->SQR3 |= (1<<0);		// SQ1[4:0]=0b00001 : CH1
    
    // Sensor2 설정
    ADC1->SMPR2 |= (0x7<<(3*8));	// ADC1_CH8 Sample TIme_480Cycles (3*Channel_8)
    ADC1->SQR3 |= (8<<5);		// SQ1[4:0]=0b01000 : CH8
    
    // Sensor3 (MCU 내부 온도센서) 설정
    ADC1->SMPR1 |= (0x7<<(3*6));	// ADC1_CH16 Sample TIme_480Cycles (3*Channel_16)
    ADC1->SQR3 |= (16<<10);		// SQ1[4:0]=0b10000 : CH16
    ADC->CCR |= (1<<23);                // ADC TSVREFE :1 (V_REFINT Enable)
    
    // ADC interrupt설정
    ADC1->CR1 |=  (1<<5);		// EOCIE=1: Interrupt enable for EOC
    NVIC->ISER[0] |= (1<<18);	// Enable ADC global Interrupt
    
    // DMA설정
    ADC1->CR2 |= 0x00000200;	// DMA requests are issued as long as data are converted and DMA=1	
				// for single ADC mode
    ADC1->CR2 |= 0x00000100;	// DMA mode enabled  (DMA=1)
    
	ADC1->CR2 |= (1<<0);		// ADON: ADC ON
    ADC1->CR2 |= 0x40000000;   // SWSTART=1
}

void ADC_IRQHandler(void)
{
    // ADC가 완료되면 화면에 표시
	ADC1->SR &= ~(1<<1);		// EOC flag clear
    
    //*Seneor1*//
    // DMA를 통해 들어온 ADC값을 읽어 전압으로 저장
	Sensor1.Voltage = (*Sensor1.ADC_Val * 3.3) / 4095;   // 3.3 : 4095 =  Volatge : ADC_Value 
    //  전압값을 온도값으로 변경
    Sensor1.Temp = (int)(3.5*(Sensor1.Voltage*Sensor1.Voltage) + 1);
    
    // 센서 데이터 LCD에 표시
    LCD_DisplayChar(1,13,((int)Sensor1.Voltage)%10 + 0x30);
    LCD_DisplayChar(1,15,((int)(Sensor1.Voltage*10))%10 + 0x30);
    
    LCD_DisplayChar(1,9,(Sensor1.Temp/10) + 0x30);
    LCD_DisplayChar(1,10,(Sensor1.Temp%10) + 0x30);
    
    LCD_SetBrushColor(RGB_WHITE);
    LCD_DrawFillRect(0, 27, 160, 10);
    LCD_SetBrushColor(RGB_RED);
    LCD_DrawFillRect(10, 27, (int)((float)Sensor1.Temp/39*140), 10);
    
    //*Seneor2*//
    // DMA를 통해 들어온 ADC값을 읽어 전압으로 저장
    Sensor2.Voltage = (*Sensor2.ADC_Val * 3.3) / 4095;   // 3.3 : 4095 =  Volatge : ADC_Value 
    //  전압값을 온도값으로 변경
    Sensor2.Temp = (int)(3.5*(Sensor2.Voltage*Sensor2.Voltage) + 1);
    
    // 센서 데이터 LCD에 표시
    LCD_DisplayChar(3,13,((int)Sensor2.Voltage)%10 + 0x30);
    LCD_DisplayChar(3,15,((int)(Sensor2.Voltage*10))%10 + 0x30);
    
    LCD_DisplayChar(3,9,(Sensor2.Temp/10) + 0x30);
    LCD_DisplayChar(3,10,(Sensor2.Temp%10) + 0x30);
    
    LCD_SetBrushColor(RGB_WHITE);
    LCD_DrawFillRect(0, 53, 160, 10);
    LCD_SetBrushColor(RGB_GREEN);
    LCD_DrawFillRect(10, 53, (int)((float)Sensor2.Temp/39*140), 10);
    
    //*Seneor3*//
    // DMA를 통해 들어온 ADC값을 읽어 전압으로 저장
    Sensor3.Voltage = (*Sensor3.ADC_Val * 3.3) / 4095;   // 3.3 : 4095 =  Volatge : ADC_Value 
    //  전압값을 온도값으로 변경
    Sensor3.Temp = (int)(((Sensor3.Voltage-0.76)/2.5)+25);
    
    // 센서 데이터 LCD에 표시
    LCD_DisplayChar(5,13,((int)Sensor3.Voltage)%10 + 0x30);
    LCD_DisplayChar(5,15,((int)(Sensor3.Voltage*10))%10 + 0x30);
    
    LCD_DisplayChar(5,9,(Sensor3.Temp/10) + 0x30);
    LCD_DisplayChar(5,10,(Sensor3.Temp%10) + 0x30);
    
    LCD_SetBrushColor(RGB_WHITE);
    LCD_DrawFillRect(0, 79, 160, 10);
    LCD_SetBrushColor(RGB_BLUE);
    LCD_DrawFillRect(10, 79, (int)((float)Sensor3.Temp/39*140), 10);;
}

void DMAInit(void)
{
 	// DMA2 Stream0 channel0 configuration *************************************
	RCC->AHB1ENR |= (1<<22);		//DMA2 clock enable
	DMA2_Stream0->CR &= ~(7<<25);	//DMA2 Stream0 channel 0 selected

	// ADC1->DR(Peripheral) ==> ADC_vlaue(Memory)
	DMA2_Stream0->PAR |= (uint32_t)&ADC1->DR;	   //Peripheral address - ADC1->DR(Regular data) Address
	DMA2_Stream0->M0AR |= (uint32_t)&ADC_value; //Memory address - ADC_Value address 
	DMA2_Stream0->CR &= ~(3<<6);		  //Data transfer direction : Peripheral-to-memory (P=>M)
	DMA2_Stream0->NDTR = 3;			  //DMA_BufferSize = 3 (ADC_Value[3])

	DMA2_Stream0->CR &= ~(1<<9); 	//Peripheral increment mode  - Peripheral address pointer is fixed
	DMA2_Stream0->CR |= (1<<10);	//Memory increment mode - Memory address pointer is incremented after each data transferd 
	DMA2_Stream0->CR |= (1<<11);	//Peripheral data size - halfword(16bit)
	DMA2_Stream0->CR |= (1<<13);	//Memory data size - halfword(16bit)   
	DMA2_Stream0->CR |= (1<<8);	//Circular mode enabled   
	DMA2_Stream0->CR |= (2<<16);	//Priority level - High

	DMA2_Stream0->FCR &= ~(1<<2);	//DMA_FIFO_direct mode enabled
	DMA2_Stream0->FCR |= (1<<0);	//DMA_FIFO Threshold_HalfFull , Not used in direct mode

	DMA2_Stream0->CR &= ~(3<<23);	//Memory burst transfer configuration - single transfer
	DMA2_Stream0->CR &= ~(3<<21);	//Peripheral burst transfer configuration - single transfer  
	DMA2_Stream0->CR |= (1<<0);	//DMA2_Stream0 enabled
}

void TIMER1_OC_Init(void) //TIM1_CH2 CC 
{
	// Timerbase Mode
	RCC->APB2ENR   |= 0x01;// RCC_APB1ENR TIMER1 Enable

	TIM1->PSC = 16800-1;   // Prescaler 168,000,000Hz/16800= 10kHz (0.1ms)

	TIM1->ARR = 4000-1;    // 400m/0.1m = 4000
    
	TIM1->CR1 &= ~(1<<4);   // DIR: Countermode = Upcounter (reset state)
    TIM1->CR1 &= ~(1<<1);     // UDIS=0 (Update event Enabled)
    TIM1->CR1 &= ~(1<<2);       // URS = 0
    TIM1->CR1 &= ~(1<<3);   // OPM=0(The counter is NOT stopped at update event) (reset state)
    TIM1->CR1 &= ~(1<<7);   // ARPE=0(ARR is NOT buffered) (reset state)
    TIM1->CR1 &= ~(3<<8);    // CKD(Clock division)=00(reset state)
    TIM1->CR1 &= ~(3<<5);    // CMS(Center-aligned mode Sel)=00 (Edge-aligned mode) (reset state)
    
       // Event & Interrup Enable : UI  
   TIM1->EGR |= (1<<0);    // UG: Update generation    
    
   // Define the corresponding pin by 'Output'  
   TIM1->CCER |= (1<<4*(2-1));   // CC2E=1: CC2 channel Output Enable
            // OC3(TIM5_CH3) Active: 해당핀을 통해 신호출력
   TIM1->CCER &= ~(3<<(4*(2-1)+1));   // CC2P=0: CC2 channel Output Polarity (OCPolarity_High : OC2으로 반전없이 출력)  

   // 'Mode' Selection : Output mode, toggle  
   TIM1->CCMR1 &= ~(3<<8); // CC2S(CC2 channel) = '0b00' : Output 
   TIM1->CCMR1 &= ~(1<<11); // OC2P=0: Output Compare 2 preload disable
   TIM1->CCMR1 |= (3<<12);   // OC2M=0b011: Output Compare 2 Mode : toggle

    TIM1->CCR2 = 3;   // TIM1 CCR2 TIM1_Pulse
    
    TIM1->BDTR |= (1<<15);  // main output enable
      
   TIM1->CR1 |= (1<<0);   // CEN: Enable the Tim5 Counter   
}

void USART1_Init(void)
{
	// USART1 : TX(PA9)
	RCC->AHB1ENR	|= (1<<0);	// RCC_AHB1ENR GPIOA Enable
	GPIOA->MODER	|= (2<<2*9);	// GPIOB PIN9 Output Alternate function mode					
	GPIOA->OSPEEDR	|= (3<<2*9);	// GPIOB PIN9 Output speed (100MHz Very High speed)
	GPIOA->AFR[1]	|= (7<<4);	// Connect GPIOA pin9 to AF7(USART1)
    
	// USART1 : RX(PA10)
	GPIOA->MODER 	|= (2<<2*10);	// GPIOA PIN10 Output Alternate function mode
	GPIOA->OSPEEDR	|= (3<<2*10);	// GPIOA PIN10 Output speed (100MHz Very High speed
	GPIOA->AFR[1]	|= (7<<8);	// Connect GPIOA pin10 to AF7(USART1)

	RCC->APB2ENR	|= (1<<4);	// RCC_APB2ENR USART1 Enable
    
	USART_BRR_Configuration(9600); // USART Baud rate Configuration
    
	USART1->CR1	|= (1<<12);	// USART_WordLength 8 Data bit
	USART1->CR1	|= (1<<10);	// NO USART_Parity
    USART1->CR1	|= (1<<9);	// ODD_Parity
	USART1->CR1	|= (1<<2);	// 0x0004, USART_Mode_RX Enable
	USART1->CR1	|= (1<<3);	// 0x0008, USART_Mode_Tx Enable
	USART1->CR2	&= ~(3<<12);	// 0b00, USART_StopBits_1
	USART1->CR3	= 0x0000;	// No HardwareFlowControl, No DMA
    
	USART1->CR1	|= (1<<5);	// 0x0020, RXNE interrupt Enable
	USART1->CR1	&= ~(1<<7); // 0x0080, TXE interrupt Disable 

	NVIC->ISER[1]	|= (1<<(37-32));// Enable Interrupt USART1 (NVIC 37번)
	USART1->CR1 	|= (1<<13);	//  0x2000, USART1 Enable
}

void USART_BRR_Configuration(uint32_t USART_BaudRate) // Baud rate  
{ 
	uint32_t tmpreg = 0x00;
	uint32_t APB2clock = 84000000;	//PCLK2_Frequency
	uint32_t integerdivider = 0x00;
	uint32_t fractionaldivider = 0x00;

	// Determine the integer part 
	if ((USART1->CR1 & USART_CR1_OVER8) != 0) // USART_CR1_OVER8=(1<<15)
	{                                         // USART1->CR1.OVER8 = 1 (8 oversampling)
		// Computing 'Integer part' when the oversampling mode is 8 Samples 
		integerdivider = ((25 * APB2clock) / (2 * USART_BaudRate));    
	}
	else  // USART1->CR1.OVER8 = 0 (16 oversampling)
	{	// Computing 'Integer part' when the oversampling mode is 16 Samples 
		integerdivider = ((25 * APB2clock) / (4 * USART_BaudRate));    
	}
	tmpreg = (integerdivider / 100) << 4;
  
	// Determine the fractional part 
	fractionaldivider = integerdivider - (100 * (tmpreg >> 4));

	// Implement the fractional part in the register 
	if ((USART1->CR1 & USART_CR1_OVER8) != 0)	// 8 oversampling
	{
		tmpreg |= (((fractionaldivider * 8) + 50) / 100) & (0x07);
	}
	else 			// 16 oversampling
	{
		tmpreg |= (((fractionaldivider * 16) + 50) / 100) & (0x0F);
	}

	// Write to USART BRR register
	USART1->BRR = (uint16_t)tmpreg;
}

void USART1_IRQHandler(void)	
{       
	// TX Buffer Empty Interrupt
	if ( (USART1->CR1 & (1<<7)) && (USART1->SR & USART_SR_TXE) )		// USART_SR_TXE=(1<<7)
	{
		USART1->CR1 &= ~(1<<7);	// 0x0080, TXE interrupt Enable 
		sprintf(str,"%2d",send_ch);
		LCD_DisplayText(8,4,str);

		USART1->DR = send_ch;	// 1 byte 전송
		DelayMS(500);///
		send_ch++;
		if (send_ch < 0x30+No_send_data )   // 데이터(문자) 10byte 전송
			USART1->CR1 |= (1<<7); 	// TXE interrupt Enable 
	} 
	// RX Buffer Full interrupt
	if ( (USART1->SR & USART_SR_RXNE) )		// USART_SR_RXNE=(1<<5) 
	{
		char ch;
		ch = (uint16_t)(USART1->DR & (uint16_t)0x01FF);	// 수신된 문자 저장
		
        switch (ch) {
        case '1':                               // 들어온 데이터가 1인경우
          sprintf(str,"%2d ",Sensor1.Temp);
          SerialSendString(str);            // 센서1 데이터 전송
          LCD_DisplayChar(0,16,ch); 	// LCD display
          break;
          case '2':                               // 들어온 데이터가 2인경우
          sprintf(str,"%2d ",Sensor2.Temp);
          SerialSendString(str);            // 센서2 데이터 전송
          LCD_DisplayChar(0,16,ch); 	// LCD display
          break;
          case '3':                               // 들어온 데이터가 3인경우
          sprintf(str,"%2d ",Sensor3.Temp);
          SerialSendString(str);            // 센서3 데이터 전송
          LCD_DisplayChar(0,16,ch); 	// LCD display
          break;
        }
		// DR 을 읽으면 SR.RXNE bit(flag bit)는 clear 된다. 즉 clear 할 필요없음 
	} 
}

void DispayTitle(void)
{	
    LCD_Clear(RGB_WHITE);

	LCD_SetFont(&Gulim8);
	LCD_SetBackColor(RGB_WHITE);	//배경색
	LCD_SetTextColor(RGB_BLACK);	//글자색
	LCD_DisplayText(0,0,"JJW 2019132036");
	LCD_SetTextColor(RGB_BLUE);	//글자색
    LCD_DisplayText(1,0,"EXT1 TMP:  C(   V)");
	LCD_DisplayText(3,0,"EXT2 TMP:  C(   V)");
    LCD_DisplayText(5,0,"INT  TMP:  C(   V)");
  
    LCD_SetTextColor(RGB_RED);	//글자색
    LCD_DisplayChar(1,14,'.');
    LCD_DisplayChar(3,14,'.');
    LCD_DisplayChar(5,14,'.');
    
    LCD_DisplayChar(0,16,'0'); 	// LCD display
}

void SerialSendChar(uint8_t Ch) // 1문자 보내기 함수
{
	while((USART1->SR & USART_SR_TXE) == RESET); // USART_SR_TXE=(1<<7), 송신 가능한 상태까지 대기

	USART1->DR = (Ch & 0x01FF);	// 전송 (최대 9bit 이므로 0x01FF과 masking)
}

void SerialSendString(char *str) // 여러문자 보내기 함수
{
	while (*str != '\0') // 종결문자가 나오기 전까지 구동, 종결문자가 나온후에도 구동시 메모리 오류 발생가능성 있음.
	{
		SerialSendChar(*str);	// 포인터가 가르키는 곳의 데이터를 송신
		str++; 			// 포인터 수치 증가
	}
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
		{		DelayMS(10);
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

void DelayMS(unsigned short wMS)
{	register unsigned short i;
	for (i=0; i<wMS; i++)
		DelayUS(1000);  // 1000us => 1ms
}

void DelayUS(unsigned short wUS)
{	volatile int Dly = (int)wUS*17;

	for(; Dly; Dly--);
}
void BEEP(void)			// Beep for 20 ms 
{ 	GPIOF->ODR |= (1<<9);	// PF9 'H' Buzzer on
	DelayMS(20);		// Delay 20 ms
	GPIOF->ODR &= ~(1<<9);	// PF9 'L' Buzzer off
}