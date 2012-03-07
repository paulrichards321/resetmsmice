/*
 * Fixes scroll wheel issues with certain Wireless Microsoft mice in X.org (includes KDE & Gnome applications), where the vertical wheel scrolls abnormally fast. Only needed if you dual boot between Microsoft Windows and some linux distro.
 * Known to fix the vertical scroll wheel issue with the following models (and likely others related):
 *    Microsoft Wireless Mobile Mouse 3500
 *    Microsoft Wireless Mouse 5000
 * This program basically just resets a setting in the mouse through usb communications and then exits. Only if the mouse matches vendor [0x045e] (Microsoft) and product code [0x0745] (a series of Microsoft Wireless mice) it will check for this usb setting.
 *
 * Copyright (C) 2011  Paul F. Richards (paulrichards321@gmail.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

static const char* syntax = 
	"Syntax: resetmsmice [OPTIONS]\n"
	"Fixes scroll wheel issues with certain Wireless Microsoft mice in X.org (includes KDE & Gnome applications), where the vertical wheel scrolls abnormally fast. Only needed if you dual boot between Microsoft Windows and some linux distro.\n"
	"Known to fix the vertical scroll wheel issue with the following models (and likely others related):\n"
	"*    Microsoft Wireless Mobile Mouse 3500\n"
	"*    Microsoft Wireless Mouse 5000\n"
	"This program basically just resets a setting in the mouse through usb communications and then exits. Only if the mouse matches vendor [0x045e] (Microsoft) and product code [0x0745] (a series of Microsoft Wireless mice) it will check for this setting.\n"
	"If it is run with no options, check all known Microsoft mice plugged into the system that can cause this issue.\n"
	"All messages are printed on screen unless run as a daemon. Will always try to log to syslog.\n"
	"This program normally needs to run with root privileges in order to talk to the mouse.\n\n"
	"The following are optional arguments, which are used to run smoother when called through startup scripts or udev:\n"
	"  -b, --busnum=NUMBER   only check the device on this usb bus, and this\n"
	"  -d, --devnum=NUMBER   usb device number (useful with udev)\n"
	"  -u, --daemon          detach from the console and run in a subprocess, to return control to caller immediately (useful with startup scripts)\n"
	"  -h, --help            this help screen.\n"
	"\nReport bugs to the writer of this program: Paul F. Richards (paulrichards321@gmail.com)";

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <iconv.h>
#include <locale.h>
#include <sys/types.h>
#include <syslog.h>
#include <libusb-1.0/libusb.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <ctype.h>
#include "conf.h"

#ifdef HAVE_STDBOOL_H
# include <stdbool.h>
#else
# ifndef HAVE__BOOL
#  ifdef __cplusplus
typedef bool _Bool;
#  else
#   define _Bool signed char
#  endif
# endif
# define bool _Bool
# define false 0
# define true 1
# define __bool_true_false_are_defined 1
#endif

#define KNOWN_MICE_IDS 1

int ms_probe_wireless_mouse(libusb_device*, struct libusb_device_descriptor*);

int print_usb_error(int error_value);

struct MOUSE_TYPE {
	uint16_t vendor_id;
	uint16_t model_id;
	const char* vendor_name;
	const char* model_notes;
	int (*probe)(libusb_device*, struct libusb_device_descriptor*);
} 
mousetype[1] = { 
	0x045e, 0x0745, "Microsoft", "Microsoft Wireless Mouse Series", ms_probe_wireless_mouse
};

#define TIME_OUT_VAL 500 // in milliseconds

const int interface = 0;

void logprintf(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);

	vprintf(format, ap);
	printf("\n");
	vsyslog(LOG_DEBUG | LOG_USER, format, ap);

	va_end(ap);
}


int ms_probe_wireless_mouse(libusb_device *dev, struct libusb_device_descriptor *desc_ptr)
{
	int i, r;
	bool claimed = false;
	bool kerneldriver = false;
	struct libusb_device_handle *devh = NULL;
	unsigned char language_in[256];
	unsigned char model_name[256];
	unsigned char datain[2] = { 0x17, 0x00 };
	unsigned char dataout[2] = { 0x17, 0x00 };
	bool doreset = false;
	uint16_t langid = 0;

    /*-----------------*/
	/* Open the device */
	/*-----------------*/
	r = libusb_open(dev, &devh);
	if (r != 0) {
		logprintf("Failed to open usb device.");
		print_usb_error(r);
		goto cleanup;
	}

	/*---------------------------------------------------------------------
		TODO: Does the configuration have to be set on the device? 
		Cannot get this to work, but does not appear to be needed:
		
		r = libusb_set_configuration(devh, interface);
		if (r < 0) {
			print_usb_error(r);

		}
	---------------------------------------------------------------------*/
		
	/*-------------------------------------------------------------*/
	/* Check for active kernel driver, if so, detach kernel driver */
	/*-------------------------------------------------------------*/
	if (libusb_kernel_driver_active(devh, interface) == 1) {
		r = libusb_detach_kernel_driver(devh, interface);
		if (r == 0) {
			kerneldriver = true;
		} else {
			kerneldriver = false;
			logprintf("Cannot open mouse, failed to detach kernel driver.");
			print_usb_error(r);
			goto cleanup;
		}
	}
		
	/*------------------------------------------------------------------*/
	/* Claim interface, a necessary library routine before sending data */
	/*------------------------------------------------------------------*/
	r = libusb_claim_interface(devh, interface);
	if (r == 0) {
		claimed = true;
	} else {
		claimed = false;
		logprintf("Cannot open mouse and claim interface.");
		print_usb_error(r);
		goto cleanup;
	}

    /*-------------------------*/
	/* Request mouse language  */
	/*-------------------------*/
	memset(language_in, 0, sizeof(language_in));
	r = libusb_get_string_descriptor(devh, 0x00, 0x0000, language_in, 255);
	if (r > 2) {
		langid = (language_in[3] << 8) | language_in[2];
	} else {
		logprintf("Could not retreive mouse language code.");
		print_usb_error(r);
	}

	/*---------------------------*/
	/* Request mouse model name  */
	/*---------------------------*/
	memset(model_name, 0, sizeof(model_name));
	r = libusb_get_string_descriptor(devh, desc_ptr->iProduct, langid, model_name, 255);	
	if (r > 2 && r <= 255) {
		char model_name_utf[256];
		char * model_name_ptr = (char*) &model_name[2];
		char * model_name_utf_ptr = model_name_utf;
		size_t inbytes_left = r - 2;
 		size_t outbytes_left = sizeof(model_name_utf);	
		
		memset(model_name_utf, 0, sizeof(model_name_utf));
		
		iconv_t iconv_ptr = iconv_open("UTF-8", "UCS2");
		if (iconv_ptr) {
			iconv(iconv_ptr, &model_name_ptr, &inbytes_left, &model_name_utf_ptr, &outbytes_left);
			logprintf("Model Name: %s", model_name_utf);
			iconv_close(iconv_ptr);
		}
	} else {
		logprintf("Failed to get model name from mouse.");
		print_usb_error(r);
	}
		
	/*-------------------------*/
	/* Request status of mouse */
	/*-------------------------*/
	r = libusb_control_transfer(devh, LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_ENDPOINT_IN, 0x01, 0x317, 1, datain, 2, TIME_OUT_VAL);
	if (r > 0) {
		if (datain[0] == 0x17 && datain[1] == 0x00) {
			logprintf("Microsoft mouse already in X.org Windows compatibility mode.\n");
			r = 0; /* set return to zero, meaning no errors */
		} else if (datain[0] == 0x17) {
			doreset = true;
			logprintf("Mouse in non-compatible mode with X.org Windows. Trying to set compatibility mode... ");
		} else {
			logprintf("Unknown mode detected on mouse. Hexadecimal values returned: %X %X", (int) datain[0], (int) datain[1]);
			logprintf("Leaving mouse as is. Report this message and values to programmer.");
		}
	} else {
		logprintf("Could not determine what mode mouse is in. Leaving mouse alone.");
		print_usb_error(r);
		goto cleanup;
	}
		
	if (doreset) {
		/*---------------------------*/
		/* Send reset codes to mouse */
		/*---------------------------*/
		r = libusb_control_transfer(devh, LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_ENDPOINT_OUT, 0x09, 0x317, 1, dataout, 2, TIME_OUT_VAL);
		if (r == 2) {
			logprintf("Successfully set mouse in X Windows compatibility mode.\n");
			r = 0; /* set return to zero, meaning sucess */
		} else {
			logprintf("Unexpected return code from usb reset.");
			print_usb_error(r);
			goto cleanup;
		}
	}
	
	/*------------------*/
	/* libusb cleanup   */
	/*------------------*/
	cleanup:
	if (claimed) libusb_release_interface(devh, interface);
	if (kerneldriver) libusb_attach_kernel_driver(devh, interface);
	if (devh) libusb_close(devh);
	return r;
}


