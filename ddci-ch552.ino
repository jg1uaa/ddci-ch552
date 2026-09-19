// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 SASANO Takayoshi <uaa@uaa.org.uk>

#ifndef USER_USB_RAM
#error "USB Settings: USER CODE w/ 148B USB ram required"
#endif

#if (F_CPU % 12000)
#error "CPU clock must be (12000 * n) Hz"
#endif

#include "src/userUsbHidKeyboard/USBHIDKeyboard.h"

#define PIN0_ON 0x01
#define PIN1_ON 0x02
#define PIN_MASK (PIN0_ON | PIN1_ON)

#define LED_pin 30
#define LED_port P3_0

#define PIN0_pin 33
#define PIN0_port P3_3

#define PIN1_pin 34
#define PIN1_port P3_4

volatile unsigned char TimerExpired = 0;
volatile unsigned char CurrPinStatus = 0;
volatile unsigned char PrevPinStatus = 0;
#define PinMaskCount 4 // 4ms debounce
volatile unsigned char PinMaskCounter0 = 0;
volatile unsigned char PinMaskCounter1 = 0;

/* due to limitation of library, no Ctrl key support */
#define PIN0_KEY '['
#define PIN1_KEY ']'

static void update_pin_status(void)
{
	CurrPinStatus = 0;

	if (!PIN0_port) CurrPinStatus |= PIN0_ON;
	if (!PIN1_port) CurrPinStatus |= PIN1_ON;

	LED_port = CurrPinStatus ? 1 : 0;
}

void Timer2Interrupt(void) __interrupt(INT_NO_TMR2)
{
	TF2 = 0; // clear timer flag

	update_pin_status();

	if (PinMaskCounter0) PinMaskCounter0--;
	if (PinMaskCounter1) PinMaskCounter1--;

	TimerExpired = 1;
}

void timer_init(void)
{
	unsigned short limit;

	/* set timer mode (Fsys/12) */
	T2CON = 0x00;
	T2MOD &= ~(bT2_CLK | bT2_CAP_M1 | bT2_CAP_M0 | T2OE | bT2_CAP1_EN);

	/* set interrupt cycle (1ms) */
	limit = 65536U - (F_CPU / 12000U);

	RCAP2H = TH2 = limit >> 8;
	RCAP2L = TL2 = limit;

	/* enable interrupt */
	ET2 = 1; // enable Timer2 interrupt
	TR2 = 1; // start Timer2
}

void setup(void)
{
	pinMode(PIN0_pin, INPUT_PULLUP);
	pinMode(PIN1_pin, INPUT_PULLUP);
	pinMode(LED_pin, OUTPUT);

	timer_init();
	USBInit();
}

void loop(void)
{
	unsigned char changed, useprev;
	signed char pin0, pin1;

	while (!TimerExpired);

	EA = 0; // disable interrupt

	changed = (CurrPinStatus ^ PrevPinStatus) & PIN_MASK;
	useprev = ((PinMaskCounter0 ? PIN0_ON : 0) |
		   (PinMaskCounter1 ? PIN1_ON : 0));
	pin0 = pin1 = 0;

	if (changed ^ useprev) {
		PrevPinStatus =
			(PrevPinStatus & useprev) | (CurrPinStatus & ~useprev);
		if (changed & ~useprev & PIN0_ON) {
			PinMaskCounter0 = PinMaskCount;
			pin0 = (CurrPinStatus & PIN0_ON) ? 1 : -1;
		}
		if (changed & ~useprev & PIN1_ON) {
			PinMaskCounter1 = PinMaskCount;
			pin1 = (CurrPinStatus & PIN1_ON) ? 1 : -1;
		}
	}
	TimerExpired = 0;

	EA = 1; // enable interupt

	if (pin0 < 0) Keyboard_release(PIN0_KEY);
	else if (pin0 > 0) Keyboard_press(PIN0_KEY);
		
	if (pin1 < 0) Keyboard_release(PIN1_KEY);
	else if (pin1 > 0) Keyboard_press(PIN1_KEY);
}
