//////////////////////////////////////////////////////////////////////////
// SPI 통신을 이용한 가속도센서 측정
//  NSS pin:  PA8 (PA4(SPI1_CS) 대신에 사용)
//  SCK pin:  PA5 (SPI1_SCK)
//  MOSI pin: PA7 (SPI1_MOSI)
//  MISO pin: PA6 (SPI1_MISO)
// SPI mode: MASTER
// 가속도센서(LIS2HH12, Slave mode) X,Y,Z 값을 250ms마다 읽은후 LCD에 표시 
//////////////////////////////////////////////////////////////////////////
#include "stm32f4xx.h"
#include "GLCD.h"
#include "ACC.h"

void DisplayTitle(void);
void _GPIO_Init(void);
void SPI1_Init(void);
void TIMER10_Init(void);
void Display_Process();

void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);

void Accelerometer_Calibration(int time);   // offset제거 calibration
void Cal_Accel(int16 *pBuf);                        // 가속도 센서값을 가속도값으로 병경
void DrawBall();                                        // 공의 위치를 계산  후 LCD표시

UINT8 bControl;
int frame[4] = {3,15,110,110};  // 공이 움질일 수 있는 사각형 정보 저장

int16 offset[3] = {0,};  // offset제거 calibration적용시 offset값 저장
float accel[3] = {0,};  // 공의 x, y, z가속도 값 저장
int ball_vel[2] = {0,0};    // x, y축의 속도값 저장
int ball_loc[2] = {0,}; // 공의 x, y축위치값 저장

const int ball_size = 4;    // 공의 크기를 4로 가정
const float m = 5; // 공의 질량을 5로 가정
const float friction = 0.37; // 공과 지면의 마찰력을 0.37로 가정

int main(void)
{
	int16 buffer[3];
    
	LCD_Init();		// LCD 구동 함수
	DelayMS(10);		// LCD구동 딜레이
	DisplayTitle();		// LCD 초기화면구동 함수
    
	_GPIO_Init();		// LED, SW 초기화
	SPI1_Init();        	// SPI1 초기화
	ACC_Init();		// 가속도센서 초기화
	TIMER10_Init();		// 가속도센서 스캔 주기 생성
    
    Accelerometer_Calibration(100); // 가속도 센서 offset제거

	while(1)
	{
		if(bControl) // bContriol가  1이 될 경우 측정 -> 200ms마다 측정
		{   
			bControl = FALSE;     
			SPI1_Process(&buffer[0]);	// SPI통신을 이용하여 가속도센서 측정
            Cal_Accel(&buffer[0]);  // 가속도 센서값을 가속도 값으로 변경하여 전역변수에 저장
            DrawBall();                 // 공의 위치를 계산 후  공을 디스플레이에 표시
			Display_Process();	// 측정값을 LCD에 표시
		}
	}
}

