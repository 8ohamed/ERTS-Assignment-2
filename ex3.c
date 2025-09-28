#include "xparameters.h"
#include "xgpio.h"
#include "xscutimer.h"
#include "xil_types.h"
#include "xstatus.h"
#include "xil_printf.h"
#include "led_ip.h"


#define ONE_SECOND 325000000U

int main(void)
{
    int status;
    XGpio switches;
    XScuTimer Timer;
    // PS Timer related definitions
    XScuTimer_Config *ConfigPtr;
    unsigned int value;
    unsigned int skip;


    status = XGpio_Initialize(&switches, XPAR_SWITCHES_DEVICE_ID);
    if (status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    XGpio_SetDataDirection(&switches, 1, 0xFFFFFFFF);

    while (1) {
        xil_printf("CMD:> ");
        /* Read an input value from the console. */
        value  = inbyte();
        skip = inbyte(); //CR
        skip = inbyte(); //LF (Skip this line using PuTTY terminal)

        switch (value) {
            case '1': {
                // Read lower 4 bits of switches and write to LED IP
                unsigned int sw = XGpio_DiscreteRead(&switches, 1) & 0xF;
                LED_IP_mWriteReg(XPAR_LED_IP_S_AXI_BASEADDR, 0, sw);
                xil_printf("Set LEDs to %d (from SW0..SW3)\r\n", sw);
            } break;

            case '2': {
                // Initialize the timer
                ConfigPtr = XScuTimer_LookupConfig(XPAR_PS7_SCUTIMER_0_DEVICE_ID);
                status = XScuTimer_CfgInitialize(&Timer, ConfigPtr, ConfigPtr->BaseAddr);
                if (status != XST_SUCCESS) {
                    break;
                }
                XScuTimer_LoadTimer(&Timer, ONE_SECOND);
                // Set AutoLoad mode
                XScuTimer_EnableAutoReload(&Timer);
                // Start the timer
                XScuTimer_Start(&Timer);

                xil_printf("Starting binary counter on LEDs.\r\n");
                {
                    unsigned int val = 0;
                    while (1) {
                        // Check timer expired
                        if (XScuTimer_IsExpired(&Timer)) {
                            // clear status bit
                            XScuTimer_ClearInterruptStatus(&Timer);
                            LED_IP_mWriteReg(XPAR_LED_IP_S_AXI_BASEADDR, 0, val & 0xF);
                            val = (val + 1) & 0xF;
                        }
                    }
                }
                XScuTimer_Stop(&Timer);
            } break;

            default:
                xil_printf("Unknown command '%c' (ASCII 0x%X)\r\n", (char)value, value);
                break;
        }
    }

    return 0;
}
