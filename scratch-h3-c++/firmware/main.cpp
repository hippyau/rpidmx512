#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <assert.h>

#include "hardware.h"
#include "networkh3emac.h"
#include "ledblink.h"

#include "display.h"

#include "spiflashinstall.h"
#include "spiflashstore.h"
#include "remoteconfig.h"
#include "remoteconfigparams.h"

#include "storenetwork.h"
#include "storeremoteconfig.h"

#include "networkhandleroled.h"

#include "firmwareversion.h"

static const char SOFTWARE_VERSION[] = "0.0";

/*
 *
 */
#include <h3/shell.h>
/*
 * Debugging System Clock framework for PTP implementation
 */
#include <arpa/inet.h>
#include <netinet/in.h>

#include "ntpclient.h"
#include "ptpclient.h"
/*
 *
 */
#include "hwclock.h"

extern "C" {

void notmain(void) {
	Hardware hw;
	HwClock hwClock;
	NetworkH3emac nw;
	LedBlink lb;
	Display display(0,4);
	FirmwareVersion fw(SOFTWARE_VERSION, __DATE__, __TIME__);

	SpiFlashInstall spiFlashInstall;
	SpiFlashStore spiFlashStore;

	fw.Print();

	NetworkHandlerOled networkHandlerOled;

	nw.SetNetworkDisplay(&networkHandlerOled);
	nw.SetNetworkStore(StoreNetwork::Get());
	nw.Init(StoreNetwork::Get());
	nw.Print();

	networkHandlerOled.ShowIp();

	/*
	 * Debugging System Clock framework for PTP implementation
	 */

	NtpClient ntpClient(nw.GetNtpServerIp());
	ntpClient.SetNtpClientDisplay(&networkHandlerOled);
	ntpClient.Start();
	ntpClient.Print();

	if (ntpClient.GetStatus() != NtpClientStatus::STOPPED) {
		printf("Set RTC from System Clock\n");
		hwClock.SysToHc();
	} else {
		printf("Set System Clock from RTC\n");
		hwClock.HcToSys();
	}

	RemoteConfig remoteConfig(REMOTE_CONFIG_RDMNET_LLRP_ONLY, REMOTE_CONFIG_MODE_CONFIG, 0);

	StoreRemoteConfig storeRemoteConfig;
	RemoteConfigParams remoteConfigParams(&storeRemoteConfig);

	if (remoteConfigParams.Load()) {
		remoteConfigParams.Set(&remoteConfig);
		remoteConfigParams.Dump();
	}

	while (spiFlashStore.Flash())
		;

	/*
	 *
	 */
	Shell shell;

	int nPrevSeconds = 60; // Force initial update

	display.ClearLine(0);
	display.ClearLine(1);

	for (;;) {
		nw.Run();
		remoteConfig.Run();
		spiFlashStore.Flash();
		lb.Run();

		/*
		 *
		 */
		shell.Run();

		/*
		 * Debugging System Clock framework for PTP implementation
		 */
		// ntpClient.Run();

		/*
		 *
		 */

		time_t ltime;
		struct tm *tm;
		ltime = time(nullptr);
		tm = localtime(&ltime);

		if (tm->tm_sec != nPrevSeconds) {
			nPrevSeconds = tm->tm_sec;
			struct rtc_time rtc;
			hwClock.Get(&rtc);

			display.Printf(1, "%.2d:%.2d:%.2d", tm->tm_hour, tm->tm_min, tm->tm_sec);
			display.Printf(2, "%.2d:%.2d:%.2d", rtc.tm_hour, rtc.tm_min, rtc.tm_sec);
		}
	}
}

}
