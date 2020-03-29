/**
 * @file main.cpp
 *
 */
/* Copyright (C) 2020 by Arjan van Vught mailto:info@raspberrypi-dmx.nl
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

#include "hardware.h"
#include "networkh3emac.h"
#include "ledblink.h"

#include "console.h"

#include "networkconst.h"

#include "spiflashinstall.h"

#include "spiflashstore.h"
#include "storeshowfile.h"
#include "storeremoteconfig.h"

#include "remoteconfig.h"
#include "remoteconfigparams.h"

#include "displayudf.h"
#include "displayudfparams.h"
#include "storedisplayudf.h"

#include "showfileparams.h"
#include "showfileosc.h"

// Format handlers
#include "olashowfile.h"

// Protocol handlers
#include "showfileprotocole131.h"
#include "showfileprotocolartnet.h"

#include "displayhandler.h"

#include "firmwareversion.h"
#include "software_version.h"

extern "C" {

void notmain(void) {
	Hardware hw;
	NetworkH3emac nw;
	LedBlink lb;
	DisplayUdf display;
	FirmwareVersion fw(SOFTWARE_VERSION, __DATE__, __TIME__);

	fw.Print();

	SpiFlashInstall spiFlashInstall;
	SpiFlashStore spiFlashStore;

	hw.SetLed(HARDWARE_LED_ON);

	console_status(CONSOLE_YELLOW, NetworkConst::MSG_NETWORK_INIT);
	display.TextStatus(NetworkConst::MSG_NETWORK_INIT, DISPLAY_7SEGMENT_MSG_INFO_NETWORK_INIT);

	nw.Init((NetworkParamsStore *)spiFlashStore.GetStoreNetwork());
	nw.SetNetworkStore((NetworkStore *)spiFlashStore.GetStoreNetwork());
	nw.Print();

	StoreShowFile storeShowFile;
	ShowFileParams showFileParams(&storeShowFile);

	if (showFileParams.Load()) {
		showFileParams.Dump();
	}

	ShowFile *pShowFile = 0;

	switch (showFileParams.GetFormat()) {
//		case SHOWFILE_FORMAT_:
//			break;
		default:
			pShowFile = new OlaShowFile;
			break;
	}

	assert(pShowFile != 0);

	DisplayHandler displayHandler;
	pShowFile->SetShowFileDisplay(&displayHandler);

	ShowFileProtocolHandler *pShowFileProtocolHandler = 0;

	switch (showFileParams.GetProtocol()) {
		case SHOWFILE_PROTOCOL_ARTNET:
			pShowFileProtocolHandler = new ShowFileProtocolArtNet;
			break;
		default:
			pShowFileProtocolHandler = new ShowFileProtocolE131;
			break;
	}

	assert(pShowFileProtocolHandler != 0);

	assert(pShowFile != 0);
	pShowFile->SetHandlers(pShowFileProtocolHandler);

	ShowFileOSC oscServer;

	showFileParams.Set();

	oscServer.Start();
	oscServer.Print();

	pShowFileProtocolHandler->Start();
	pShowFileProtocolHandler->Print();

	RemoteConfig remoteConfig(REMOTE_CONFIG_SHOWFILE, REMOTE_CONFIG_MODE_PLAYER, 0);

	StoreRemoteConfig storeRemoteConfig;
	RemoteConfigParams remoteConfigParams(&storeRemoteConfig);

	if (remoteConfigParams.Load()) {
		remoteConfigParams.Set(&remoteConfig);
		remoteConfigParams.Dump();
	}

	while (spiFlashStore.Flash())
		;

	display.SetTitle("Showfile player");
	display.Set(2, DISPLAY_UDF_LABEL_HOSTNAME);
	display.Set(3, DISPLAY_UDF_LABEL_IP);
	display.Set(4, DISPLAY_UDF_LABEL_VERSION);

	display.TextStatus(pShowFile->GetShowFileName(), pShowFile->GetShowFile());

	StoreDisplayUdf storeDisplayUdf;
	DisplayUdfParams displayUdfParams(&storeDisplayUdf);

	if(displayUdfParams.Load()) {
		displayUdfParams.Set(&display);
		displayUdfParams.Dump();
	}

	display.Show();

	pShowFile->SetShowFile(showFileParams.GetShow());
	pShowFile->Print();

	if (showFileParams.IsAutoStart()) {
		pShowFile->Start();
	}

	for (;;) {
		nw.Run();
		//
		pShowFile->Run();
		pShowFileProtocolHandler->Run();
		oscServer.Run();
		//
		remoteConfig.Run();
		spiFlashStore.Flash();
		lb.Run();
	}
}

}