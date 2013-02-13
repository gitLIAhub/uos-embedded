/*
 * IWDT driver for Milandr 1986BE microcontrollers.
 *
 * Copyright (C) 2013 Dmitry Podkhvatilin, <vatilin@gmail.com>
 *
 * This file is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You can redistribute this file and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software Foundation;
 * either version 2 of the License, or (at your discretion) any later version.
 * See the accompanying file "COPYING.txt" for more details.
 *
 * As a special exception to the GPL, permission is granted for additional
 * uses of the text contained in this file.  See the accompanying file
 * "COPY-UOS.txt" for details.
 */

#ifndef MILANDR_IWDG_H_
#define MILANDR_IWDG_H_

/*
 * Initialize IWDT.
 * timeout - watchdog timeout in milliseconds
 */
void init_iwdt(unsigned timeout);

/*
 * Acknowledge the watchdog so that it won't reset the processor.
 */
void ack_iwdt();

#endif /* MILANDR_IWDG_H_ */
