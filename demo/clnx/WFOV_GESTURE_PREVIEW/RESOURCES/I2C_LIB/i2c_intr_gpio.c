#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <wpiExtensions.h>
#include <cstdint>

extern "C"
{

volatile bool intr_asserted = false;

static void intr_handler (void) 
{ 
    //printf("IRQ\n");
    intr_asserted = true;
}

void init_i2c_intr_gpio( uint8_t IRQpin )
{
    int major, minor;
    intr_asserted = false;

    wiringPiVersion(&major, &minor);

    printf("\nISR test (WiringPi %d.%d)\n", major, minor);

    wiringPiSetupPinType(WPI_PIN_BCM);
    
    pinMode(IRQpin, INPUT);
    pullUpDnControl(IRQpin, PUD_UP);
    printf("Testing IRQ @ GPIO%d \n", IRQpin);
    int err = wiringPiISR (IRQpin, INT_EDGE_FALLING , &intr_handler);
    printf("%s IRQ @ GPIO%d \n", (err == 0) ? "Succeeded" : "Failed", IRQpin);
}

bool is_i2c_intr_asserted()
{
    return intr_asserted;
    //return true;
}

void clear_i2c_intr_asserted()
{
    intr_asserted = false;
}

}