///////////////////////////////////////////////////////
// Master mode, Full-duplex, 8bit frame(MSB first), 
// fPCLK/32 Baud rate, Software slave management EN
void SPI1_Init(void)
{
	/*!< Clock Enable  *********************************************************/
	RCC->APB2ENR 	|= (1<<12);	// 0x1000, SPI1 Clock EN
	RCC->AHB1ENR 	|= (1<<0);	// 0x0001, GPIOA Clock EN		
  
	/*!< SPI1 pins configuration ************************************************/
	
	/*!< SPI1 NSS pin(PA8) configuration : GPIO 핀  */
	GPIOA->MODER 	|= (1<<(2*8));	// 0x00010000, PA8 Output mode
	GPIOA->OTYPER 	&= ~(1<<8); 	// 0x0100, push-pull(reset state)
	GPIOA->OSPEEDR 	|= (3<<(2*8));	// 0x00030000, PA8 Output speed (100MHZ) 
	GPIOA->PUPDR 	&= ~(3<<(2*8));	// 0x00030000, NO Pullup Pulldown(reset state)
    
	/*!< SPI1 SCK pin(PA5) configuration : SPI1_SCK */
	GPIOA->MODER 	|= (2<<(2*5)); 	// 0x00000800, PA5 Alternate function mode
	GPIOA->OTYPER 	&= ~(1<<5); 	// 0020, PA5 Output type push-pull (reset state)
	GPIOA->OSPEEDR 	|= (3<<(2*5));	// 0x00000C00, PA5 Output speed (100MHz)
	GPIOA->PUPDR 	|= (2<<(2*5)); 	// 0x00000800, PA5 Pull-down
	GPIOA->AFR[0] 	|= (5<<(4*5));	// 0x00500000, Connect PA5 to AF5(SPI1)
    
	/*!< SPI1 MOSI pin(PA7) configuration : SPI1_MOSI */    
	GPIOA->MODER 	|= (2<<(2*7));	// 0x00008000, PA7 Alternate function mode
	GPIOA->OTYPER	&= ~(1<<7);	// 0x0080, PA7 Output type push-pull (reset state)
	GPIOA->OSPEEDR 	|= (3<<(2*7));	// 0x0000C000, PA7 Output speed (100MHz)
	GPIOA->PUPDR 	|= (2<<(2*7)); 	// 0x00008000, PA7 Pull-down
	GPIOA->AFR[0] 	|= (5<<(4*7));	// 0x50000000, Connect PA7 to AF5(SPI1)
    
	/*!< SPI1 MISO pin(PA6) configuration : SPI1_MISO */
	GPIOA->MODER 	|= (2<<(2*6));	// 0x00002000, PA6 Alternate function mode
	GPIOA->OTYPER 	&= ~(1<<6);	// 0x0040, PA6 Output type push-pull (reset state)
	GPIOA->OSPEEDR 	|= (3<<(2*6));	// 0x00003000, PA6 Output speed (100MHz)
	GPIOA->PUPDR 	|= (2<<(2*6));	// 0x00002000, PA6 Pull-down
	GPIOA->AFR[0] 	|= (5<<(4*6));	// 0x05000000, Connect PA6 to AF5(SPI1)

	// Init SPI1 Registers 
	SPI1->CR1 |= (1<<2);	// MSTR(Master selection)=1, Master mode
	SPI1->CR1 &= ~(1<<15);	// SPI_Direction_2 Lines_FullDuplex
	SPI1->CR1 &= ~(1<<11);	// SPI_DataSize_8bit
	SPI1->CR1 |= (1<<9);  	// SSM(Software slave management)=1, 
				// NSS 핀 상태가 코딩에 의해 결정
	SPI1->CR1 |= (1<<8);	// SSI(Internal_slave_select)=1,
				// 현재 MCU가 Master이므로 NSS 상태는 'High' 
	SPI1->CR1 &= ~(1<<7);	// LSBFirst=0, MSB transmitted first    
	SPI1->CR1 |= (4<<3);	// BR(BaudRate)=0b100, fPCLK/32 (84MHz/32 = 2.625MHz)
	SPI1->CR1 |= (1<<1);	// CPOL(Clock polarity)=1, CK is 'High' when idle
	SPI1->CR1 |= (1<<0);	// CPHA(Clock phase)=1, 두 번째 edge 에서 데이터가 샘플링
 
	SPI1->CR1 |= (1<<6);	// SPE=1, SPI1 Enable 
}

void TIMER10_Init(void)	// 가속도센서 측정 주기 생성: 200ms
{
	RCC->APB2ENR 	|= (1<<17);	// TIMER10 Clock Enable
     
	TIM10->PSC 	= 8400-1;	// Prescaler 84MHz/8400 = 10KHz (0.1ms)  
	TIM10->ARR 	= 2500-1;	// Auto reload  0.1ms * 2000 = 200ms

	TIM10->CR1	&= ~(1<<4);	// Countermode = Upcounter (reset state)
	TIM10->CR1 	&= ~(3<<8);	// Clock division = 1 (reset state)
	TIM10->EGR 	|=(1<<0);	// Update Event generation    
    
    TIM10->EGR |= (1<<1);   // Timer10 CCEvent generation
    TIM10->CCMR1 &= ~(3<<0);    // Timr10 OCmode select
    TIM10->CCMR1 &= (1<<4);     // Set cc match event 
    
    TIM10->CCER |= (1<<0);  // CC1E=1 channel output enable;
    
    TIM10->CCR1 = 100;
    TIM10->DIER |= (1<<1) ;      // CC1 update interrupt
	NVIC->ISER[0] 	|= (1 << 25);	// Enable Timer10 global Interrupt
	TIM10->CR1 	|= (1<<0);	// Enable Tim10 Counter    
}

