// SMART STICK FOR BLIND PEOPLE

#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>

#define LCD_PORT PORTF
#define LCD_DDR  DDRF
#define RS  PK0
#define RW  PK1
#define EN  PK2


// ---------- Ultrasonic Sensor ----------
#define TRIG_PIN  PB6
#define ECHO_PIN  PB7


// ---------- Other Components ----------
#define BUZZER_PIN  PB5
#define LED_PIN     PB4
#define BUTTON_PIN  PD2  
#define LDR_CHANNEL 0   


// ---------- Function Prototypes ----------
void lcd_command(unsigned char cmd);
void lcd_data(unsigned char data);
void lcd_init();
void lcd_string(const char *str);
void lcd_gotoxy(uint8_t x, uint8_t y);
void adc_init();
uint16_t adc_read(uint8_t ch);
void ultrasonic_init();
uint16_t get_distance();
void lcd_print_num(uint16_t num);
void lcd_clear();


// ---------- MAIN ----------
int main(void)
{
    // I/O Configurations
    DDRB |= (1<<TRIG_PIN) | (1<<BUZZER_PIN) | (1<<LED_PIN);
    DDRD |= (1<<RS);
    DDRA |= (1<<EN);      
    LCD_DDR = 0xFF;      
    DDRB &= ~(1<<ECHO_PIN); 
    PORTB &= ~(1<<ECHO_PIN);


    // Configure button pin with pull-up
    DDRD &= ~(1<<BUTTON_PIN); 
    PORTD |= (1<<BUTTON_PIN); 


    // Initialize peripherals
    lcd_init();
    adc_init();
    ultrasonic_init();

    lcd_string("SMART STICK INIT");
    _delay_ms(1500);
    lcd_clear();

    uint16_t distance = 0;
    uint16_t ldr_value = 0;
    uint8_t button_state = 0;

    while (1)
    {
        // ---------- Read Button ----------
        if (!(PIND & (1<<BUTTON_PIN))) 
            button_state = 1;
        else
            button_state = 0;


        // ---------- Ultrasonic Distance ----------
        distance = get_distance();


        // ---------- LDR Reading ----------
        ldr_value = adc_read(LDR_CHANNEL);


        // ---------- LED Auto Light ----------
        if (ldr_value > 40)
            PORTB |= (1<<LED_PIN);   
        else
            PORTB &= ~(1<<LED_PIN);


        // ---------- Buzzer Control via Button ----------
        // ---------- Buzzer Control (Auto + Manual) ----------
if (distance < 10 || button_state)    
    PORTB |= (1<<BUZZER_PIN);       
else
    PORTB &= ~(1<<BUZZER_PIN);     


        // ---------- Display Data on LCD ----------
        lcd_clear();

        // Line 1: LED Status
        lcd_gotoxy(0, 0);
        if (ldr_value > 40)
            lcd_string("LED: ON ");
        else
            lcd_string("LED: OFF");

lcd_gotoxy(0, 1);
if (distance < 10 || button_state)
    lcd_string("Buzzer: ON ");
else
    lcd_string("Buzzer: OFF");


        _delay_ms(300);
    }
}


// ---------- LCD FUNCTIONS ----------
void lcd_command(unsigned char cmd)
{
    LCD_PORT = cmd;
    PORTK &= ~(1<<RS); 
    PORTK &= ~(1<<RW);   
    PORTK |=  (1<<EN);  
    _delay_us(1);
    PORTK &= ~(1<<EN);
    _delay_ms(2);
}

void lcd_data(unsigned char data)
{
    LCD_PORT = data;
    PORTK |=  (1<<RS);   
    PORTK &= ~(1<<RW);  
    PORTK |=  (1<<EN);  
    _delay_us(1);
    PORTK &= ~(1<<EN);
    _delay_ms(2);
}

void lcd_init()
{
    LCD_DDR = 0xFF;              
    DDRK |= (1<<RS)|(1<<RW)|(1<<EN); 

    _delay_ms(50);
    lcd_command(0x38); 
    _delay_ms(5);
    lcd_command(0x0C);  
    lcd_command(0x01);  
    _delay_ms(2);
    lcd_command(0x06);
    lcd_command(0x80); 
}


void lcd_string(const char *str)
{
    while(*str) {
        lcd_data(*str++);
    }
}

void lcd_gotoxy(uint8_t x, uint8_t y)
{
    uint8_t addr = (y == 0) ? 0x80 + x : 0xC0 + x;
    lcd_command(addr);
}

void lcd_print_num(uint16_t num)
{
    char buffer[6];
    itoa(num, buffer, 10);
    lcd_string(buffer);
}

void lcd_clear()
{
    lcd_command(0x01);
    _delay_ms(2);
}


// ---------- ADC FUNCTIONS ----------
void adc_init()
{
    ADMUX = (1<<REFS0); // AVcc as reference
    ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0); // Enable ADC, Prescaler 128
}

uint16_t adc_read(uint8_t ch)
{
    ch &= 0x07; // 0-7 channels
    ADMUX = (ADMUX & 0xF8) | ch;
    ADCSRA |= (1<<ADSC);
    while (ADCSRA & (1<<ADSC));
    return ADC;
}


// ---------- ULTRASONIC FUNCTIONS ----------
void ultrasonic_init()
{
    DDRB |= (1<<TRIG_PIN);
    DDRB &= ~(1<<ECHO_PIN);
}

uint16_t get_distance()
{
    uint16_t count;
    double distance;

    // Send trigger pulse
    PORTB |= (1<<TRIG_PIN);
    _delay_us(10);
    PORTB &= ~(1<<TRIG_PIN);

    // Wait for echo start
    while (!(PINB & (1<<ECHO_PIN)));
    TCNT1 = 0;
    TCCR1B = (1<<CS11); // Start timer with prescaler 8

    // Wait for echo end
    while (PINB & (1<<ECHO_PIN));
    TCCR1B = 0; // Stop timer

    count = TCNT1;
    distance = (count * 0.008575); // Convert to cm
    return (uint16_t)distance;
}
