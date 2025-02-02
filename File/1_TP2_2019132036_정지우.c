/////////////////////////////////////////////////////////////
// ��������Ʈ��: ��������
// ��������: 2bit�� ũ�⸦ ������ �ǿ����ڸ� 6���� ������ �� �ϳ��� �����Ͽ�
//                  ������ �� 4bit�� ���, ���Ӹ��� ������� ���������� +1
// �����  �ϵ����(���): GPIO, SW, LED, Fram, EXTI
// ������: 2023. 6. 11
// ������  Ŭ����: ȭ���Ϲ�
//           �й�: 2019132036
//           �̸�: ������
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

void UpdateDisplay(int); // LCD�� ǥ�õǴ� ������ LCD�� �׸��� �Լ�

uint8_t numA = 0;               // �ǿ����� 1
uint8_t numB = 0;               // �ǿ����� 2
uint8_t result;                 // �����
const char operatorlist[] = {'+', '-', 'x', '&', '|', '^'}; // �����ڸ� ������ �����ϱ� ���� ����Ʈ
uint8_t operator_ = 0;             // �������� �ε���
int flag = 0;                           // ������� 1�� ������Ű�� ��ƾ�� �����Ű�� flag

int main(void)
{
  /*�ý��� �ʱ�ȭ */
   LCD_Init();                                                          // LCD ��� �ʱ�ȭ
   _GPIO_Init();                                                        // GPIO �ʱ�ȭ
   _EXTI_Init();                                                          // EXTI �ʱ�ȭ
    Fram_Init();                                                            // FRAM �ʱ�ȭ H/W �ʱ�ȭ
	Fram_Status_Config();                                         // FRAM �ʱ�ȭ S/W �ʱ�ȭ
    DelayMS(10);
    
    /* ���� �� ���� �ʱ�ȭ*/
   DisplayInitScreen();                                             // LCD �ʱ�ȭ��
   
   int data = Fram_Read(530);                                   // Fram�� 530�������� �����͸� �ҷ��� 
   if (data > 5 || data < 0) {Fram_Write(530, 0);}      // Fram���� �ҷ��� ���� ��ȿ�� ���� �ƴϸ� Fram 530���� 0���� �ʱ�ȭ
   
   data = Fram_Read(531);                                       // Fram�� 531�������� �����͸� �ҷ���
   data &= ~0x80;                                                       // FRAM�� �������� 7����Ʈ�� 0���� (����� ������ 7����Ʈ�� ǥ����. ������ ��ǻ�Ϳ����� �̸� ���ڷ� �ν��ϱ� ������ ����ó���� ����)
   if (data < 0 || data > 15) {Fram_Write(531, 0);}     // Fram���� �ҷ��� ���� ��ȿ�� ���� �ƴϸ� Fram 531���� 0���� �ʱ�ȭ
   
   operator_ = Fram_Read(530);      // Fram�� 530�������� �����͸� �ҷ��� ������ �ε��� ������ ���� 
   result = Fram_Read(531);             // Fram�� 531�������� �����͸� �ҷ��� ����� ����
   
   UpdateDisplay(0);                        // LCD�� �ʱ� ������ ���
   
   /*Main ����*/
   while(1)
   {
     switch (KEY_Scan()) {              // SW �Է�
     case 0xFB00:                               //  SW2
       numA &= 0x03;                          // numA�� ������� �ʴ� ��Ʈ 0���� �ʱ�ȭ
       numA ^= 0x02;                            // ������Ʈ XOR
       BEEP();
       UpdateDisplay(0);                    // LCD ���
     break;
     case 0xF700:                               // SW3
       numA &= 0x03;                            // numA�� ������� �ʴ� ��Ʈ 0���� �ʱ�ȭ
       numA ^=0x01;                             // ������Ʈ XOR
       BEEP();
       UpdateDisplay(0);                        // LCD���
     break;
     case 0xEF00:                               // SW4
       numB &= 0x03;                            // numB�� ������� �ʴ� ��Ʈ 0���� �ʱ�ȭ
       numB ^=0x02;                             // ������Ʈ XOR
       BEEP();
       UpdateDisplay(0);                        //LCD���
     break;
     case 0xDF00:                               // SW5
       numB &= 0x03;                            // numB�� ������� �ʴ� ��Ʈ 0���� �ʱ�ȭ
       numB ^=0x01;                             // ������Ʈ XOR
       BEEP();
       UpdateDisplay(0);                        // LCD���
     break;
     }
   }
}