void TIM1_UP_TIM10_IRQHandler(void)	// 200ms int
{
	TIM10->SR &= ~(1<<1);	//Interrupt flag Clear

	bControl = TRUE;	// 200ms마다 센서 측정
}

void Display_Process()
{  
	// X 축 가속도 표시		
	if (accel[0] < 0)  //음수
	{
		LCD_DisplayChar(2,22,'-'); // 16진수 부호 표시
        accel[0] *= -1;                 // 양의 값으로 변경
	}
	else				// 양수
	{
		LCD_DisplayChar(2,22,'+'); // 16진수 부호 표시
	}
	LCD_DisplayChar(2,23, (int)accel[0] +0x30); // 일의자리 표시
	LCD_DisplayChar(2,24,'.');
	LCD_DisplayChar(2,25, ((int)(accel[0]*10))%10 +0x30); // 소수점 자리 표시
    
    // Y 축 가속도 표시		
	if (accel[1] < 0)  //음수
	{
		LCD_DisplayChar(3,22,'-'); // 16진수 부호 표시
        accel[1] *= -1;                 // 양의 값으로 변경
	}
	else				// 양수
	{
		LCD_DisplayChar(3,22,'+'); // 16진수 부호 표시
	}
	LCD_DisplayChar(3,23, (int)accel[1] +0x30); // 일의자리 표시
	LCD_DisplayChar(3,24,'.');
	LCD_DisplayChar(3,25, ((int)(accel[1]*10))%10 +0x30); // 소수점 자리 표시
    
    // Z 축 가속도 표시		
	if (accel[2] < 0)  //음수
	{
		LCD_DisplayChar(4,22,'-'); // 16진수 부호 표시
        accel[2] *= -1;                 // 양의 값으로 변경
	}
	else				// 양수
	{
		LCD_DisplayChar(4,22,'+'); // 16진수 부호 표시
	}
	LCD_DisplayChar(4,23, (int)accel[2] +0x30); // 일의자리 표시
	LCD_DisplayChar(4,24,'.');
	LCD_DisplayChar(4,25, ((int)(accel[2]*10))%10 +0x30); // 소수점 자리 표시
}

void DrawBall() 
{
  float fvx = (float)(friction*accel[2]); // z축 가속도를 이용하여 x축 마찰력 계산
  if (fvx < 0)                                      // 음수 일 경우 양수로 변경
    fvx *= -1;

  float fvy = fvx;                                 // y축  마찰력에 x축 마찰력 저장
  
  if (ball_vel[0] == 0)                             // 공이 x축으로 움직이지 않으면 x축 마찰력 0
    fvx = 0;
  if (ball_vel[0] > 0)                              // x축 속도가 양수면  마찰력 음의 방향
    fvx *= -1;
  float Fx = m*accel[0];                            //  x축 방향의 가속도를 이용하여  힘을 계산
  Fx -= fvx;                                            // 힘에 마찰력을 빼 x축 방향 알짜힘 계산
  float ax = -Fx/m;                                  // 알짜힘을 질량으로 나눠 실제 가속도 계산   
  
  
  if (ball_vel[1] == 0)                 // 공이 y축으로 움직이지 않으면 y축 마찰력 0
    fvy = 0;
  if (ball_vel[1] > 0)                  // y축 속도가 양수면  마찰력 음의 방향
    fvy *= -1;
  float Fy = m*accel[1];            //  y축 방향의 가속도를 이용하여  힘을 계산
  Fy -= fvy;                             // 힘에 마찰력을 빼 y축 방향 알짜힘 계산
  float ay = -Fy/m;                 // 알짜힘을 질량으로 나눠 실제 가속도 계산   
  
  ball_vel[0] += ax*20;    // 가속도를 더하여 속도 저장
  ball_vel[1] += ay*20;
  
  LCD_SetPenColor(RGB_WHITE);   // 이전 공의 위치를 지움
  LCD_DrawRectangle(ball_loc[0] - ball_size/2,
                     ball_loc[1] - ball_size/2,
                     ball_size,ball_size);
  
  ball_loc[0] += ball_vel[0];       // 속도를 더하여 위치 업데이트
  ball_loc[1] += ball_vel[1];
  
  // 공이 프레임 밖을 넘어갈지 검사
  // 넘어갈 경우 벽에 붙는 위치로 위치 저장
  if (ball_loc[0] < frame[0]+ball_size/2){
    ball_loc[0] = frame[0]+ball_size/2;
    ball_vel[0] = 0;
  }
  else if (ball_loc[0] > frame[0]+frame[2]-ball_size/2) {
    ball_loc[0] = frame[0]+frame[2]-ball_size/2;
    ball_vel[0] = 0;
  }
  
  if (ball_loc[1] < frame[1]+ball_size/2) {
    ball_loc[1] = frame[1]+ball_size/2;
    ball_vel[1] = 0;
  }
  else if (ball_loc[1] > frame[1]+frame[3]-ball_size/2) {
    ball_loc[1] = frame[1]+frame[3]-ball_size/2;
    ball_vel[1] = 0;
  }
   
  LCD_SetPenColor(RGB_RED); // 새 공의 위치 그리기
  LCD_DrawRectangle(ball_loc[0] - ball_size/2,
                    ball_loc[1] - ball_size/2,
                    ball_size,ball_size);
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

	GPIOG->ODR &= 0x00;	// LED0~7 Off 
}

