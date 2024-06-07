#ifndef F_CPU
#define F_CPU 16000000
#endif // F_CPU

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdbool.h>

typedef struct {
    int ECHO_PIN;
    int TRIGGER_PIN;
    int ID;
} UltrasonicSensor;

typedef struct {
    int INPUT_PIN;
} ServoMotor;

UltrasonicSensor  US_1 = {PD2, PB3, 1};
UltrasonicSensor  US_2 = {PD3, PB5, 2};
ServoMotor        SERVO = {PB2};

void UART_init(unsigned int baud) {
    unsigned int ubrr = F_CPU/16/baud-1;
    UBRR0H = (unsigned char)(ubrr>>8);
    UBRR0L = (unsigned char)ubrr;
    UCSR0B = (1<<RXEN0)|(1<<TXEN0);
    UCSR0C = (1<<USBS0)|(3<<UCSZ00);
}

void UART_sendChar(unsigned char data) {
    while (!(UCSR0A & (1<<UDRE0)));
    UDR0 = data;
}

void UART_sendString(const char *str) {
    while (*str) {
        UART_sendChar(*str++);
    }
}

void init_ports() {
    // Outputs
    DDRB |= (1 << US_1.TRIGGER_PIN);
    DDRB |= (1 << US_2.TRIGGER_PIN);
    DDRB |= (1 << SERVO.INPUT_PIN);

    //Inputs
    DDRD &= ~(1 << US_1.ECHO_PIN);
    DDRD &= ~(1 << US_2.ECHO_PIN);
}

uint32_t measure_distance(UltrasonicSensor US, bool display_distance) {
    // Send a 10us pulse to trigger pin
    PORTB &= ~(1 << US.TRIGGER_PIN);
    _delay_us(2);
    PORTB |= (1 << US.TRIGGER_PIN);
    _delay_us(10);
    PORTB &= ~(1 << US.TRIGGER_PIN);

    // Measure the duration of the pulse from echo pin
    unsigned long duration = 0;
    unsigned long max_duration = F_CPU / 10;
    while (!(PIND & (1 << US.ECHO_PIN))) {
        if (duration++ >= max_duration) return -1;
        _delay_us(1);
    }
    TCNT1 = 0;
    TCCR1B |= (1 << CS11);
    while (PIND & (1 << US.ECHO_PIN)) {
        if (TCNT1 > max_duration) return -1;
    }
    TCCR1B &= ~(1 << CS11);

    // Calculate distance in centimeters
    float time_taken = TCNT1 * (8.0 / F_CPU);
    float distance = (time_taken * 34300.0) / 2.0;
    
    if (1) {
      display_distances(distance, US.ID);
    }
    
    return distance;
}

void display_distances(float distance, int id) {
    char buffer[32];
    char distance_buffer[16];
    
    if (distance == -1) {
        _delay_ms(500);
        return;
    }
    dtostrf(distance, 6, 2, distance_buffer);
    snprintf(buffer, sizeof(buffer), "Distance %d = %s cm\r\t", id, distance_buffer);
    UART_sendString(buffer);
}

void move_fan() {
    // Get distances (and displays them if boolean is true)
    float distance1 = measure_distance(US_1, true);
    float distance2 = measure_distance(US_2, true);

    int difference = (int)(distance1 - distance2);
    if (difference < 0) {
        difference = -difference;
    }
}

int main(void) {
    // Initialize ports and UART
    init_ports();
    UART_init(9600);
    
    while (1) {
      UART_sendString("\n");\
      move_fan();
    }
    
    return 0;
}
