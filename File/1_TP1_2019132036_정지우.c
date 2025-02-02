/////////////////////////////////////////////////////////////
// ��������Ʈ��: ����������
// ��������: �ΰ��� ���������͸� �ð�ȭ�Ͽ� ž������ 
//              ���� �� ���������� �� ����� ���������Ͱ� 
//              ���õǾ� ��ǥ������ �̵��Ͽ� ������ ����
//              �Ǿ ���� ������������ ������ �����ϴ� 
//             ���α׷� �ۼ�
// �����  �ϵ����(���): GPIO, SW, LED, Fram, EXTI
// ������: 2023. 6. 6
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

void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);

typedef struct Elevator{                        // ������������ ������ ������ �ִ� ����ü
  char num;                                             // ������������ �¿�����
  int location;                                             // ���������� LCD��� x��ǥ ����
  int pressent;                                         // ������
}Elevator;

void Move(Elevator*, int);                          // ���������� �����̱� �Լ�
void UpdateDisplay(Elevator*, char);     // ���÷��� ������Ʈ
void BEEPS(int);                                    // Ƚ���� �Է¹޾� Ƚ����ŭ ������ �︲

int _mode = 0;              // ���۸�� : 0:�����ø��, 1:������
int _select = 1;                // ���õ� ���������� : 0:left, 1:Right
int _target = 1;                // ��ǥ��
int _user_floor = 1;            // ž���� ��

int main(void)
{
  /*�ý��� �ʱ�ȭ*/
   LCD_Init();                       // LCD ��� �ʱ�ȭ
   _GPIO_Init();                    // GPIO �ʱ�ȭ
   _EXTI_Init();                    // EXTI �ʱ�ȭ
    Fram_Init();                    // FRAM �ʱ�ȭ H/W �ʱ�ȭ
	Fram_Status_Config();   // FRAM �ʱ�ȭ S/W �ʱ�ȭ
    DelayMS(10);
    
    /*����ϴ� Fram�ּҿ� ��ȿ�� ���� ������ 1�� �ʱ�ȭ*/
    if (Fram_Read(2023) < 1 || Fram_Read(2023) > 6) {Fram_Write(2023, 1);}
    if (Fram_Read(2024) < 1 || Fram_Read(2024) > 6) {Fram_Write(2024, 1);}
    
    /*Fram���� ���������� ���� ������ �޾� Elevator�ʱ�ȭ*/
   Elevator LElevator = {'L', 25, Fram_Read(2023)};
   Elevator RElevator = {'R', 115, Fram_Read(2024)};
  Elevator ele[] = {LElevator, RElevator};              // ���������͸� �����ϱ� ���ϵ��� �迭�� ����
  
  DisplayInitScreen();              // LCD �ʱ�ȭ��
  UpdateDisplay(ele, 'S');          // ���������� S���� ǥ���ϰ� ������ ���������� �׸���
  _select = 0;                          // ���� ���������� ����
  UpdateDisplay(ele, 'S');          // ���������͸�� S���ϰ� ���� ���������� �׸���

  GPIOG->ODR &= ~0x00FF;   // �ʱⰪ: LED0~7 Off
  GPIOG->ODR |= 0x0080;     // 7LED ON
   while(1) {
     if (_mode == 0) {                              // ������ ���
      switch (KEY_Scan()) {
      case 0xFD00:                                  // SW1 down
         GPIOG->ODR &= ~0x0004;             // LED3 OFF
         GPIOG->ODR |= 0x0002;                  // LED2 ON
         _user_floor++;                                 // ž���� ����
          if (_user_floor > 6) _user_floor =1;      // ž������ 6���� ũ�� 1�� �ʱ�ȭ
          UpdateDisplay(ele, 'S');                      // ���÷��� ������Ʈ
          BEEPS(1);                                     // ���� 1ȸ
       break;
      case 0xFB00:                                  // SW2 down
         GPIOG->ODR &= ~0x0002;             // LED2 OFF
         GPIOG->ODR |= 0x0004;                  // LED3 ON
         _target++;                                     // ��ǥ�� 1����
          if (_target > 6) _target = 1;                 // ��ǥ���� 6����  ũ�� 1�� �ʱ�ȭ
          UpdateDisplay(ele, 'S');                      //���÷��� ������Ʈ
          BEEPS(1);                                     // ���� 1ȸ
      break;
      }
     } else if (_mode == 1) {                       // ������
       int Ldiffer = abs(ele[0].pressent-_user_floor);  // ž������ ���� ���������� ���� ���
       int Rdiffer = abs(ele[1].pressent-_user_floor);  // ž������ ������ ���������� ���� ���
       if (Rdiffer >= Ldiffer) _select = 0;                     // ���� ���������Ͱ� �� �����ų� ������ ���� ���������� ����
       else _select = 1;                                            // �������������Ͱ� �� ������ �������������� ����
       DelayMS(500);                                                // 0.5�� ������
       Move(ele, _user_floor);                                  // ���õ� ���������͸� ž�������� �̵�
       UpdateDisplay(ele, 'S');                                     // ���������͸� stop���� �ϰ� ���÷���  ������Ʈ
       DelayMS(500);                                                // 0.5�� ������
       Move(ele, _target);                                          // ���������� ��ǥ������ �̵�
       
       Fram_Write(2023, ele[0].pressent);                   // ���� ������������ �������� Fram 2023������ ����
       Fram_Write(2024, ele[1].pressent);                   // ���� ������������ ����v�� Fram 2024������ ����
       BEEPS(3);                                                    // ���� 3ȸ
       GPIOG->ODR &= ~0x01;                                 // LED1 OFF
       GPIOG->ODR |= 0x80;                                      // LED7 ON
       _mode = 0;                                                   // ������ ���� ����
     }
  }
}

