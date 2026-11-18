#include<LPC21XX.H>
#include<string.h>
#include "delay.h"
#define d 0xf<<14   // p0.14-p0.17(d4-d7) as data pins
#define rs 1<<12    // p0.12 as rs
#define e 1<<13    // p0.13 as enable
#define g_led 1<<2 // p0.2 as green led
#define r_led 1<<4 // p0.4 as red led
#define buzz 1<<5 // p0.5 as buzzer

//p0.3 as interrupt switch
//p0.8 & p0.9 as tx and rx of uart1 respectively

volatile unsigned char flag=0;

// gpio, interrupt, lcd, uart initialisation
void gpio_init(void);
void ext_init(void);
void lcd_init(void);
void uart0_config(void);
void uart1_config(void);
// lcd definitions
void lcd_command(unsigned char );
void lcd_data(unsigned char );
void lcd_str(unsigned char *);
void lcd_msg(unsigned char *);
// uart definitions
void uart0_tx(unsigned char);
unsigned char uart0_rx(void);
void uart0_str(unsigned char *);
void uart1_tx(unsigned char);
unsigned char uart1_rx(void);
void uart1_str(unsigned char *);
// gsm definitons
void sms_read(void);
void normal_alert(void);
void emergency_alert(void);

//vectored IRQ to turn off the buzzer
void ext1_isr(void)__irq
{
EXTINT=0X02;
IOSET0=buzz;
flag=1;
VICVectAddr=0;
}

int main()
{
gpio_init();
lcd_init();
uart0_config();
uart1_config();
ext_init();
lcd_command(0x80);
lcd_str("testing lcd");
uart0_str("system started\r\n");
uart1_str("AT\r\n");
delay_sec(1);
uart1_str("AT+CMGF=1\r\n");
delay_sec(1);
uart1_str("AT+CNMI=2,2,0,0,0\r\n");
delay_sec(1);
lcd_command(0x01);
lcd_command(0x80);
lcd_str("waiting for sms...");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                
while(1)
{
sms_read();
}
}
void ext_init(void)
{
PINSEL0|=0xc0; // p0.3 as EINT1 functionality
VICIntSelect=0x00000000;
VICVectCntl0=(1<<5)|15;
VICVectAddr0=(int)ext1_isr;
EXTMODE=0X00;
EXTPOLAR=0X00;
VICIntEnable=1<<15;
}

void gpio_init(void)
{
IODIR0|= g_led|r_led|buzz;
IOSET0|= g_led|r_led|buzz;
}

void normal_alert(void)
{
IOSET0=g_led|r_led|buzz;
IOCLR0=g_led;
}

void emergency_alert(void)
{
IOSET0=g_led|r_led|buzz;
IOCLR0=r_led|buzz;
}

void uart0_tx(unsigned char c)
{
while(((U0LSR>>5)&1)==0);
U0THR=c;
}

unsigned char uart0_rx(void)
{
while((U0LSR&1)==0);
return U0RBR;
}

void uart0_str(unsigned char *s)
{
while(*s)
{
uart0_tx(*s++);
delay_ms(2);
}
}

void lcd_init(void)
{
IODIR0|=d|rs|e;
lcd_command(0x01);
lcd_command(0x02);
lcd_command(0x0c);
lcd_command(0x28);
}

void lcd_command(unsigned char cmd)
{
IOCLR0=d;
IOSET0=(cmd & 0xf0)<<10;
IOCLR0=rs;
IOSET0=e;
delay_ms(2);
IOCLR0=e;

IOCLR0=d;
IOSET0=(cmd & 0x0f)<<14;
IOCLR0=rs;
IOSET0=e;
delay_ms(2);
IOCLR0=e;
}

void lcd_data(unsigned char data)
{
IOCLR0=d;
IOSET0=(data & 0xf0)<<10;
IOSET0=rs;
IOSET0=e;
delay_ms(2);
IOCLR0=e;

IOCLR0=d;
IOSET0=(data & 0x0f)<<14;
IOSET0=rs;
IOSET0=e;
delay_ms(2);
IOCLR0=e;
}

void lcd_str(unsigned char *s)
{
while(*s)
lcd_data(*s++);
}

void sms_read(void)
{
	const char key[] = "+CMT:";
    char ch;
    unsigned char msg[160];
    int i = 0;
    int k = 0;
    int state = 0;     

    while (1)
    {
        ch = uart1_rx();

        if (state == 0)                      
        {
            if (ch == key[k])
            {
                k++;
                if (key[k] == '\0')
                {
                    state = 1;
                    k = 0;
                }
            }
            else
            {
                k = (ch == '+') ? 1 : 0;
            }
        }
        else if (state == 1)                 
        {
            if (ch == '\n')
            {
                state = 2;
                i = 0;
            }
        }
        else                                 
        {
            if (ch == '*' || ch == '\n')    
            {
                if (i > 0)
                {
                    msg[i] = '\0';

                    lcd_msg(msg);

                    uart0_str((unsigned char *)"\r\nSMS RECEIVED : ");
                    uart0_str(msg);
                    uart0_str((unsigned char *)"\r\n");

                    if (strstr((char *)msg, "URGENT") ||
                        strstr((char *)msg, "EMERGENCY") ||
                        strstr((char *)msg, "ALERT"))
                    {
                        emergency_alert();
                    }
                    else
                    {
                        normal_alert();
                    }
                    return;                  
                }
            }
            else if (i < 159)
            {
                msg[i++] = ch;
            }
			}
			}
}

void uart0_config(void)
{
PINSEL0|=0x5;
U0LCR=0x83;
U0DLL=97;
U0DLM=0;
U0LCR=0x03;
}

void uart1_config(void)
{
PINSEL0|=0x50000;
U1LCR=0x83;
U1DLL=8;
U1DLM=0;
U1LCR=0x03;
}

void uart1_tx(unsigned char c)
{
while(((U1LSR>>5)&1)==0);
U1THR=c;
}

unsigned char uart1_rx(void)
{
while((U1LSR&1)==0);
return U1RBR;
}

void uart1_str(unsigned char *s)
{
while(*s)
{
uart1_tx(*s++);
}
}

void lcd_msg(unsigned char *s)
{
    int n = 0;
    lcd_command(0x01);
    delay_ms(2);
    lcd_command(0x80);
    while (*s && n < 32)
    {
        if (n == 16)
            lcd_command(0xC0);               
        lcd_data(*s++);
        n++;
    }
}
