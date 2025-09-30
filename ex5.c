#include "xil_printf.h"
#include "xil_io.h"
#include "xparameters.h"
#include "xscutimer.h"
#include <string.h>
#include "matrix_ip.h"

#define MSIZE 4
#define TIMER_FREQ 325000000U // 1 second = 325M ticks for Cortex-A9 private timer







typedef union {
    unsigned char comp[MSIZE];
    unsigned int vect;
} vectorType;

typedef vectorType VectorArray[MSIZE];

// 4.1: Global matrices
VectorArray aInst;
VectorArray bTInst;
VectorArray pInst;

// 4.2
void setInputMatrices(VectorArray A, VectorArray B) {
    unsigned char Aval[MSIZE][MSIZE] = {
        { 1,  2,  3,  4 },
        { 5,  6,  7,  8 },
        { 9, 10, 11, 12},
        {13, 14, 15, 16}
    };

    unsigned char Bval[MSIZE][MSIZE] = {
        {1, 1, 1, 1},
        {2, 2, 2, 2},
        {3, 3, 3, 3},
        {4, 4, 4, 4}
    };

    // Assign A
    for (int r = 0; r < MSIZE; r++) {
        for (int c = 0; c < MSIZE; c++) {
            A[r].comp[c] = Aval[r][c];
        }
    }

    // Assign B transpose
    for (int r = 0; r < MSIZE; r++) {
        for (int c = 0; c < MSIZE; c++) {
            B[r].comp[c] = Bval[c][r];
        }
    }
}

// 4.3
void displayMatrix(VectorArray M) {
    for (int r = 0; r < MSIZE; r++) {
        for (int c = 0; c < MSIZE; c++) {
            xil_printf("%3d ", M[r].comp[c]);
        }
        xil_printf("\r\n");
    }
    xil_printf("\r\n");
}

// 4.4
void multiMatrixSoft(VectorArray A, VectorArray B, VectorArray P) {
    for (int r = 0; r < MSIZE; r++) {
        for (int c = 0; c < MSIZE; c++) {
            unsigned int sum = 0;
            for (int k = 0; k < MSIZE; k++) {
                sum += (unsigned int)A[r].comp[k] * (unsigned int)B[c].comp[k];
            }
            P[r].comp[c] = (unsigned char)(sum & 0xFF); // Because during multiplication and summing, sum becomes a larger 32-bit int,
            												//and we only want the firs 8-bit (o-255)

        }
    }
}


void multiMatrixHard(VectorArray A, VectorArray B, VectorArray P) {
    for (int r = 0; r < MSIZE; r++) {
        for (int c = 0; c < MSIZE; c++) {
            // Write inputs (each row packed into 32-bit)
        	Xil_Out32(XPAR_MATRIX_IP_0_S00_AXI_BASEADDR + MATRIX_IP_S00_AXI_SLV_REG0_OFFSET, A[r].vect);
        	Xil_Out32(XPAR_MATRIX_IP_0_S00_AXI_BASEADDR + MATRIX_IP_S00_AXI_SLV_REG1_OFFSET, B[c].vect);

            // Read result (dot product of 4 bytes)
        	P[r].comp[c] = Xil_In32(XPAR_MATRIX_IP_0_S00_AXI_BASEADDR + MATRIX_IP_S00_AXI_SLV_REG2_OFFSET);
        }
    }
}







int main(void) {
    XScuTimer Timer;
    XScuTimer_Config *ConfigPtr;
    int status;

    // Initialize SCU private timer
    ConfigPtr = XScuTimer_LookupConfig(XPAR_PS7_SCUTIMER_0_DEVICE_ID);
    status = XScuTimer_CfgInitialize(&Timer, ConfigPtr, ConfigPtr->BaseAddr);


    setInputMatrices(aInst, bTInst);

    xil_printf("Matrix A:\r\n");
    displayMatrix(aInst);
    xil_printf("Matrix B^T:\r\n");
    displayMatrix(bTInst);

    char cmdBuffer[16];

    while (1) {
    	//REading command
        xil_printf("CMD:> ");
        int idx = 0;
        char ch;
        while ((ch = inbyte()) != '\r' && idx < sizeof(cmdBuffer)-1) {
            xil_printf("%c", ch);
            cmdBuffer[idx++] = ch;
        }
        cmdBuffer[idx] = '\0';
        xil_printf("\r\n");
        ///////////////////////////////////////////////////////////////////////

        if (strcmp(cmdBuffer, "mul") == 0) {
            xil_printf("Computing P = A x B^T\r\n");

            // ---------------- Soft Multiply Timing ----------------
            XScuTimer_LoadTimer(&Timer, 0xFFFFFFFF); // max count
            XScuTimer_Start(&Timer);

            multiMatrixSoft(aInst, bTInst, pInst);

            XScuTimer_Stop(&Timer);

            // Timer counts down, so subtract remaining from start to get elapsed ticks
            u32 soft_ticks = 0xFFFFFFFF - XScuTimer_GetCounterValue(&Timer);

            // Convert ticks to microseconds (Timer runs at 325 MHz)
            u32 soft_usec = (soft_ticks * 1000000ULL) / 325000000;

            xil_printf("\r\n");
            xil_printf("multiMatrixSoft took %u ticks (~%u us)\r\n", soft_ticks, soft_usec);
            xil_printf("\r\n");

            displayMatrix(pInst);

            // ---------------- Hard Multiply Timing ----------------
            XScuTimer_LoadTimer(&Timer, 0xFFFFFFFF);
            XScuTimer_Start(&Timer);

            multiMatrixHard(aInst, bTInst, pInst);

            XScuTimer_Stop(&Timer);

            u32 hard_ticks = 0xFFFFFFFF - XScuTimer_GetCounterValue(&Timer);
            u32 hard_usec = (hard_ticks * 1000000ULL) / 325000000;

            xil_printf("multiMatrixHard took %u ticks (~%u us)\r\n", hard_ticks, hard_usec);


        }
    }

    return 0;
}