int print_usb_error(int error_value)
{
	switch (error_value) {
	case LIBUSB_ERROR_TIMEOUT:
		logprintf("Timeout in communication with mouse.");
		break;
	case LIBUSB_ERROR_PIPE:
		logprintf("Pipe error in communication with mouse.");
		break;
	case LIBUSB_ERROR_NO_DEVICE:
		logprintf("Received \"no device\" error in communication with mouse.");
		break;
	case LIBUSB_ERROR_NO_MEM:
		logprintf("Cannot open Microsoft device: out of memory error");
		break;
	case LIBUSB_ERROR_ACCESS:
		logprintf("Cannot open Microsoft device: insufficient permissions");
		break;
	}
	logprintf("libusb returned: %i", error_value);
	return error_value;
}


int main(int argc, char** argv)
{
	int busnum = 0, devnum = 0;
	bool busnum_set = false, devnum_set = false;
	bool daemonize = false;
	int choice;
	libusb_device *dev, **devs;
	struct libusb_device_descriptor desc;
	int r;
	ssize_t cnt;
	int problem_devices = 0;
	pid_t pID;
	int option_index = 0;
	static struct option long_options[] =
	{
		{"busnum", required_argument, 0, 'b'},
		{"devnum", required_argument, 0, 'd'},
		{"daemon", no_argument, 0, 'u'},
		{"help", no_argument, 0, 'h'},
		{0, 0, 0, 0}
	};
	const char* short_options = "b:d:u";
	
	setlocale(LC_ALL, ""); /* We need this for the unicode mouse model name, pulled from mouse */

	/*--------------------------------*/
	/* Parse the command line options */
	/*--------------------------------*/
	choice = getopt_long(argc, argv, short_options, long_options, &option_index);
	while (choice != -1) {
		switch (choice) {
		case 'b':
			if (isdigit(optarg[0])) {
				busnum = atoi(optarg);
				busnum_set = true;
			} else {
				puts(syntax);
				exit(1);
			}
			break;
		case 'd':
			if (isdigit(optarg[0])) {
				devnum = atoi(optarg);
				devnum_set = true;
			} else {
				puts(syntax);
				exit(1);
			}
			break;
		case 'u':
			daemonize = true;
			break;
		case 'h':
			puts(syntax);
			exit(0);
		default:
			puts(syntax);
			exit(1);
		}
		choice = getopt_long(argc, argv, short_options, long_options, &option_index);
	}
	
	/* Make sure user supplied both the bus number and device number and not just one or the other... */
	if (devnum_set || busnum_set) {
		if (devnum_set == false || busnum_set == false) {
			puts("Please supply BOTH bus and device number.");
			puts(syntax);
			exit(1);
		}
	}

	if (daemonize) {
		pID = fork(); /* This is needed when we need to return from udev as soon as possible */

		if (pID == 0) {
			close(0); /* In child process */
			close(1);
			close(2);
			setsid();
		} else if (pID < 0) {
			perror("Fatal error: cannot fork process");
			exit(1); /* if error */
		} else {
			exit(0); /* The parent process can close */
		}
	}

	char logname[32];
	snprintf(logname, sizeof(logname), "resetmsmice[%i]", getpid());
	openlog(logname, LOG_CONS, LOG_USER);
	
	if (devnum_set) {
		logprintf("Checking for X.org compatibility mode on usb bus %i device %i...", busnum, devnum);
	} else {
		logprintf("Checking for X.org compatibility mode on all Microsoft usb mice plugged into the system...");
	}
	
	r = libusb_init(NULL);
	if (r < 0) {
		logprintf("Error initializing usb system: libusb returned value: %X", r);
		return r;
	}

	cnt = libusb_get_device_list(NULL, &devs);
	if (cnt < 0) {
		logprintf("Error getting usb device list: libusb returned value: %X", cnt);
		return cnt;
	}

	int i;
	int currentBus, currentDev;
	for (i = 0; i < cnt; i++) {
		dev = devs[i];
		if (dev == NULL) break; // this shouldn't happen

		currentBus = (int) libusb_get_bus_number(dev);
		currentDev = (int) libusb_get_device_address(dev);

		if (busnum_set && devnum_set) {
			if (currentBus != busnum || currentDev != devnum)
				continue;
		}
		
		r = libusb_get_device_descriptor(dev, &desc);
		if (r < 0) {
			logprintf("Failed to get device descriptor on usb device #%i", i);
			logprintf("libusb returned: %i", r);
			continue;
		}

		/*---------------------------------------------*/
		/* Skip to next device if not vendor id        */
		/*---------------------------------------------*/
		bool foundmatch = false;
		int x;
		for (x = 0; x < KNOWN_MICE_IDS; x++) {
			if (desc.idVendor == mousetype[x].vendor_id &&
				desc.idProduct == mousetype[x].model_id) {
				foundmatch = true;
				problem_devices++;
				break;
			}
		}
		
		if (foundmatch) {
			logprintf("Mouse detected that might not be in X.org compatible mode. Making sure device is in compatibility mode...");
			logprintf("Vendor Id: %04X  Product Id: %04X  Release: %i", 
				(int) mousetype[x].vendor_id, (int) mousetype[x].model_id, 
				(int) desc.bcdDevice);
			logprintf("Bus: %i  Device Number: %i", currentBus, currentDev);
			logprintf("Vendor Name: %s", mousetype[x].vendor_name);
			r = mousetype[x].probe(dev, &desc);
		}
	}
	libusb_free_device_list(devs, 1);

	libusb_exit(NULL);

	if (problem_devices == 0) {
		if (devnum_set) {
			logprintf("USB device at location %i:%i not known to be incompatible with X.org", busnum, devnum);
		} else {
			logprintf("No known X.org problematic mice attached to system.");
		}
	}
	return r; /* This program returns 0 on success, otherwise a libusb return code */
}