/* GLCD �ʱ�ȭ�� ���� �Լ� */
void DisplayInitScreen(void)
{
   LCD_Clear(RGB_WHITE);      // ȭ�� Ŭ����
   LCD_SetFont(&Gulim8);      // ��Ʈ : ���� 8
   LCD_SetBackColor(RGB_WHITE);   // ���ڹ��� : Green
   LCD_SetTextColor(RGB_BLACK);   // ���ڻ� : Black
   LCD_DisplayText(0,0,"MC-Elevator(JJW)");     // Title
  LCD_DisplayChar(6, 9, '>');                           //ȭ��ǥ ǥ��
  
  LCD_SetTextColor(RGB_GREEN);      // �ʷϻ� ����
  for (int i = 1; i < 7; i++) {                 // ���� �� ��ȣ ���
    char num = (7-i) + 0x30;                // ���ڸ� char�� �ٲ㰡�� ���
    LCD_DisplayChar(i+1,13, num);
  } 
  LCD_SetTextColor(RGB_BLUE);       // �Ķ��� ����
  LCD_DisplayText(4, 8, " -E");         // ���õ� ���������Ͱ� ǥ��� ���� ǥ��
  for (int i = 1; i < 7; i++) {                 // ���� ��ǥ��
    char num = (7-i) + 0x30;                // ���ڸ� char�� �ٲ㰡�� ���
    LCD_DisplayChar(i+1,5, num);
  }
}

/*���������� LCD������Ʈ*/
void UpdateDisplay(Elevator* ele, char direction)
{
   LCD_SetFont(&Gulim8);                        // ��Ʈ : ���� 8
   LCD_SetBackColor(RGB_WHITE);             // ���ڹ��� : Green
   LCD_SetTextColor(RGB_RED);                   // ���ڻ� : Black
  if (_mode == 0) LCD_DisplayText(2, 9, "FL");  // mode�� 0�϶� FLǥ��
  else if (_mode == 1) LCD_DisplayText(2, 9, "EX"); // mode�� 1�϶� EXǥ��
  else LCD_DisplayText(2, 9, "??");                         // mode�� �̻��� ���� ����� �� ??�� ���
  LCD_DisplayChar(6, 8, _user_floor + 0x30);            // ž���� ���
  LCD_DisplayChar(6, 10, _target + 0x30);               // ��ǥ�� ���
   LCD_DisplayChar(5,9,direction);                          // ���������� �̵����� ���
  LCD_SetTextColor(RGB_BLUE);                               // ���ڻ� �Ķ���
  LCD_DisplayChar(4, 8, ele[_select].num);              // ���õ� ���������Ͱ� �������� �������� ǥ��
  
  LCD_SetBrushColor(RGB_WHITE);                         // �귯���� ���
  LCD_DrawFillRect(ele[_select].location, 10, 11, 100); //���õ� ���������͸� ����簢���� �׷� ����

  if (_select == 0) LCD_SetBrushColor(RGB_BLUE);    // ���� ���������͸� �Ķ��� �귯��
  else LCD_SetBrushColor(RGB_GREEN);                // ���� ���������͸� �ʷϻ� �귯��

  // ���������Ϳ� ����� x��ǥ�� ������������ �������� ���� ���õ� ���������� �׸���
  LCD_DrawFillRect(ele[_select].location, 14 + 13*(7-ele[_select].pressent) ,10, 13*(ele[_select].pressent));
}

