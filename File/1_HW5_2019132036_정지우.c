/////////////////////////////////////////////////////////////
// ������: ����������
// ��������: SW�� ������ �Է¹޾� �ش� ������ �̵��� LED�� ǥ���ϸ�, SW7���� ��������� �� �� 5�� �Ŀ� �����Ѵ�.
// �����  �ϵ����(���): GPIO, SW, LED, EXTI, GLCD
// ������: 2023. 5. 30
// ������  Ŭ����: ȭ���Ϲ�
//           �й�: 2019132036
//           �̸�: ������
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

void UpdateDisplay(int CurFlow, int DesFlow);   // ���÷��̿� ���� ���� ���� ǥ�� // 179
void UpdateLED(int CurFlow);                            // �������� LED�� Ű�� �Լ�// 199
int TargetFloor(void);                                          // ��ǥ���� 0~6�� ����ġ�� ���� �Է¹޾� ��ȯ�ϴ� �Լ�// 205
void BEEPS(int);                                                // ������ ������ �︮�� �Լ�


int main(void)
{
	_GPIO_Init();		// GPIO (LED & SW) �ʱ�ȭ
    _EXTI_Init();       // ���ͷ�Ʈ �ʱ�ȭ
	LCD_Init();		    // LCD ��� �ʱ�ȭ
	DelayMS(100);       // ����ȭ

	GPIOG->ODR |= 0x0001;	// LED �ʱⰪ: LED0~7 Off
	DisplayInitScreen();	// LCD �ʱ�ȭ��
    int DesFlow = -1;       // ��ǥ�� -1�� �ʱ�ȭ
    int CurFlow = 0;        // ������ 0���� �ʱ�ȭ
    
	while(1)
	{
		DesFlow = TargetFloor();            // ����ġ�� ��ǥ���� �Է¹��� (�Է��� ������ -1)
        if (DesFlow == -1) continue;        // �Է� ���� �ʾҴٸ� ó������
        if (DesFlow == CurFlow) continue;   // ��ǥ���� �������� ������ ���۾���
        
        UpdateDisplay(CurFlow, DesFlow);    // �Էµ� ��ǥ���� ������Ʈ
        DelayMS(1000);
        
        if (DesFlow > CurFlow) {                // ��ǥ���� ���������� ���� ���
          for (;CurFlow != DesFlow; CurFlow++){ // ��ǥ���� �������� ���� ������ �������� 1������Ű�� �ݺ�
            UpdateDisplay(CurFlow, DesFlow);    // ���÷����� �� ���� ������Ʈ
            UpdateLED(CurFlow);                 // LED�� �� ���� ������Ʈ
            DelayMS(1000);                      // 1�� ���
            }
        } else {                                // ��ǥ���� ���������� ���� ���
          for (;CurFlow != DesFlow; CurFlow--){ // ��ǥ���� �������� ���� ������ �������� 1���ҽ�Ű�� �ݺ�
            UpdateDisplay(CurFlow, DesFlow);    // ���÷����� �� ���� ������Ʈ
            UpdateLED(CurFlow);                 // LED�� �� ���� ������Ʈ
            DelayMS(1000);                      // 1�� ���
          }
        }
        DesFlow = -1;                           // ��ǥ���� -1�� �ʱ�ȭ
        UpdateDisplay(CurFlow, DesFlow);        // ���÷����� �� ���� ������Ʈ
        UpdateLED(CurFlow);                     // LED�� �� ���� ������Ʈ
        
        BEEPS(3);                                       // ���� 3ȸ
	}
}

