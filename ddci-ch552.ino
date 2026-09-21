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

#define CFG_pin 35
#define CFG_port P3_5

volatile unsigned char TimerExpired = 0;
volatile unsigned char CurrPinStatus = 0;
volatile unsigned char PrevPinStatus = 0;
#define PinMaskCount 4 // 4ms debounce
volatile unsigned char PinMaskCounter0 = 0;
volatile unsigned char PinMaskCounter1 = 0;

/* src/userUsbHidKeyboard/USBHIDKeyboard.c */
#define HIDKey_size 1
extern __xdata uint8_t HIDKey[HIDKey_size];
extern uint8_t USB_EP1_send(void);

/* src/userUsbHidKeyboard/USBhandler.c */
extern uint8_t USB_RemoteWakeup();

/* src/userUsbHidKeyboard/USBconstant.c */
extern __code uint8_t ReportDescriptor_bracket[];
extern __code uint8_t ReportDescriptor_ctrl[];
extern __code uint8_t *ReportDescriptor;

static void send_usb(unsigned char status)
{
	// XXX always send key event even if it was used for wakeup
	USB_RemoteWakeup();

	HIDKey[0] = status;
	USB_EP1_send();
}

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

static void display_status(char *str)
{
#define DOT_TIME 100

	for (; *str; str++) {
		LED_port = 1;
		delay(DOT_TIME * ((*str == '-') ? 3 : 1));
		LED_port = 0;
		delay(DOT_TIME);
	}

	delay(DOT_TIME * 2);
}

static void mode_config(void)
{
	bool mode;

	mode = (!PIN0_port ^ !CFG_port);

	// same code length
	if (mode) {
		display_status("--");
		ReportDescriptor = ReportDescriptor_ctrl;
	} else {
		display_status("....");
		ReportDescriptor = ReportDescriptor_bracket;
	}

	while(!PIN0_port); // wait for key release
}

void setup(void)
{
	pinMode(PIN0_pin, INPUT_PULLUP);
	pinMode(PIN1_pin, INPUT_PULLUP);
	pinMode(CFG_pin, INPUT_PULLUP);
	pinMode(LED_pin, OUTPUT);
	LED_port = 0; // LED is on at boot
	delay(50);

	mode_config();
	timer_init();
	USBInit();
}

void loop(void)
{
	unsigned char changed, useprev, status;
	bool send;

	while (!TimerExpired);

	EA = 0; // disable interrupt

	changed = CurrPinStatus ^ PrevPinStatus;
	useprev = ((PinMaskCounter0 ? PIN0_ON : 0) |
		   (PinMaskCounter1 ? PIN1_ON : 0));
	send = false;

	PrevPinStatus = status =
		(PrevPinStatus & useprev) | (CurrPinStatus & ~useprev);
	if (changed & ~useprev & PIN0_ON) {
		PinMaskCounter0 = PinMaskCount;
		send = true;
	}
	if (changed & ~useprev & PIN1_ON) {
		PinMaskCounter1 = PinMaskCount;
		send = true;
	}
	TimerExpired = 0;

	EA = 1; // enable interupt

	if (send) send_usb(status);
}
