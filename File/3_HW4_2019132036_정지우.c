//////////////////////////////////////////////////////////////////////////
// SPI ����� �̿��� ���ӵ����� ����
//  NSS pin:  PA8 (PA4(SPI1_CS) ��ſ� ���)
//  SCK pin:  PA5 (SPI1_SCK)
//  MOSI pin: PA7 (SPI1_MOSI)
//  MISO pin: PA6 (SPI1_MISO)
// SPI mode: MASTER
// ���ӵ�����(LIS2HH12, Slave mode) X,Y,Z ���� 250ms���� ������ LCD�� ǥ�� 
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

void Accelerometer_Calibration(int time);   // offset���� calibration
void Cal_Accel(int16 *pBuf);                        // ���ӵ� �������� ���ӵ������� ����
void DrawBall();                                        // ���� ��ġ�� ���  �� LCDǥ��

UINT8 bControl;
int frame[4] = {3,15,110,110};  // ���� ������ �� �ִ� �簢�� ���� ����

int16 offset[3] = {0,};  // offset���� calibration����� offset�� ����
float accel[3] = {0,};  // ���� x, y, z���ӵ� �� ����
int ball_vel[2] = {0,0};    // x, y���� �ӵ��� ����
int ball_loc[2] = {0,}; // ���� x, y����ġ�� ����

const int ball_size = 4;    // ���� ũ�⸦ 4�� ����
const float m = 5; // ���� ������ 5�� ����
const float friction = 0.37; // ���� ������ �������� 0.37�� ����

int main(void)
{
	int16 buffer[3];
    
	LCD_Init();		// LCD ���� �Լ�
	DelayMS(10);		// LCD���� ������
	DisplayTitle();		// LCD �ʱ�ȭ�鱸�� �Լ�
    
	_GPIO_Init();		// LED, SW �ʱ�ȭ
	SPI1_Init();        	// SPI1 �ʱ�ȭ
	ACC_Init();		// ���ӵ����� �ʱ�ȭ
	TIMER10_Init();		// ���ӵ����� ��ĵ �ֱ� ����
    
    Accelerometer_Calibration(100); // ���ӵ� ���� offset����

	while(1)
	{
		if(bControl) // bContriol��  1�� �� ��� ���� -> 200ms���� ����
		{   
			bControl = FALSE;     
			SPI1_Process(&buffer[0]);	// SPI����� �̿��Ͽ� ���ӵ����� ����
            Cal_Accel(&buffer[0]);  // ���ӵ� �������� ���ӵ� ������ �����Ͽ� ���������� ����
            DrawBall();                 // ���� ��ġ�� ��� ��  ���� ���÷��̿� ǥ��
			Display_Process();	// �������� LCD�� ǥ��
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
	
	/*!< SPI1 NSS pin(PA8) configuration : GPIO ��  */
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
				// NSS �� ���°� �ڵ��� ���� ����
	SPI1->CR1 |= (1<<8);	// SSI(Internal_slave_select)=1,
				// ���� MCU�� Master�̹Ƿ� NSS ���´� 'High' 
	SPI1->CR1 &= ~(1<<7);	// LSBFirst=0, MSB transmitted first    
	SPI1->CR1 |= (4<<3);	// BR(BaudRate)=0b100, fPCLK/32 (84MHz/32 = 2.625MHz)
	SPI1->CR1 |= (1<<1);	// CPOL(Clock polarity)=1, CK is 'High' when idle
	SPI1->CR1 |= (1<<0);	// CPHA(Clock phase)=1, �� ��° edge ���� �����Ͱ� ���ø�
 
	SPI1->CR1 |= (1<<6);	// SPE=1, SPI1 Enable 
}

void TIMER10_Init(void)	// ���ӵ����� ���� �ֱ� ����: 200ms
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

	bControl = TRUE;	// 200ms���� ���� ����
}

void Display_Process()
{  
	// X �� ���ӵ� ǥ��		
	if (accel[0] < 0)  //����
	{
		LCD_DisplayChar(2,22,'-'); // 16���� ��ȣ ǥ��
        accel[0] *= -1;                 // ���� ������ ����
	}
	else				// ���
	{
		LCD_DisplayChar(2,22,'+'); // 16���� ��ȣ ǥ��
	}
	LCD_DisplayChar(2,23, (int)accel[0] +0x30); // �����ڸ� ǥ��
	LCD_DisplayChar(2,24,'.');
	LCD_DisplayChar(2,25, ((int)(accel[0]*10))%10 +0x30); // �Ҽ��� �ڸ� ǥ��
    
    // Y �� ���ӵ� ǥ��		
	if (accel[1] < 0)  //����
	{
		LCD_DisplayChar(3,22,'-'); // 16���� ��ȣ ǥ��
        accel[1] *= -1;                 // ���� ������ ����
	}
	else				// ���
	{
		LCD_DisplayChar(3,22,'+'); // 16���� ��ȣ ǥ��
	}
	LCD_DisplayChar(3,23, (int)accel[1] +0x30); // �����ڸ� ǥ��
	LCD_DisplayChar(3,24,'.');
	LCD_DisplayChar(3,25, ((int)(accel[1]*10))%10 +0x30); // �Ҽ��� �ڸ� ǥ��
    
    // Z �� ���ӵ� ǥ��		
	if (accel[2] < 0)  //����
	{
		LCD_DisplayChar(4,22,'-'); // 16���� ��ȣ ǥ��
        accel[2] *= -1;                 // ���� ������ ����
	}
	else				// ���
	{
		LCD_DisplayChar(4,22,'+'); // 16���� ��ȣ ǥ��
	}
	LCD_DisplayChar(4,23, (int)accel[2] +0x30); // �����ڸ� ǥ��
	LCD_DisplayChar(4,24,'.');
	LCD_DisplayChar(4,25, ((int)(accel[2]*10))%10 +0x30); // �Ҽ��� �ڸ� ǥ��
}

void DrawBall() 
{
  float fvx = (float)(friction*accel[2]); // z�� ���ӵ��� �̿��Ͽ� x�� ������ ���
  if (fvx < 0)                                      // ���� �� ��� ����� ����
    fvx *= -1;

  float fvy = fvx;                                 // y��  �����¿� x�� ������ ����
  
  if (ball_vel[0] == 0)                             // ���� x������ �������� ������ x�� ������ 0
    fvx = 0;
  if (ball_vel[0] > 0)                              // x�� �ӵ��� �����  ������ ���� ����
    fvx *= -1;
  float Fx = m*accel[0];                            //  x�� ������ ���ӵ��� �̿��Ͽ�  ���� ���
  Fx -= fvx;                                            // ���� �������� �� x�� ���� ��¥�� ���
  float ax = -Fx/m;                                  // ��¥���� �������� ���� ���� ���ӵ� ���   
  
  
  if (ball_vel[1] == 0)                 // ���� y������ �������� ������ y�� ������ 0
    fvy = 0;
  if (ball_vel[1] > 0)                  // y�� �ӵ��� �����  ������ ���� ����
    fvy *= -1;
  float Fy = m*accel[1];            //  y�� ������ ���ӵ��� �̿��Ͽ�  ���� ���
  Fy -= fvy;                             // ���� �������� �� y�� ���� ��¥�� ���
  float ay = -Fy/m;                 // ��¥���� �������� ���� ���� ���ӵ� ���   
  
  ball_vel[0] += ax*20;    // ���ӵ��� ���Ͽ� �ӵ� ����
  ball_vel[1] += ay*20;
  
  LCD_SetPenColor(RGB_WHITE);   // ���� ���� ��ġ�� ����
  LCD_DrawRectangle(ball_loc[0] - ball_size/2,
                     ball_loc[1] - ball_size/2,
                     ball_size,ball_size);
  
  ball_loc[0] += ball_vel[0];       // �ӵ��� ���Ͽ� ��ġ ������Ʈ
  ball_loc[1] += ball_vel[1];
  
  // ���� ������ ���� �Ѿ�� �˻�
  // �Ѿ ��� ���� �ٴ� ��ġ�� ��ġ ����
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
   
  LCD_SetPenColor(RGB_RED); // �� ���� ��ġ �׸���
  LCD_DrawRectangle(ball_loc[0] - ball_size/2,
                    ball_loc[1] - ball_size/2,
                    ball_size,ball_size);
}

void _GPIO_Init(void)
{
	// LED (GPIO G) ����
	RCC->AHB1ENR	|=  0x00000040;	// RCC_AHB1ENR : GPIOG(bit#6) Enable							
	GPIOG->MODER 	|=  0x00005555;	// GPIOG 0~7 : Output mode (0b01)						
	GPIOG->OTYPER	&= ~0x00FF;	// GPIOG 0~7 : Push-pull  (GP8~15:reset state)	
	GPIOG->OSPEEDR 	|=  0x00005555;	// GPIOG 0~7 : Output speed 25MHZ Medium speed 
    
	// SW (GPIO H) ���� 
	RCC->AHB1ENR    |=  0x00000080;	// RCC_AHB1ENR : GPIOH(bit#7) Enable							
	GPIOH->MODER 	&= ~0xFFFF0000;	// GPIOH 8~15 : Input mode (reset state)				
	GPIOH->PUPDR 	&= ~0xFFFF0000;	// GPIOH 8~15 : Floating input (No Pull-up, pull-down) :reset state

	// Buzzer (GPIO F) ���� 
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
	LCD_SetFont(&Gulim7);		//��Ʈ
    LCD_SetBackColor(RGB_WHITE);
	LCD_SetTextColor(RGB_BLACK);    //���ڻ�
	LCD_DisplayText(0,0,"Ball game: JJW 2019132036");  // Title
		
	LCD_SetBackColor(RGB_WHITE);    //���ڹ���

	LCD_DisplayText(2,19,"Ax:");	//X AXIS
	LCD_DisplayText(3,19,"Ay:");	//Y AXIS
	LCD_DisplayText(4,19,"Az:");	//Z AXIS
    
    LCD_SetTextColor(RGB_RED);    //���ڻ�
    
    LCD_SetPenColor(RGB_BLUE); 
    LCD_DrawRectangle(frame[0]-1,frame[1]-1,frame[2]+2,frame[3]+2); // ���� ������ �ڽ� �׸���
    ball_loc[0] = frame[0] + frame[2]/2;    // �������� ������ ���� �߾ӿ� ���� ��ġ ����
    ball_loc[1] = frame[1] + frame[3]/2;
    
    LCD_SetPenColor(RGB_RED); 
    LCD_DrawRectangle(ball_loc[0] - ball_size/2, // �� �׸���
                      ball_loc[1] - ball_size/2,
                      ball_size,ball_size);
    
}

void Accelerometer_Calibration(int time){ // average ���͸� �̿��� �ʱ� bias����
  int16 buffer[3];
  long long l_offset[3] = {0,};
  for (int i = 0; i<time; i++) {    // �Է� �� Ƚ����ŭ ���ӵ� ���� �޾� ���ذ�.
    SPI1_Process(&buffer[0]);	// SPI����� �̿��Ͽ� ���ӵ����� ����
    l_offset[0] += buffer[0];
    l_offset[1] += buffer[1];
    l_offset[2] += buffer[2];
    memset(buffer, NULL,3);
  }
  // ���� Ƚ���� ������ ������ ����
  offset[0] = l_offset[0] / time;
  offset[1] = l_offset[1] / time;
  offset[2] = l_offset[2] / time - 0.9 * 0x4009;    // z���� ��Ⱑ  ������ ���� ������ 0.9�� ���� ������ �ǹǷ� �ش� ��ġ�� ��.
}

void Cal_Accel(int16 *pBuf) {
  // �������� ������ �� ���ӵ� ���
  accel[0] = ((float)(pBuf[0] - offset[0])*10 / 0x4009)/10;
  accel[1] = ((float)(pBuf[1] - offset[1])*10 / 0x4009)/10;
  accel[2] = ((float)(pBuf[2] - offset[2])*10 / 0x4009)/10;
}