/* GLCD �ʱ�ȭ�� ���� �Լ� */
void DisplayInitScreen(void)
{
  LCD_Clear(RGB_WHITE);                          // ȭ�� Ŭ����
  LCD_SetBackColor(RGB_WHITE);          // ���ڹ��� : WHITE
  LCD_SetTextColor(RGB_BLACK);           // ���ڻ� : Black
  
  /*�����ȣ ���� ���*/
  LCD_SetBrushColor(GET_RGB(255, 0, 255));  // ��ȫ�� �귯�� ����
  LCD_SetPenColor(RGB_BLACK);                   // ������ ����
  LCD_DrawFillRect(70, 53, 20, 25);                     // �����ȣ ���� ���
  LCD_DrawRectangle(70, 53, 20, 25);
  
  /*���� ���� ���*/
  LCD_SetPenColor(RGB_GREEN);                   // �ʷ��� ����
  LCD_DrawRectangle(54, 15, 50, 100);
  
  /*�ǿ����� ���� ���� ���*/
  LCD_DrawRectangle(5, 31, 20, 20);             // A�ǿ����� ǥ�� ���� ���
  LCD_DrawRectangle(5, 81, 20, 20);             // B�ǿ����� ǥ�� ���� ���
  LCD_DrawRectangle(137, 55, 20, 20);           // ����� ǥ�� ���� ���
  LCD_SetFont(&Gulim10);                                // ����ũ�� 10����
  
  LCD_DrawChar(11, 33, 'A');            // A���
  LCD_DrawChar(11, 83, 'B');            // B���
  LCD_DrawChar(142, 57, 'C');           // C���
  
  /*�� ���*/
  LCD_SetPenColor(RGB_BLACK);                   // ������ ����
  LCD_DrawHorLine(105, 22, 14);                     // A MSB �� ���
  LCD_DrawHorLine(40, 32, 14);                      // A LSB �� ���
  LCD_DrawHorLine(40, 52, 14);          
  LCD_DrawHorLine(40, 82, 14);
  LCD_DrawHorLine(40, 102, 14);
  for (int i = 0; i < 4; i++) LCD_DrawHorLine(105, 47 + 20 * i, 14);
  
  /*�ǿ����� �� ��� ���� ���*/
  LCD_SetBrushColor(RGB_YELLOW);            // ����� �� ����
  for (int i = 0; i < 51; i = i+50) {                       // �� �ǿ����� ���� ���
    LCD_DrawFillRect(28, 24 + i, 15, 15);
    LCD_DrawFillRect(28, 44 + i, 15, 15);
    LCD_DrawRectangle(28, 24 + i, 15, 15);
    LCD_DrawRectangle(28, 44 + i, 15, 15);
  }
  for (int i = 0; i  < 4; i++) {                            // ����� ���� ���
    LCD_DrawFillRect(119, 40 + 20 * i, 15, 15);
    LCD_DrawRectangle(119, 40 + 20 * i, 15, 15);
  }
  LCD_DrawFillRect(70, 97, 20, 15);                 //  ���Ӹ�� ���� ���
  LCD_DrawRectangle(70, 97, 20, 15);
  
  LCD_DrawFillRect(119, 15, 15, 15);                // ��� ��ȣ ���� ���
  LCD_DrawRectangle(119, 15, 15, 15);
  LCD_SetFont(&Gulim8);                                             // ��Ʈ : ���� 8
  LCD_SetBackColor(RGB_YELLOW);                             // ���� ���� �����
  LCD_DrawChar(72, 99, '+');                            // ���Ӹ���� +���
}

void UpdateDisplay(int mode) {
  LCD_SetFont(&Gulim10);                                // ��Ʈ : ���� 10
  LCD_SetBackColor(GET_RGB(255, 0, 255));   //���ڹ��� ��ȫ��
  LCD_DrawChar(76, 56, operatorlist[operator_]); // ������ ���
  
  LCD_SetFont(&Gulim8);                                             // ��Ʈ : ���� 8
  LCD_SetBackColor(RGB_YELLOW);                             // ���� ���� �����
  LCD_DrawChar(32, 26, ((numA & 0x02)>>1) + 0x30);      // numA�� ������Ʈ ���
  LCD_DrawChar(32, 46, ((numA & 0x01)>>0) + 0x30);      // numA�� ������Ʈ ���
  LCD_DrawChar(32, 76, ((numB & 0x02)>>1) + 0x30);      // numB�� ������Ʈ ���
  LCD_DrawChar(32, 96, ((numB & 0x01)>>0) + 0x30);      // numB�� ������Ʈ ���
  char sig;                                                                     //  ��ȣ�� ����� ���� ����
  if ((result & 0x80)>>7 == 1) sig = '-';                               // ����� �ֻ��� ��Ʈ�� 1�� ������ sig�� -����
  else sig =  '+';                                                               // ����� �ֻ��� ��Ʈ�� 0�� ������ sig�� +����
  LCD_DrawChar(123, 17, sig);                                      // ����� ��ȣ ���
  LCD_DrawChar(123, 42, ((result & 0x08)>>3) + 0x30);   // ����� ������Ʈ  ���
  LCD_DrawChar(123, 62, ((result & 0x04)>>2) + 0x30);   // ����� �������� 2��° ��Ʈ ���
  LCD_DrawChar(123, 82, ((result & 0x02)>>1) + 0x30);   // ����� �������� 3��° ��Ʈ ���
  LCD_DrawChar(123, 102, ((result & 0x01)>>0) + 0x30);  // ����� ������Ʈ ���
  
  LCD_DrawChar(80, 99, mode + 0x30);                            // ���Ӹ�� ���ϴ� ���� ���
}

