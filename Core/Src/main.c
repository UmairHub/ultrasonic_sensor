
#include "main.h"

void TIM2_Init_Microsecond(void);
uint32_t HCSR04_ReadEcho(void);
void HCSR04_GPIO_Init(void);
uint32_t HCSR04_ReadDistanceCM(void);


void UART2_Init(void);
void UART2_SendChar(char c);
void UART2_SendString(char *str);
void UART2_SendInt(int32_t num);

int main(void)
{
    HCSR04_GPIO_Init();
    TIM2_Init_Microsecond();
    UART2_Init();

    while (1)
    {
        uint32_t distance = HCSR04_ReadDistanceCM();

        UART2_SendString("Distance: ");
        UART2_SendInt(distance);
        UART2_SendString(" cm\r\n");

        for (volatile int d = 0; d < 800000; d++);  // ~50ms between readings
    }
}

void TIM2_Init_Microsecond(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    // At 16MHz APB1 (no prescaler on APB1 timers when APB1 prescaler = 1),
    // TIM2 clock = 16MHz. Prescaler to get 1MHz (1us per tick):
    TIM2->PSC = 15;      // 16MHz / (15+1) = 1MHz -> 1 tick = 1us
    TIM2->ARR = 0xFFFFFFFF;  // TIM2 is 32-bit on F4, let it run free, max range
    TIM2->CNT = 0;
    TIM2->CR1 |= TIM_CR1_CEN;  // start counting
}

void HCSR04_GPIO_Init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;

    // PB0 = TRIG (output)
    GPIOB->MODER &= ~(3 << (0*2));
    GPIOB->MODER |=  (1 << (0*2));   // output mode

    // PB1 = ECHO (input) -- remember: needs voltage divider from 5V to 3.3V
    GPIOB->MODER &= ~(3 << (1*2));   // input mode (00, already default but explicit)
}

void HCSR04_Trigger(void)
{
    GPIOB->BSRR = (1 << 0);              // TRIG high
    uint32_t start = TIM2->CNT;
    while ((TIM2->CNT - start) < 10);    // wait 10us precisely
    GPIOB->BSRR = (1 << (0 + 16));       // TRIG low
}

uint32_t HCSR04_ReadDistanceCM(void)
{
    HCSR04_Trigger();

    uint32_t timeout_start = TIM2->CNT;
    while (!(GPIOB->IDR & (1 << 1)))     // wait for ECHO to go high
    {
        if ((TIM2->CNT - timeout_start) > 30000) return 0xFFFFFFFF; // timeout ~30ms, no echo
    }

    uint32_t echo_start = TIM2->CNT;

    while (GPIOB->IDR & (1 << 1))        // wait while ECHO stays high
    {
        if ((TIM2->CNT - echo_start) > 30000) return 0xFFFFFFFF; // timeout
    }

    uint32_t echo_end = TIM2->CNT;
    uint32_t duration_us = echo_end - echo_start;

    return duration_us / 58;   // convert to cm
}




void UART2_Init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    // PA2 = TX, PA3 = RX, Alternate Function
    GPIOA->MODER &= ~(3 << (2*2)); GPIOA->MODER |= (2 << (2*2));
    GPIOA->MODER &= ~(3 << (3*2)); GPIOA->MODER |= (2 << (3*2));

    GPIOA->AFR[0] &= ~(0xF << (2*4));
    GPIOA->AFR[0] |=  (7   << (2*4));   // AF7 = USART2 on PA2
    GPIOA->AFR[0] &= ~(0xF << (3*4));
    GPIOA->AFR[0] |=  (7   << (3*4));   // AF7 = USART2 on PA3

    USART2->CR1 = 0;                     // disable while configuring
    // Assumes APB1 clock = 16MHz
    // Baud = 115200 -> USARTDIV = APB1clk / (16 * baud)
    USART2->BRR = 0x8B;  // 16MHz APB1 clock at 9600 baud
    USART2->CR1 = 0x0008; //enable Tx, 8-bit data
    USART2->CR2 = 0x0;   // 1 stop bit
    USART2->CR3 = 0x0;   //no flow control

    USART2->CR1 |= USART_CR1_TE;         // enable transmitter
    USART2->CR1 |= USART_CR1_UE;         // enable USART
}

void UART2_SendChar(char c)
{
    while (!(USART2->SR & USART_SR_TXE));
    USART2->DR = c;
}

void UART2_SendString(char *str)
{
    while (*str)
        UART2_SendChar(*str++);
}
void UART2_SendInt(int32_t num)
{
    char buf[12];
    int i = 0;
    uint8_t neg = 0;

    if (num < 0) { neg = 1; num = -num; }
    if (num == 0) buf[i++] = '0';

    while (num > 0)
    {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    }
    if (neg) buf[i++] = '-';

    while (i > 0)
        UART2_SendChar(buf[--i]);
}
