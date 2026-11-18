void delay_ms(unsigned int );
void delay_sec(unsigned int );
void delay_ms(unsigned int ms)
{
    unsigned int i;
	for( ;ms>0;ms--)
	for(i=12000;i>0;i--);
}

void delay_sec(unsigned int s)
{
  unsigned int i;
  for( ;s>0;s--)
  for(i=12000000;i>0;i--);
}
