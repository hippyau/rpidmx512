/**
 * @file main.cpp
 *
 */
/* Copyright (C) 2019-2020 by Arjan van Vught mailto:info@orangepi-dmx.nl
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include "hardware.h"
#include "networkh3emac.h"
#include "ledblink.h"

#include "console.h"
#include "h3/showsystime.h"

#include "ntpclient.h"

#include "displayudf.h"
#include "displayudfparams.h"
#include "storedisplayudf.h"

#include "networkconst.h"

#include "e131bridge.h"
#include "e131params.h"
#include "e131msgconst.h"

// Monitor Output
#include "dmxmonitor.h"

#include "firmwareversion.h"
#include "software_version.h"

#include "displayudfnetworkhandler.h"
#include "displayhandler.h"

extern "C" {

void notmain(void) {
	Hardware hw;
	NetworkH3emac nw;
	LedBlink lb;
	DisplayUdf display;
	FirmwareVersion fw(SOFTWARE_VERSION, __DATE__, __TIME__);
	ShowSystime showSystime;

	fw.Print();

	console_puts("Ethernet sACN E1.31 ");
	console_set_fg_color(CONSOLE_GREEN);
	console_puts("Real-time DMX Monitor");
	console_set_fg_color(CONSOLE_WHITE);
	console_putc('\n');

	hw.SetLed(HARDWARE_LED_ON);
	lb.SetLedBlinkDisplay(new DisplayHandler);

	display.TextStatus(NetworkConst::MSG_NETWORK_INIT, DISPLAY_7SEGMENT_MSG_INFO_NETWORK_INIT, CONSOLE_YELLOW);

	nw.SetNetworkDisplay(new DisplayUdfNetworkHandler);
	nw.Init();
	nw.Print();

	NtpClient ntpClient;
	ntpClient.Init();
	ntpClient.Print();

	display.TextStatus(E131MsgConst::PARAMS, DISPLAY_7SEGMENT_MSG_INFO_BRIDGE_PARMAMS, CONSOLE_YELLOW);

	E131Bridge bridge;
	E131Params e131params;

	if (e131params.Load()) {
		e131params.Set(&bridge);
		e131params.Dump();
	}

	bridge.SetUniverse(0, E131_OUTPUT_PORT, e131params.GetUniverse());

	DMXMonitor monitor;
	// There is support for HEX output only
	bridge.SetDirectUpdate(false);
	bridge.SetOutput(&monitor);
	monitor.Cls();
	console_set_top_row(20);

	bridge.Print();

	display.SetTitle("Eth sACN E1.31 Monitor");
	display.Set(2, DISPLAY_UDF_LABEL_BOARDNAME);
	display.Set(3, DISPLAY_UDF_LABEL_IP);
	display.Set(4, DISPLAY_UDF_LABEL_VERSION);
	display.Set(5, DISPLAY_UDF_LABEL_UNIVERSE);
	display.Set(6, DISPLAY_UDF_LABEL_AP);

	DisplayUdfParams displayUdfParams;

	if(displayUdfParams.Load()) {
		displayUdfParams.Set(&display);
		displayUdfParams.Dump();
	}

	display.Show(&bridge);

	display.TextStatus(E131MsgConst::START, DISPLAY_7SEGMENT_MSG_INFO_BRIDGE_START, CONSOLE_YELLOW);

	bridge.Start();

	display.TextStatus(E131MsgConst::STARTED, DISPLAY_7SEGMENT_MSG_INFO_BRIDGE_STARTED, CONSOLE_GREEN);

	hw.WatchdogInit();

	for (;;) {
		hw.WatchdogFeed();
		nw.Run();
		bridge.Run();
		ntpClient.Run();
		lb.Run();
		showSystime.Run();
		display.Run();
	}
}

}