void DelayMS(unsigned short wMS)
{
	register unsigned short i;

	for (i=0; i<wMS; i++)
		DelayUS(1000);		//1000us => 1ms
}

void DelayUS(unsigned short wUS)
{
        volatile int Dly = (int)wUS*17;
         for(; Dly; Dly--);
}

void DisplayTitle(void)
{
	LCD_Clear(RGB_WHITE);
	LCD_SetFont(&Gulim7);		//폰트
    LCD_SetBackColor(RGB_WHITE);
	LCD_SetTextColor(RGB_BLACK);    //글자색
	LCD_DisplayText(0,0,"Ball game: JJW 2019132036");  // Title
		
	LCD_SetBackColor(RGB_WHITE);    //글자배경색

	LCD_DisplayText(2,19,"Ax:");	//X AXIS
	LCD_DisplayText(3,19,"Ay:");	//Y AXIS
	LCD_DisplayText(4,19,"Az:");	//Z AXIS
    
    LCD_SetTextColor(RGB_RED);    //글자색
    
    LCD_SetPenColor(RGB_BLUE); 
    LCD_DrawRectangle(frame[0]-1,frame[1]-1,frame[2]+2,frame[3]+2); // 공이 움직일 박스 그리기
    ball_loc[0] = frame[0] + frame[2]/2;    // 프레임의 정보를 기준 중앙에 공의 위치 결정
    ball_loc[1] = frame[1] + frame[3]/2;
    
    LCD_SetPenColor(RGB_RED); 
    LCD_DrawRectangle(ball_loc[0] - ball_size/2, // 공 그리기
                      ball_loc[1] - ball_size/2,
                      ball_size,ball_size);
    
}

void Accelerometer_Calibration(int time){ // average 필터를 이용한 초기 bias제거
  int16 buffer[3];
  long long l_offset[3] = {0,};
  for (int i = 0; i<time; i++) {    // 입력 된 횟수만큼 가속도 값을 받아 더해감.
    SPI1_Process(&buffer[0]);	// SPI통신을 이용하여 가속도센서 측정
    l_offset[0] += buffer[0];
    l_offset[1] += buffer[1];
    l_offset[2] += buffer[2];
    memset(buffer, NULL,3);
  }
  // 더한 횟수를 나누어 오프셋 저장
  offset[0] = l_offset[0] / time;
  offset[1] = l_offset[1] / time;
  offset[2] = l_offset[2] / time - 0.9 * 0x4009;    // z축은 기기가  평평한 곳에 있을시 0.9의 값을 가지게 되므로 해당 수치를 뺌.
}

void Cal_Accel(int16 *pBuf) {
  // 오프셋을 제거한 후 가속도 계산
  accel[0] = ((float)(pBuf[0] - offset[0])*10 / 0x4009)/10;
  accel[1] = ((float)(pBuf[1] - offset[1])*10 / 0x4009)/10;
  accel[2] = ((float)(pBuf[2] - offset[2])*10 / 0x4009)/10;
}