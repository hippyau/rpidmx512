/*
 * hub75bdisplay.cpp
 *
 */
/**
 * PoC
 */

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "hub75bdisplay.h"

#include "h3_spi.h"
#include "h3_gpio.h"
#include "board/h3_opi_zero.h"

/*
 * FPS
 */
#include "irq_timer.h"
#include "h3_timer.h"
#include "arm/arm.h"
#include "arm/synchronize.h"

#include "debug.h"

#define HUB75B_A		GPIO_EXT_13		// PA0
#define HUB75B_B		GPIO_EXT_11		// PA1
#define HUB75B_C		GPIO_EXT_22		// PA2
#define HUB75B_D		GPIO_EXT_15		// PA3

#define HUB75B_CK		GPIO_EXT_26		// PA10
#define HUB75B_LA		GPIO_EXT_7		// PA6
#define HUB75B_OE		GPIO_EXT_12		// PA7

#define HUB75B_R1		GPIO_EXT_24		// PA13
#define HUB75B_G1		GPIO_EXT_23		// PA14
#define HUB75B_B1		GPIO_EXT_19		// PA15

#define HUB75B_R2		GPIO_EXT_21		// PA16
#define HUB75B_G2		GPIO_EXT_18		// PA18
#define HUB75B_B2		GPIO_EXT_16		// PA19

/*
 * BAM (not PWM:)
 *
 *
 * The basic process is to
 * - blank the display,
 * - latch in the previously shifted data,
 * - update the row selects,
 * - unblank the display,
 * - shift in the next set of pixel data,
 * and then wait for an update timer to expire.
 * This is repeated four times for each row.
 * If you examine the blanking output,
 * you’ll notice that its low period doubles three times within the output period for each display row.
 * This is the result of using binary coded modulation to vary the intensity of each pixel.
 */

/**
 * Timer 1
 * FPS
 */

static volatile uint32_t updates_per_seconde;
static uint32_t updates_per_seconde_previous;
static uint32_t updates_counter;

static void irq_timer1(__attribute__((unused)) uint32_t clo) {
	dmb();

	updates_per_seconde = updates_counter - updates_per_seconde_previous;
	updates_per_seconde_previous = updates_counter;

	dmb();
}

Hub75bDisplay::Hub75bDisplay(uint32_t nColumns, uint32_t nRows): m_nColumns(nColumns), m_nRows(nRows) {
	h3_spi_end();

	h3_gpio_fsel(HUB75B_CK, GPIO_FSEL_OUTPUT);
	h3_gpio_fsel(HUB75B_LA, GPIO_FSEL_OUTPUT);
	h3_gpio_fsel(HUB75B_OE, GPIO_FSEL_OUTPUT);

	h3_gpio_clr(HUB75B_CK);
	h3_gpio_clr(HUB75B_LA);
	h3_gpio_set(HUB75B_OE);

	h3_gpio_fsel(HUB75B_A, GPIO_FSEL_OUTPUT);
	h3_gpio_fsel(HUB75B_B, GPIO_FSEL_OUTPUT);
	h3_gpio_fsel(HUB75B_C, GPIO_FSEL_OUTPUT);
	h3_gpio_fsel(HUB75B_D, GPIO_FSEL_OUTPUT);

	h3_gpio_clr(HUB75B_A);
	h3_gpio_clr(HUB75B_B);
	h3_gpio_clr(HUB75B_C);
	h3_gpio_clr(HUB75B_D);

	h3_gpio_fsel(HUB75B_R1, GPIO_FSEL_OUTPUT);
	h3_gpio_fsel(HUB75B_G1, GPIO_FSEL_OUTPUT);
	h3_gpio_fsel(HUB75B_B1, GPIO_FSEL_OUTPUT);

	h3_gpio_clr(HUB75B_R1);
	h3_gpio_clr(HUB75B_G1);
	h3_gpio_clr(HUB75B_B1);

	h3_gpio_fsel(HUB75B_R2, GPIO_FSEL_OUTPUT);
	h3_gpio_fsel(HUB75B_G2, GPIO_FSEL_OUTPUT);
	h3_gpio_fsel(HUB75B_B2, GPIO_FSEL_OUTPUT);

	h3_gpio_clr(HUB75B_R2);
	h3_gpio_clr(HUB75B_G2);
	h3_gpio_clr(HUB75B_B2);

	__disable_irq();
	irq_timer_init();
	irq_timer_set(IRQ_TIMER_1, irq_timer1);

	m_pFramebuffer = new uint32_t[m_nColumns * m_nRows]; //TODO This must be coherent memory
	assert(m_pFramebuffer != nullptr);

	for (uint32_t i = 0; i < (m_nColumns * m_nRows); i++) {
		m_pFramebuffer[i] = 0;
	}
}

void Hub75bDisplay::Start() {
	H3_TIMER->TMR1_INTV = 0xB71B00; // 1 second
	H3_TIMER->TMR1_CTRL &= ~(TIMER_CTRL_SINGLE_MODE);
	H3_TIMER->TMR1_CTRL |= (TIMER_CTRL_EN_START | TIMER_CTRL_RELOAD);

	__enable_irq();
	isb();
}