/* GPIO (GPIOG(LED), GPIOH(Switch), GPIOF(Buzzer)) �ʱ� ����   */
void _GPIO_Init(void)
{
   // LED (GPIO G) ���� : Output mode
   RCC->AHB1ENR   |=  0x00000040;           // RCC_AHB1ENR : GPIOG(bit#6) Enable                     
   GPIOG->MODER    |=  0x00005555;          // GPIOG 0~7 : Output mode (0b01)                  
   GPIOG->OTYPER   &= ~0x00FF;              // GPIOG 0~7 : Push-pull  (GP8~15:reset state)   
   GPIOG->OSPEEDR    |=  0x00005555;        // GPIOG 0~7 : Output speed 25MHZ Medium speed 
   
   // SW (GPIO H) ���� : Input mode 
   RCC->AHB1ENR    |=  0x00000080;          // RCC_AHB1ENR : GPIOH(bit#7) Enable                     
   GPIOH->MODER    &= ~0x003C0000;          // GPIOH 1,2 : Input mode (reset state)            
   GPIOH->PUPDR    &= ~0x003C0000;          // GPIOH 1,2 : Floating input (No Pull-up, pull-down) :reset state

   // Buzzer (GPIO F) ���� : Output mode
   RCC->AHB1ENR   |=  0x00000020;           // RCC_AHB1ENR : GPIOF(bit#5) Enable                     
   GPIOF->MODER    |=  0x00040000;          // GPIOF 9 : Output mode (0b01)                  
   GPIOF->OTYPER    &= ~0x0200;             // GPIOF 9 : Push-pull     
   GPIOF->OSPEEDR    |=  0x00004000;      // GPIOF 9 : Output speed 25MHZ Medium speed 
}   

/* EXTI (EXTI8(GPIOH.8, SW0), EXTI9(GPIOH.9, SW1)) �ʱ� ����  */
void _EXTI_Init(void)
{
   RCC->AHB1ENR    |= 0x00000180;   // RCC_AHB1ENR GPIOH,GPIOI Enable
   RCC->APB2ENR    |= 0x00004000;   // Enable System Configuration Controller Clock
   
   GPIOH->MODER    &= ~0x300C0000;   // GPIOH PIN9,PIN14 Input mode (reset state)             
   GPIOI->MODER &= ~0x00033000;         // GPIOI PIN6, pin8 Inpput mode

  SYSCFG->EXTICR[1] |= 0x0800;      // EXTI6�� ���� �ҽ� �Է��� GPIOI�� ����
   SYSCFG->EXTICR[2] |= 0x0078;   // EXTI9�� ���� �ҽ� �Է��� GPIOH�� ����, EXTI8�� ���� �ҽ� �Է��� GPIOI�� ����
   SYSCFG->EXTICR[3] |= 0x0700;     // EXTI14�� ���� �ҽ� �Է��� GPIOH�� ����

   EXTI->FTSR |= 0x004340;              // EXTI6,8,9,14: Falling Trigger Enable 
   EXTI->IMR  |= 0x004340;              // EXTI8,9 ���ͷ�Ʈ mask (Interrupt Enable) ����
      
   NVIC->ISER[0] |= (1<<23);            // Enable 'Global Interrupt EXTI6,8' == (1<<23)
  NVIC->ISER[1] |= (1<<(40-32));        // 40 == (1<<(40-32))
  
  NVIC->IP[23] = 0xF0;                      // EXTI6 �켱 ���� ��������
  NVIC->IP[40] = 0xE0;                      // EXTI14 �켱 ���� ���Ͽ��� �ι�°��
}

