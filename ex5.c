#include "xil_printf.h"
#include "xtime_l.h"
#include <string.h>
#include "xil_io.h"
#include "xparameters.h"
#define MATRIX_IP_S00_AXI_SLV_REG0_OFFSET 0
#define MATRIX_IP_S00_AXI_SLV_REG1_OFFSET 4
#define MATRIX_IP_S00_AXI_SLV_REG2_OFFSET 8

#define MSIZE 4

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


// 5.3
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
    setInputMatrices(aInst, bTInst);

    xil_printf("Matrix A:\r\n");
    displayMatrix(aInst);

    xil_printf("Matrix B^T:\r\n");
    displayMatrix(bTInst);

    char cmdBuffer[16];

    while (1) {
        xil_printf("CMD:> ");

        int idx = 0;
        char ch;

        while ((ch = inbyte()) != '\r' && idx < sizeof(cmdBuffer) - 1) {
        	xil_printf("%c", ch);
            cmdBuffer[idx++] = ch;
        }
        cmdBuffer[idx] = '\0'; // add 0 to thee end so it terminates the string
        xil_printf("\r\n");

        if (strcmp(cmdBuffer, "mul") == 0) {
            xil_printf("Computing P = A x B^T\r\n");

            XTime t0, t1;
            XTime_GetTime(&t0);
            multiMatrixSoft(aInst, bTInst, pInst);
            XTime_GetTime(&t1);

            unsigned long long ticks = (unsigned long long)(t1 - t0);
            xil_printf("multiMatrixSoft took %llu ticks\r\n", ticks);

            displayMatrix(pInst);

            XTime_GetTime(&t0);
            multiMatrixHard(aInst, bTInst, pInst);
            XTime_GetTime(&t1);
            xil_printf("Hard: %llu ticks\r\n", (unsigned long long)(t1 - t0));
        }
    }

    return 0;
}