/* GPIO (GPIOG(LED), GPIOH(Switch), GPIOF(Buzzer) �ʱ� ����	*/
void _GPIO_Init(void)
{
	// LED (GPIO G) ���� : Output mode
	RCC->AHB1ENR	|=  0x00000040;	// RCC_AHB1ENR : GPIOG(bit#6) Enable							
	GPIOG->MODER 	|=  0x00005555;	// GPIOG 0~7 : Output mode (0b01)						
	GPIOG->OTYPER	&= ~0x00FF;	// GPIOG 0~7 : Push-pull  (GP8~15:reset state)	
	GPIOG->OSPEEDR 	|=  0x00005555;	// GPIOG 0~7 : Output speed 25MHZ Medium speed 
   
	// SW (GPIO H) ���� : Input mode 
	RCC->AHB1ENR    |=  0x00000080;	// RCC_AHB1ENR : GPIOH(bit#7) Enable							
	//GPIOH->MODER 	&= ~0xFFFF0000;	// GPIOH 8~15 : Input mode (reset state)				
	GPIOH->PUPDR 	&= ~0xFFFF0000;	// GPIOH 8~15 : Floating input (No Pull-up, pull-down) :reset state

	// Buzzer (GPIO F) ���� : Output mode
	RCC->AHB1ENR	|=  0x00000020;	// RCC_AHB1ENR : GPIOF(bit#5) Enable							
	GPIOF->MODER 	|=  0x00040000;	// GPIOF 9 : Output mode (0b01)						
	GPIOF->OTYPER 	&= ~0x0200;	// GPIOF 9 : Push-pull  	
	GPIOF->OSPEEDR 	|=  0x00040000;	// GPIOF 9 : Output speed 25MHZ Medium speed 
}	

void _EXTI_Init(void) {
    RCC->AHB1ENR    |= 0x00000080;  // RCC_AHB1ENR : GPIOH enable
    RCC->APB2ENR    |= 0x00004000;  // Enable System Configuration Controller Clock
    
    GPIOH->MODER    &= ~0xC000000;  // GPIOH 15 : Input Mode
    SYSCFG->EXTICR[3]   |= 0x7000;  // EXTI15�� ���� �ҽ� �Է��� GPIOH�� ����, EXTI15 <- PH15
    
    EXTI->FTSR  |= 0x008000;        // EXTI15 : Falling Trigger Enable
    EXTI->IMR   |= 0x008000;        // EXTI15 : ���ͷ�Ʈ mask ����
    
    NVIC->ISER[1]   |= 0x0100;      // Enable 'Global Interrupt EXTI15'
}

void EXTI15_10_IRQHandler() {   // 10~10�� ���ͷ�Ʈ �߻�
    if (EXTI->PR & 0x8000) {    // SW7 ����
      EXTI->PR |= 0x8000;       // �ڵ鷯�� �ٽ� �۵����� �ʵ��� �ʱ�ȭ
      EXTI->IMR   &= ~0x008000;     // mask�� �ٽ� ���� �������� �߻��� ���ͷ�Ʈ ����
      GPIOG->ODR |= 0x0080;     // 7�� LED ON
      BEEPS(2);                    //  ���� 2ȸ
      DelayMS(5000);            // 5�� ���
      BEEPS(2);                     // ���� 2ȸ
      GPIOG->ODR &= ~0x0080;    // 7�� LED OFF
      EXTI->IMR   |= 0x008000;      // �ٽ� mask�� ����
    }
}

/* Switch�� �ԷµǾ������� ���ο� � switch�� �ԷµǾ������� ������ return�ϴ� �Լ�  */ 
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
	LCD_Clear(RGB_YELLOW);		        // ȭ�� Ŭ����
	LCD_SetFont(&Gulim8);		        // ��Ʈ : ���� 8
	LCD_SetBackColor(RGB_YELLOW);	    // ���ڹ��� : �����
	LCD_SetTextColor(RGB_BLACK);	    // ���ڻ� : Black
	LCD_DisplayText(0,0,"MECHA Elevator(JJW)");	// Title

	LCD_SetBackColor(RGB_YELLOW);	    //���ڹ��� : Yellow
          
	LCD_DisplayText(1,0,"Cur FL: ");    // ��°�ٿ� "Cur FL : "���
	LCD_DisplayText(2,0,"Des FL: ");    // ��°�ٿ� "Des FL : "���
    
    LCD_SetTextColor(RGB_RED);	        // ���ڻ� : Red
    LCD_DisplayChar(1,8, '0');          // ���� ��ġ�� �ʱ���ġ�� 0���� ���
	LCD_DisplayChar(2,8, '-');          // ��ǥ ��ġ�� �ʱⰪ�� -�� ���
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