/* GPIO (GPIOG(LED), GPIOH(Switch), GPIOF(Buzzer)) �ʱ� ����   */
void _GPIO_Init(void)
{
   // LED (GPIO G) ���� : Output mode
   RCC->AHB1ENR   |=  0x00000040;   // RCC_AHB1ENR : GPIOG(bit#6) Enable                     
   GPIOG->MODER    |=  0x00005555;   // GPIOG 0~7 : Output mode (0b01)                  
   GPIOG->OTYPER   &= ~0x00FF;   // GPIOG 0~7 : Push-pull  (GP8~15:reset state)   
   GPIOG->OSPEEDR    |=  0x00005555;   // GPIOG 0~7 : Output speed 25MHZ Medium speed 
   
   // SW (GPIO H) ���� : Input mode 
   RCC->AHB1ENR    |=  0x00000080;   // RCC_AHB1ENR : GPIOH(bit#7) Enable                     
   GPIOH->MODER    &= ~0x003C0000;   // GPIOH 1,2 : Input mode (reset state)            
   GPIOH->PUPDR    &= ~0x003C0000;   // GPIOH 1,2 : Floating input (No Pull-up, pull-down) :reset state

   // Buzzer (GPIO F) ���� : Output mode
   RCC->AHB1ENR   |=  0x00000020;   // RCC_AHB1ENR : GPIOF(bit#5) Enable                     
   GPIOF->MODER    |=  0x00040000;   // GPIOF 9 : Output mode (0b01)                  
   GPIOF->OTYPER    &= ~0x0200;   // GPIOF 9 : Push-pull     
   GPIOF->OSPEEDR    |=  0x00004000;   // GPIOF 9 : Output speed 25MHZ Medium speed 
}   

/* EXTI (EXTI8(GPIOH.8, SW0), EXTI9(GPIOH.9, SW1)) �ʱ� ����  */
void _EXTI_Init(void)
{
   RCC->AHB1ENR    |= 0x00000080;   // RCC_AHB1ENR GPIOH Enable
   RCC->APB2ENR    |= 0x00004000;   // Enable System Configuration Controller Clock
   
   GPIOH->MODER    &= ~0xC0030000;   // GPIOH PIN8,PIN15 Input mode (reset state)             
   
   SYSCFG->EXTICR[2] |= 0x0007;   // EXTI8�� ���� �ҽ� �Է��� GPIOH�� ����
               // EXTI8 <- PH8,
               // EXTICR3(EXTICR[2])�� �̿� 
               // reset value: 0x0000   
   SYSCFG->EXTICR[3] |= 0x7000;//15

   EXTI->FTSR |= 0x008100;      // EXTI8: Falling Trigger Enable 
   EXTI->IMR  |= 0x008100;      // EXTI8,9 ���ͷ�Ʈ mask (Interrupt Enable) ����
      
   NVIC->ISER[0] |= (1<<23);   // Enable 'Global Interrupt EXTI8,9' == (1<<23)
  NVIC->ISER[1] |= (1<<(40-32));  // 40 == (1<<(40-32))
}

