#include <runtime/lib.h>
#include <kernel/uos.h>
#include <uart/uart.h>

#if defined(__AVR__)
#   include "avr.h"
#endif

#if defined(ARM_S3C4530)
#   include "samsung.h"
#endif

#if defined(ARM_AT91SAM)
#   include "at91sam.h"
#endif

#if defined(PIC32MX)
#   include "pic32.h"
#endif

#if defined (ELVEES)
#   include "elvees.h"
#endif

#if defined(MSP430)
#   include "msp430.h"
#endif

#if defined(LINUX386)
#   include "linux.h"
#endif

#ifndef uart_io_base
#define uart_io_base(port) (port)
#else
#endif


/*
 * Start transmitting a byte.
 * Assume the transmitter is stopped, and the transmit queue is not empty.
 * Return 1 when we are expecting the hardware interrupt.
 */
static bool_t
uart_transmit_start (uart_t *u)
{
	if (u->out_first == u->out_last)
		mutex_signal (&u->transmitter, 0);

	/* Check that transmitter buffer is busy. */
	if (! test_transmitter_empty (u->port)) {
		return 1;
	}

	/* Nothing to transmit or no CTS - stop transmitting. */
	if (u->out_first == u->out_last ||
	    (u->cts_query && u->cts_query (u) == 0)) {
		/* Disable `transmitter empty' interrupt. */
		disable_transmit_interrupt (u->port);
		return 0;
	}

	/* Send byte. */
#if	UART_PORT_FIFO_SIZE > 1
	unsigned least = UART_PORT_FIFO_SIZE;
    for (;(u->out_first != u->out_last) && (least > 0); --least ) {
        transmit_byte (u->port, *u->out_first);
        ++u->out_first;
        if (u->out_first >= u->out_buf + UART_OUTBUFSZ)
            u->out_first = u->out_buf;
    }
#else
	transmit_byte (u->port, *u->out_first);

	++u->out_first;
    if (u->out_first >= u->out_buf + UART_OUTBUFSZ)
        u->out_first = u->out_buf;
#endif

	/* Enable `transmitter empty' interrupt. */
	enable_transmit_interrupt (u->port);
	return 1;
}

/*
 * Wait for transmitter to finish.
 */
static void
uart_fflush (uart_t *u)
{
    if (u->out_first == u->out_last)
        return;

    mutex_lock (&u->transmitter);

	/* Check that transmitter is enabled. */
	if (u->out_first != u->out_last)
	if (test_transmitter_enabled (u->port))
		while (u->out_first != u->out_last)
			mutex_wait (&u->transmitter);

	mutex_unlock (&u->transmitter);
}

/*
 * CTS is active - wake up the transmitter.
 */
void
uart_cts_ready (uart_t *u)
{
	mutex_lock (&u->transmitter);
	uart_transmit_start (u);
	mutex_unlock (&u->transmitter);
}

/*
 * Register the CTS poller function.
 */
void
uart_set_cts_poller (uart_t *u, bool_t (*func) (uart_t*))
{
	mutex_lock (&u->transmitter);
	u->cts_query = func;
	mutex_unlock (&u->transmitter);
}

/*
 * Send a byte to the UART transmitter.
 */
void
uart_putchar (uart_t *u, short c)
{
	unsigned char *newlast;

	mutex_lock (&u->transmitter);

	/* Check that transmitter is enabled. */
	if (test_transmitter_enabled (u->port)) {
again:		newlast = u->out_last + 1;
		if (newlast >= u->out_buf + UART_OUTBUFSZ)
			newlast = u->out_buf;
		while (u->out_first == newlast)
			mutex_wait (&u->transmitter);

		/* TODO: unicode to utf8 conversion. */
		*u->out_last = c;
		u->out_last = newlast;
		uart_transmit_start (u);

		if (u->onlcr && c == '\n') {
			c = '\r';
			goto again;
		}
	}
	mutex_unlock (&u->transmitter);
}

/*
 * Wait for the byte to be received and return it.
 */
unsigned short
uart_getchar (uart_t *u)
{
	unsigned char c;

	mutex_lock (&u->receiver);

	/* Wait until receive data available. */
	while (u->in_first == u->in_last)
		mutex_wait (&u->receiver);
	/* TODO: utf8 to unicode conversion. */
	c = *u->in_first++;
	if (u->in_first >= u->in_buf + UART_INBUFSZ)
		u->in_first = u->in_buf;

	mutex_unlock (&u->receiver);
	return c;
}

