/////////////////////////////////////////////////////////////
// 과제명: 커피자판기
// 과제개요: 스위치를 이용하여 커피자판기 의 기능을 LED로 대체하여 구현
// 사용한  하드웨어(기능): GPIO, SW, LED
// 제출일: 2023. 5. 8
// 제출자  클래스: 화요일반
//           학번: 2019132036
//           이름: 정지우
///////////////////////////////////////////////////////////////

#include "stm32f4xx.h"

void _GPIO_Init(void);
uint16_t KEY_Scan(void);
void BEEP(void);

void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);

void Coffee(const char coffee); // Coffee production motion function


int main(void)
{
  DelayMS(100); // Stabilization
  _GPIO_Init();    // GPIO Initialization
  GPIOG->ODR &= ~0x00FF;	// 초기값: LED0~7 Off
  
  unsigned short coin = 0; // coin Initialization

  while(1)
  {
    switch(KEY_Scan()) {
      case 0xFE00:          // Coin(SW0) 
        if (coin == 0) {        // If the coin has not come in before
          BEEP();               //  buzzer
          coin = 1;               //  Save that coin came in
          GPIOG->BSRRL |= 0x0001; // CoinLED(LED0) On
        }
        break;
      case 0xFD00 : 	// BlackCoffee(SW1)
        if (coin == 0) break; // If the coin didn't come in, it wouldn't activate
        GPIOG->ODR |= 0x0002;   // BlackCoffee(LED1) On
        BEEP();
        Coffee('b'); //activate of coffee function as a parameter of black coffee
        coin = 0; // coin Initialization
      break;
      case 0xFB00:                  //Sugar(SW2)
        if (coin == 0) break;       // If the coin didn't come in, it wouldn't activate
        GPIOG->ODR |= 0x0004; // Sugar(LED2) On
        BEEP();
        Coffee('s'); //activate of coffee function as a parameter of sugar coffee
        coin = 0; // coin Initialization
      break;
      case 0xF700: //Mix(SW3)
        if (coin == 0) break; // If the coin didn't come in, it wouldn't activate
        GPIOG->ODR |= 0x008; // Mix(LED3) On
        BEEP();
        Coffee('m'); //activate of coffee function as a parameter of mix coffee
        coin = 0; // coin Initialization
      break;
    }
  }
}

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

/* GPIO (GPIOG(LED), GPIOH(Switch), GPIOF(Buzzer)) 초기 설정	*/
void _GPIO_Init(void)
{
	// LED (GPIO G) 설정 : Output mode
	RCC->AHB1ENR	    |=      0x00000040;	// RCC_AHB1ENR : GPIOG(bit#6) Enable							
	GPIOG->MODER 	    |=      0x00005555;	// GPIOG 0~7 : Output mode (0b01)						
	GPIOG->OTYPER	    &=   ~0x00FF;	    // GPIOG 0~7 : Push-pull  (GP8~15:reset state)	
	GPIOG->OSPEEDR 	|=      0x00005555;	// GPIOG 0~7 : Output speed 25MHZ Medium speed 
   
	// SW (GPIO H) 설정 : Input mode 
	RCC->AHB1ENR        |=      0x00000080;	// RCC_AHB1ENR : GPIOH(bit#7) Enable							
	GPIOH->MODER        &=   ~0x00FF0000;	// GPIOH 8~12 : Input mode (reset state)				
	GPIOH->PUPDR 	     &=   ~0x00FF0000;	// GPIOH 8~12 : Floating input (No Pull-up, pull-down) :reset state

	// Buzzer (GPIO F) 설정 : Output mode 
	RCC->AHB1ENR	|=  0x00000020;  // RCC_AHB1ENR : GPIOF(bit#5) Enable		
    GPIOF->MODER 	&= ~0x000C0000;	 // GPIOF 9 : Initialization					
	GPIOF->MODER 	|=  0x00040000;	 // GPIOF 9 : Output mode (0b01)						
	GPIOF->OTYPER 	&= ~0x0200;	     // GPIOF 9 : Push-pull  	
 	GPIOF->OSPEEDR  |=  0x00040000;	 // GPIOF 9 : Output speed 25MHZ Medium speed 
}	

void BEEP(void)			/* beep for 30 ms */
{ 	
	GPIOF->ODR |=  0x0200;	// PF9 'H' Buzzer on
	DelayMS(30);			// Delay 30 ms
	GPIOF->ODR &= ~0x0200;	// PF9 'L' Buzzer off
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

void Coffee(const char coffee){
  if (coffee !='b' && coffee !='s' && coffee !='m') return;  // Exit unless it is black, mix, sugar coffee
  int i = 0;
  DelayMS(1000);    //Wait for 1 second
  
  GPIOG->ODR  |= 0x0080;    // CUP(LED7) Blinking One time
  DelayMS(500);
  GPIOG->ODR  &= ~0x0080;
  
  for (i = 0; i < 2 && (coffee == 's' || coffee == 'm'); i++) { // If it is Sugar or Mix coffee, Sugar(LED6) Blinking Two time every 0.5 seconds
    DelayMS(500);
    GPIOG->ODR  |= 0x0040;    
    DelayMS(500);
    GPIOG->ODR  &= ~0x0040;
  }
  
  for (i = 0; i < 2 && coffee == 'm'; i++) { //If it is Mix coffee, Mix(LED5) Blinking Two time every 0.5 seconds
    DelayMS(500);
    GPIOG->ODR  |= 0x0020;    
    DelayMS(500);
    GPIOG->ODR  &= ~0x0020;
  }
  
  for (i = 0; i < 3; i++) { // Water/Coffee(LED4) Blinking Three time every 0.5 seconds
    DelayMS(500);
    GPIOG->ODR  |= 0x0010;    
    DelayMS(500);
    GPIOG->ODR  &= ~0x0010;
  }
  
  for (i = 0; i < 3; i++) { // Coffee production done, buzzer 3 times every 0.5 seconds
      DelayMS(500);
      BEEP();
  }
  DelayMS(1000);                    // Wait for 1 second
  GPIOG->BSRRH |= 0x0001; // Coin reset
  GPIOG->ODR &= ~0x000E;	// LED1~7 Initialization
}