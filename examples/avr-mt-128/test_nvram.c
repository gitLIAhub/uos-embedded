/*
 * Testing NVRAM.
 */
#include <runtime/lib.h>
#include <kernel/uos.h>
#include <nvram/nvram.h>
#include <lcd2/lcd.h>
#include "avr-mt-128.h"

lcd_t line1, line2;
ARRAY (task, 280);

void wait_for_button ()
{
	puts (&line2, "\fPress button B4.");

	/* Wait until right button released. */
	while (button_right_pressed ())
		continue;

	/* Wait until right button pressed. */
	while (! button_right_pressed ())
		continue;
}

void test (void *data)
{
	unsigned i, errors;
	unsigned char c;

	puts (&line1, "\fTesting NVRAM.");
	for (;;) {
		wait_for_button ();
		errors = 0;

		for (i=0; i<=E2END; ++i) {
			if ((i & 63) == 0)
				printf (&line2, "\fWriting %d...", i);
			nvram_write_byte (i, ~i);
		}
		for (i=0; i<=E2END; ++i) {
			if ((i & 1023) == 0)
				printf (&line2, "\fReading %d...", i);
			c = nvram_read_byte (i);
			if (c != (unsigned char) ~i) {
				++errors;
				printf (&line1, "\f%d: w %02X r %02X\n",
					i, (unsigned char) ~i, c);
				wait_for_button ();
				printf (&line2, "\fReading %d", i);
			}
		}
		if (errors)
			printf (&line1, "\fTotal %d errs.", errors);
		else
			printf (&line1, "\f%d bytes OK.", E2END+1);
	}
}

void uos_init (void)
{
	lcd_init (&line1, &line2, 0);
	nvram_init ();
	task_create (test, 0, "lcd", 1, task, sizeof (task));
}