int
uart_peekchar (uart_t *u)
{
	int c;

	mutex_lock (&u->receiver);
	/* TODO: utf8 to unicode conversion. */
	c = (u->in_first == u->in_last) ? -1 : *u->in_first;
	mutex_unlock (&u->receiver);
	return c;
}

/*
 * Receive interrupt task.
 */
static void
uart_receiver (void *arg)
{
	uart_t *u = (uart_t *)arg;
	unsigned char* newlast;

	/*
	 * Enable transmitter.
	 */
#ifdef TRANSMIT_IRQ
	mutex_lock_irq (&u->transmitter, TRANSMIT_IRQ (u->port),
		(handler_t) uart_transmit_start, u);
	enable_transmitter (u->port);
	mutex_unlock (&u->transmitter);
#endif
	/*
	 * Enable receiver.
	 */
	mutex_lock_irq (&u->receiver, RECEIVE_IRQ (u->port), 0, 0);
	enable_receiver (u->port);
	enable_receive_interrupt (u->port);

	for (;;) {
		mutex_wait (&u->receiver);

#ifdef clear_receive_errors
		clear_receive_errors (u->port);
#else
		if (test_frame_error (u->port)) {
			/*debug_printf ("FRAME ERROR\n");*/
			clear_frame_error (u->port);
		}
		if (test_parity_error (u->port)) {
			/*debug_printf ("PARITY ERROR\n");*/
			clear_parity_error (u->port);
		}
		if (test_overrun_error (u->port)) {
			/*debug_printf ("RECEIVE OVERRUN\n");*/
			clear_overrun_error (u->port);
		}
		if (test_break_error (u->port)) {
			/*debug_printf ("BREAK DETECTED\n");*/
			clear_break_error (u->port);
		}
#endif
#ifndef TRANSMIT_IRQ
		if (test_transmitter_enabled (u->port)){
		        mutex_lock(&u->transmitter);
                uart_transmit_start (u);
                mutex_unlock (&u->transmitter);
		}
#endif
		/* Check that receive data is available, and get the received bytes. */
#ifdef test_receiver_avail
        while ( test_receiver_avail(u->port) )
#else
        unsigned char c = 0;
        while (test_get_receive_data (u->port, &c))
#endif
		{
/*debug_printf ("%02x", c);*/

		newlast = u->in_last + 1;
		if (newlast >= u->in_buf + UART_INBUFSZ)
			newlast = u->in_buf;

#ifdef test_receiver_avail
        /* Ignore input on buffer overflow. */
        if (u->in_first == newlast)
            break;
		*u->in_last = get_received_byte(u->port);
#else
        /* Ignore input on buffer overflow. */
        if (u->in_first == newlast)
            continue;
        *u->in_last = c;
#endif
		u->in_last = newlast;
		} //while (test_get_receive_data (u->port, &c))
	}
}

mutex_t *
uart_receive_lock (uart_t *u)
{
	return &u->receiver;
}

#ifdef __cplusplus
#define idx(i)
#define item(i)
#else
#define idx(i) [i] = 
#define item(i) .i =
#endif

static stream_interface_t uart_interface = {
	item(putc) (void (*) (stream_t*, short))		uart_putchar,
	item(getc) (unsigned short (*) (stream_t*))	uart_getchar,
	item(peekc) (int (*) (stream_t*))			uart_peekchar,
	item(flush) (void (*) (stream_t*))			uart_fflush,
	item(eof)   (bool_t (*) (stream_t*))        0,
	item(close) (void (*) (stream_t*))          0,
	item(receiver) (mutex_t *(*) (stream_t*))		uart_receive_lock,
#if STREAM_HAVE_ACCEESS > 0
    //* позволяют потребовать монопольного захвата потока
    item(access_rx)                             0
    , item(access_tx)                           0
#endif
};

void
uart_init (uart_t *u, small_uint_t port, int prio, unsigned int khz,
	unsigned long baud)
{
	u->interface = &uart_interface;
	u->in_first = u->in_last = u->in_buf;
	u->out_first = u->out_last = u->out_buf;
	u->khz = khz;
	u->onlcr = 1;
#if (defined(ARM_1986BE9) || defined(ARM_1986BE1))
	u->port = (port == 0) ? ARM_UART1_BASE : ARM_UART2_BASE;
#else
	u->port = uart_io_base(port);
#endif

	mutex_init(&u->transmitter);
    mutex_init(&u->receiver);

	/* Setup baud rate generator. */
	setup_baud_rate (u->port, u->khz, baud);

	/* Create uart receive task. */
	task_create (uart_receiver, u, "uartr", prio,
		u->rstack, sizeof (u->rstack));
}