void UpdateDisplay(int CurFlow, int DesFlow) { // ���÷��̿� ���� ���� ���� ǥ��
    if (CurFlow < 0 || CurFlow  > 6) { // �߸��� �������� �ԷµǸ� �� ��� ?ǥ���� �Լ� ����
      LCD_DisplayChar(1, 8, '?');
      return;
    }
    if (DesFlow < -1 || DesFlow  > 6) { // �߸��� ��ǥ���� �ԷµǸ� �� ��� ?ǥ���� �Լ� ����
      LCD_DisplayChar(2, 8, '?');
      return;
    }
    
    char Cflow = CurFlow + 0x30;    // �������� �������� ���������� ��ȯ
    char Dflow;
    if (DesFlow == -1) Dflow = '-'; // ��ǥ���� -1�̸�(�Է��� ����) ��ǥ�� ǥ�ú����� -����
    else Dflow = DesFlow + 0x30;    // ��ǥ���� -1�� �ƴϸ�(�Էµ�) �������� ��ǥ���� ���������� ��ȯ
    
    LCD_SetTextColor(RGB_RED);	    // ���ڻ� : Red
    LCD_DisplayChar(1,8,Cflow);     // ������ Display�� ���
	LCD_DisplayChar(2,8,Dflow);     // ��ǥ�� Display�� ���
}

void UpdateLED(int CurFlow) {               // �������� LED�� Ű�� �Լ�
    if (CurFlow > 6 || CurFlow < 0) return; // �������� �۵���ų �� �ִ� LED�� ������ �ʰ��ϸ� ����
    GPIOG->ODR &= ~0x00FF;                  // ��� LED OFF
    GPIOG->ODR |= (1<<CurFlow);             // ����Ʈ �����ڸ� ���� CurFlow��ġ�� LED ON
}

int TargetFloor(void) {     // ��ǥ���� 0~6�� ����ġ�� ���� �Է¹޾� ��ȯ�ϴ� �Լ�
    int DesFlow;            // ����ġ�� ���� �Էµ� ��ǥ���� ������ ����
    switch(KEY_Scan())
		{
			case 0xFE00  : 	//SW0 -> 0��
                DesFlow = 0;    // ��ǥ���� 0�� ����
				BEEP();
			break;
			case 0xFD00 : 	//SW1 -> 1��
                DesFlow = 1;    // ��ǥ���� 1�� ����
				BEEP();
			break;
            case 0xFB00 : 	//SW2 -> 2��
				DesFlow = 2;    // ��ǥ���� 2�� ����
                BEEP();
			break;
            case 0xF700 : 	//SW3 -> 3��
				DesFlow = 3;    // ��ǥ���� 3�� ����
                BEEP();
			break;
            case 0xEF00 : 	//SW4 -> 4��
				DesFlow = 4;    // ��ǥ���� 4�� ����
                BEEP();
			break;
            case 0xDF00 : 	//SW5 -> 5��
				DesFlow = 5;    // ��ǥ���� 5�� ����
                BEEP();
			break;
            case 0xBF00 : 	//SW6 -> 6��
				DesFlow = 6;    // ��ǥ���� 6�� ����
                BEEP();
			break;
            default:        // ����ġ ���Է�
                DesFlow = -1;   // ��ǥ�� ������ -1����
		}
    return DesFlow;         // ��ǥ�� ��ȯ
}

void BEEPS(int time) {              // ������ ������ �︮�� �Լ�
  if (time < 1) return;                 // ������ �︮�� Ƚ���� 0�����̸� �Լ� ����
  for (int i = 0; i < time -1; i++) { // ������ time������ �ϳ� ���� 400ms�������� �︰��
    BEEP();
    DelayMS(400);
  }
  BEEP();                                   // ������ �ѹ� �︰��. (�������� �����̸� ���� �ʱ� ����)
}