void Hub75bDisplay::SetPixel(uint32_t nColumn, uint32_t nRow, uint8_t nRed, uint8_t nGreen, uint8_t nBlue) {
	if (nRow < (m_nRows / 2)) { //TODO No need to calculate this /2 all the time
		const uint32_t nIndex = (nRow * m_nColumns) + nColumn;

		uint32_t nValue = m_pFramebuffer[nIndex];

		nValue &= ~((1U << HUB75B_R1) | (1U << HUB75B_G1) | (1U << HUB75B_B1));

		if ((nRed & 0x80) == 0x80) {
			nValue |= (1U << HUB75B_R1);
		}

		if ((nGreen & 0x80) == 0x80) {
			nValue |= (1U << HUB75B_G1);
		}

		if ((nBlue & 0x80) == 0x80) {
			nValue |= (1U << HUB75B_B1);
		}

		m_pFramebuffer[nIndex] = nValue;

		if (m_pFramebuffer[nIndex] & 0x01) {
			debug_dump(&m_pFramebuffer[nIndex], 4);
		}
	} else {
		const uint32_t nIndex = ((nRow - (m_nRows / 2)) * m_nColumns) + nColumn;

		uint32_t nValue = m_pFramebuffer[nIndex];
		nValue &= ~((1U << HUB75B_R2) | (1U << HUB75B_G2) | (1U << HUB75B_B2));

		if ((nRed & 0x80) == 0x80) {
			nValue |= (1U << HUB75B_R2);
		}

		if ((nGreen & 0x80) == 0x80) {
			nValue |= (1U << HUB75B_G2);
		}

		if ((nBlue & 0x80) == 0x80) {
			nValue |= (1U << HUB75B_B2);
		}

		m_pFramebuffer[nIndex] = nValue;

		if (m_pFramebuffer[nIndex] & 0x01) {
			printf("%d %d %d %x\n", nColumn, nRow, nIndex, nValue);
		}
	}
}

void Hub75bDisplay::Dump() {
	//debug_dump(m_pFramebuffer, (m_nColumns * m_nRows / 2));

	for (uint32_t row = 0; row < (m_nRows / 2); row++) {
		for (uint32_t i = 0; i < m_nColumns; i++) {
			const uint32_t nIndex = (row * m_nColumns) + i;
			printf("%x ", m_pFramebuffer[nIndex]);
		}
		puts("");
	}
}

uint32_t Hub75bDisplay::GetFps() {
	dmb();
	return updates_per_seconde;
}

/*
 * Update LA, OE and row select
 * Just output m_nColumns clock cycles
 */
void Hub75bDisplay::Run() {
	uint32_t nGPIO = H3_PIO_PORTA->DAT & ~((1U << HUB75B_R1) | (1U << HUB75B_G1) | (1U << HUB75B_B1) | (1U << HUB75B_R2) | (1U << HUB75B_G2) | (1U << HUB75B_B2));

	for (uint32_t row = 0; row < (m_nRows / 2); row++) {
		/* Shift in next data */
		for (uint32_t i = 0; i < m_nColumns; i++) {
			const uint32_t nIndex = (row * m_nColumns) + i;
			const uint32_t nValue =  m_pFramebuffer[nIndex];
			// Clock high with data
			H3_PIO_PORTA->DAT = nGPIO | (1U << HUB75B_CK) | nValue;
			// Clock low
			H3_PIO_PORTA->DAT = nGPIO | nValue;
		}

		/* Blank the display */
		H3_PIO_PORTA->DAT = nGPIO | (1U << HUB75B_OE);

		/* Latch the previous data */
		H3_PIO_PORTA->DAT = nGPIO | (1U << HUB75B_LA) | (1U << HUB75B_OE);
		nGPIO |= (1U << HUB75B_OE);
		H3_PIO_PORTA->DAT = nGPIO;

		/* Update the row select */
		//row_index = (row_index + 1) & 0xF;
		nGPIO &= ~(0xFU);
		nGPIO |= row;
		H3_PIO_PORTA->DAT = nGPIO;

		/* Enable the display */
		nGPIO &= ~(1U << HUB75B_OE);
		H3_PIO_PORTA->DAT = nGPIO;
	}

	updates_counter++;
}


void Hub75bDisplay::SetData(uint8_t nPort, const uint8_t *pData, uint16_t nLength) {
	// Quick and Dirty
	// 170 pixels per port

	uint32_t nIndex = 0;
	for (uint32_t i = 0; i < nLength; i = i + 3) {
		uint32_t nPixelIndex = (nPort * 170U) + nIndex++;
		const uint32_t nRow = nPixelIndex / m_nColumns;
		const uint32_t nColumn = nPixelIndex - (nRow * m_nColumns);
		SetPixel(nColumn, nRow, pData[i], pData[i+1], pData[i+2]);
	}
}