/* EXTI5~9 ���ͷ�Ʈ �ڵ鷯(ISR: Interrupt Service Routine) */
void EXTI9_5_IRQHandler(void)   // EXTI6, EXTI8, EXIT9 Handler
{      
   if(EXTI->PR & 0x0040)                    // EXTI6 Interrupt Pending(�߻�) ���� //J_UP
   {
      EXTI->PR |= 0x0040;                   // Pending bit Clear
      EXTI->IMR &= ~0x4000;             // ����ũ�� ���� �������� �߻��� ���ͷ�Ʈ  ����
      flag = 1;                                     // ���Ӹ�� flag�� 1���Ͽ� ON
      GPIOG->ODR |= 0x80;               // 7LED ON
      EXTI->IMR |= 0x4000;                  // EXTI6 ����ũ ����
      result &= ~0x80;
      while(flag) {                                 // ���� ���
        result++;                                   //��� 1����
        if (result & 0x10) result = 0;      // ����� ������Ʈ�κ��� 5��° ��Ʈ�� 1�̵Ǹ� ����� 0���� �ʱ�ȭ
        Fram_Write(531, result);            // Fram 531������ ��� ����
        UpdateDisplay(1);                   // ���÷��̿� ���Ӹ�� ���ڸ� 1�� �ٲٰ� LCD������Ʈ
        BEEP();
        DelayMS(500);
      }
      UpdateDisplay(0);                   // ���÷��̿� ���Ӹ�� ���ڸ� 0�� �ٲٰ� LCD������Ʈ
      BEEP();
      DelayMS(500);
      BEEP();
      DelayMS(500);
      BEEP();
      GPIOG->ODR &= ~0x80;    // 7LED OFF
   } 
  else if (EXTI->PR & 0x0100)       //EXTI8 Interrupt Pending(�߻�) ���� //J_RIGHT
  {
    EXTI->PR |= 0x0100;                 // Pending bit Clear
    EXTI->IMR &= ~0x0100;// ����ũ�� ���� �������� �߻��� ���ͷ�Ʈ  ����
    operator_++;                                //������ �ε��� 1����
    if (operator_ > 5) operator_ = 0;   // ������ �ε����� 5���� ũ�� 0����  �ʱ�ȭ
    Fram_Write(530, operator_);         // ������ �ε��� Fram530������ ����
    UpdateDisplay(0);                       // ���÷��� ������Ʈ
    BEEP();
    EXTI->IMR |= 0x0100;// EXTI8 ����ũ ����
  }
  else if (EXTI->PR & 0x0200)       //EXTI9 Interrupt Pending(�߻�) ���� // SW1
  {
    EXTI->PR |= 0x0200;                 // Pending bit Clear
    EXTI->IMR &= ~0x0200;               // ����ũ�� ���� �������� �߻��� ���ͷ�Ʈ  ����
    BEEP();
    result = 0;                                     // ����� 0���� �ʱ�ȭ (��ȣ �� ������ �ʱ�ȭ)
    switch (operator_) {                    // ������ �ε����� ���� ���� �з�
    case 0:                                         // �ε��� 0 , +
      result = numA + numB;                 //numA�� numB�� ���� ����� ����
      break;
    case 1:                                         // �ε��� 1 , -                      
      result = abs(numA - numB);        // ����� numA�� numB�� ���� ����
      if ((numA- numB) < 0)               // ���⸦ �ϸ� �����̸� ��� ������ 7�� ��Ʈ�� 1�� ��.
        result |= 0x80;
      break;
    case 2:                                         // �ε��� 2 , *
      result = numA * numB;                 // numA�� numB�� ���� ����� ����
      break;    
    case 3:                                         // �ε��� 3 , &
      result = numA & numB;                 // numA�� numB�� and�����Ͽ� ����� ����
      break;
    case 4:                                         // �ε��� 4 , |
      result = numA | numB;                 // numA�� numB or �����Ͽ� ����� ����
      break;
    case 5:                                         //�ε��� 5 , ^
      result = numA ^ numB;                 // numA�� numB�� XOR �����Ͽ� ����� ����
      break;
    }
    Fram_Write(531, result);                // Fram 531������ ��� ����
    UpdateDisplay(0);                           // ���÷��� ������Ʈ
    EXTI->IMR |= 0x0200;                    // EXTI9 ����ũ ����
  }
}

/* EXTI10~15 ���ͷ�Ʈ �ڵ鷯(ISR: Interrupt Service Routine) */
void EXTI15_10_IRQHandler(void) 
{                              // SW6
  
   if(EXTI->PR & 0x4000)         // EXTI14 Interrupt Pending(�߻�) ����?
   {
    EXTI->PR |= 0x4000;         // Pending bit Clear 
    EXTI->IMR &= ~0x0040;       // ����ũ�� ���� �������� �߻��� ���ͷ�Ʈ ����

    flag = 0;                               // ���Ӹ�� flag�� 0 ->����
    BEEP();
    DelayMS(1000);                  // 1�� ���
    EXTI->IMR |= 0x0040;          // EXTI14 ����ũ ����
   }
}

/* Switch�� �ԷµǾ����� ���ο� � switch�� �ԷµǾ������� ������ return�ϴ� �Լ�  */ 
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