/* EXTI5~9 ���ͷ�Ʈ �ڵ鷯(ISR: Interrupt Service Routine) */
void EXTI9_5_IRQHandler(void)   // SW0
{      
   if(EXTI->PR & 0x0100)         // EXTI8 Interrupt Pending(�߻�) ����? -> ����ũ�� �����.
   {
      EXTI->PR |= 0x0100;      // Pending bit Clear (clear�� ���ϸ� ���ͷ�Ʈ ������ �ٽ� ���ͷ�Ʈ �߻�)
      EXTI->IMR &= ~0x000100;   // EXTI8���� ����ŷ�� �Ͽ� �ߺ����� �߻��� ���ͷ�Ʈ ����
      GPIOG->ODR &= ~0xFF;      // LED OFF
      GPIOG->ODR |= 0x01;           // 1�� LED ON
      _mode = 1;                            // ��带 1�� ����
      BEEPS(1);                             // ���� 1ȸ
      EXTI->IMR |= 0x000100;            // EXTI8 ����ũ ����
   }
}

/* EXTI10~15 ���ͷ�Ʈ �ڵ鷯(ISR: Interrupt Service Routine) */
void EXTI15_10_IRQHandler(void)     //SW7
{      
   if(EXTI->PR & 0x8000)         // EXTI15 Interrupt Pending(�߻�) ����? -> ����ũ�� �����.
   {
     EXTI->PR |= 0x8000;        // Pending bit Clear (clear�� ���ϸ� ���ͷ�Ʈ ������ �ٽ� ���ͷ�Ʈ �߻�)
     if (_mode == 1) {                  // ��� 1������ �۵��ϵ��� ��.
      EXTI->IMR &= ~0x8000;         // EXTI15���� ����ŷ�� �Ͽ� �ߺ����� �߻��� ���ͷ�Ʈ ����
      LCD_SetTextColor(RGB_RED);    // ���ڻ��� ����������
      LCD_DisplayText(2, 9, "HD");          // HDǥ��-���������� �ߴܸ�� ǥ��
      GPIOG->ODR |= 0x40;                   // LED6 ON
      GPIOG->ODR &= ~0x01;              // LED1 OFF
      BEEPS(10);                                // ���� 10ȸ + ������ 0.5�� x 9
      DelayMS(500);                             // ������ 0.5��  
      GPIOG->ODR &= ~0x40;   // LED6 OFF
      GPIOG->ODR |= 0x01;   // LED1 ON
      LCD_DisplayText(2, 9, "EX");          // EXǥ�� - ���������� ������ ǥ��
      EXTI->IMR |= 0x8000;                  // EXTI15 ����ũ ����
     }
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
         DelayMS(20);
          return key;
      }
   }
}

/* Buzzer: Beep for 30 ms */
void BEEPS(int num)         
{  
  for (int i = 0; i < num; i++) {   // �Ű������� �Էµ� Ƚ����ŭ �ݺ�
   GPIOF->ODR |=  0x0200;   // PF9 'H' Buzzer on
   DelayMS(30);      // Delay 30 ms
   GPIOF->ODR &= ~0x0200;   // PF9 'L' Buzzer off
   if (i+1 < num) DelayMS(500);     // ������Ƚ���� �����ϰ� ������ �︰�� 0.5�� ������
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

/* ���������͸� target�� ����������� �̵���Ű�� �Լ�*/
void Move(Elevator* ele, int target) {          
  while ((ele[_select].pressent) < target) {    // ���õ� ������������ �������� ��ǥ������ ���� ���
    UpdateDisplay(ele, 'U');                            // ���������Ͱ� ���� �̵������� ǥ��
    DelayMS(500);                                       // 0.5�� ���
    ele[_select].pressent++;                        // ���������� ���
    UpdateDisplay(ele, 'U');                            // �̵��� ���������� ǥ��
  }
  while ((ele[_select].pressent) > target) {   // ���õ� ������������ �������� ��ǥ������ ���� ���
    UpdateDisplay(ele, 'D');                        // ������Ƽ� �Ʒ��� �̵������� ǥ��
    DelayMS(500);                                   //0.5�� ���
    ele[_select].pressent--;                        // ���������� �ϰ�
    UpdateDisplay(ele, 'D');                            // �̵��� ���������� ǥ��
  }
  DelayMS(500);                                         // 0.5�� ���
  UpdateDisplay(ele, 'S');                              // ���������Ͱ� �����Ǿ����� ǥ��
}