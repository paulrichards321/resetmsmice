/*
 * Fixes scroll wheel issues with certain Microsoft mice in X.org
 * (includes KDE & Gnome applications) or OS X, where the vertical
 * wheel scrolls abnormally fast. Only needed if you dual boot between
 * Microsoft Windows and Mac OS X or some linux distro.
 *
 * Known to fix the vertical scroll wheel issue with the following
 * models (and likely others related):
 *
 *    Microsoft Wireless Mouse 1000
 *    Microsoft Wireless Optical Desktop 3000
 *    Microsoft Wireless Mobile Mouse 3500
 *    Microsoft Wireless Mobile Mouse 4000
 *    Microsoft Comfort Mouse 4500
 *    Microsoft Wireless Mouse 5000
 *    Microsoft Sculpt Mouse
 *
 * Copyright (C) 2011-2020  Paul F. Richards (paulrichards321@gmail.com)
 *
 * Parts of this code copyrighted by Alan Ott, Signal 11 Software
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

static const char * about = 
  "Fixes scroll wheel issues with certain Wireless Microsoft mice in X.org\n"
  "(includes KDE & Gnome applications), where the vertical wheel scrolls\n"
  "abnormally fast. Only needed if you dual boot between Microsoft Windows\n"
  "and some linux distro.\n\n"
  "Known to fix the vertical scroll wheel issue with the following models\n"
  "(and likely others related):\n"
  "*    Microsoft Wireless Mouse 1000\n"
  "*    Microsoft Wireless Optical Desktop 3000\n"
  "*    Microsoft Wireless Mobile Mouse 3500\n"
  "*    Microsoft Wireless Mobile Mouse 4000\n"
  "*    Microsoft Comfort Mouse 4500\n"
  "*    Microsoft Wireless Mouse 5000\n"
  "*    Microsoft Sculpt Mouse\n"
  "This program basically just resets a setting in the mouse through usb\n"
  "communications and then exits.\n";
 
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
#include <pthread.h>
#include "pthread-misc.h"
#include "hid-userland.h"
#include "resetmsmice.h"

static void hid_cleanup(struct hid_device*);
static int mouse_open(struct hid_device*);
static bool ms_probe_mouse_x17(struct libusb_device*, struct libusb_device_descriptor*, int* status);

static int print_usb_error(int error_value);

#define KNOWN_MICE_IDS 3

struct MOUSE_TYPE {
	uint16_t vendor_id;
	uint16_t model_id;
	const char* vendor_name;
	const char* model_notes;
    bool (*probe)(struct libusb_device*, struct libusb_device_descriptor*, int*);
} 
mousetype[KNOWN_MICE_IDS] = { 
	{ 0x045e, 0x0745, "Microsoft", "Microsoft Wireless Mouse Series", ms_probe_mouse_x17 },
	{ 0x045e, 0x00F9, "Microsoft", "Microsoft Wireless Desktop Receiver 3.1", ms_probe_mouse_x17 },
	{ 0x045e, 0x076c, "Microsoft", "Microsoft Corp. Comfort Mouse 4500", ms_probe_mouse_x17 }
//	{ 0x045e, 0x00e1, "Microsoft", "Microsoft Corp. Wireless Laser Mouse 6000 Reciever", ms_probe_mouse_x17 },
};

#define TIME_OUT_VAL 1000 // in milliseconds
bool print_interfaces = false;
void* logger_data = NULL;
void (*logger)(const char*, void*) = puts_logger;

struct libusb_context* usb_context;

void puts_logger(const char* text, void* data)
{
  puts(text);
}

void logprintf(const char *format, ...)
{
	va_list ap;
  char buf[512] = { 0 };

	va_start(ap, format);
	vsnprintf(buf, sizeof(buf), format, ap);
  va_end(ap);
	
	syslog(LOG_USER | LOG_NOTICE, "%s", buf);
  logger(buf, logger_data);
}

const char * resetmsmice_about()
{
  return about;
}

static void read_callback(struct libusb_transfer *transfer)
{
	struct hid_device *dev = (struct hid_device*) transfer->user_data;
	int r;
	
	if (transfer->status == LIBUSB_TRANSFER_COMPLETED) {
	} else if (transfer->status == LIBUSB_TRANSFER_CANCELLED) {
		dev->shutdown_thread = true;
		return;
	} else if (transfer->status == LIBUSB_TRANSFER_NO_DEVICE) {
		dev->shutdown_thread = true;
		return;
	} else if (transfer->status == LIBUSB_TRANSFER_TIMED_OUT) {
	}	
	/* Re-submit the transfer object. */
	r = libusb_submit_transfer(transfer);
	if (r != 0) {
		logprintf("Unable to submit URB. libusb error code: %d\n", r);
		dev->shutdown_thread = true;
	}
}


static void *interrupt_thread(void *param)
{
	struct hid_device *dev = (struct hid_device*) param;
	struct timeval tv;
	unsigned char *buf;
	int r;

	buf = malloc(dev->max_packet_size);
	if (buf == NULL) {
		pthread_barrier_wait(&dev->barrier);
		return NULL;
	}
	
	/* Set up the transfer object. */
	dev->transfer = libusb_alloc_transfer(0);
	libusb_fill_interrupt_transfer(dev->transfer,
		dev->usb_devh,
		dev->input_endpoint,
		buf,
		dev->max_packet_size,
		read_callback,
		dev,
		5000/*timeout*/);
	
	/* Make the first submission. Further submissions are made
	   from inside read_callback() */
	r = libusb_submit_transfer(dev->transfer);
	if (r != 0) {
		logprintf("Error submitting interrupt to mouse.");
		print_usb_error(r);
	}

	/* Notify the main thread that the read thread is up and running. */
	pthread_barrier_wait(&dev->barrier);
	
	/* Handle all the events. */
	tv.tv_sec = 0;
	tv.tv_usec = 500000;
	while (!dev->shutdown_thread) {
		r = libusb_handle_events_timeout(usb_context, &tv);
		if (r < 0) {
			/* There was an error. */
			logprintf("interrupt_thread(): libusb reports error # %d\n", r);

			/* Break out of this loop only on fatal error.*/
			if (r != LIBUSB_ERROR_BUSY &&
			    r != LIBUSB_ERROR_TIMEOUT &&
			    r != LIBUSB_ERROR_OVERFLOW &&
			    r != LIBUSB_ERROR_INTERRUPTED) {
				break;
			}
		}
	}
	
	/* Cancel any transfer that may be pending. This call will fail
	   if no transfers are pending, but that's OK. */
	if (libusb_cancel_transfer(dev->transfer) == 0) {
		/* The transfer was cancelled, so wait for its completion. */
		libusb_handle_events(usb_context);
	}
	
	/* Cleanup */
	free(buf);
	libusb_free_transfer(dev->transfer);

	return NULL;
}


static int reset_feature_x17(struct hid_device *dev, bool *success)
{
	int r;
	unsigned char datain[2] = { 0x17, 0x00 };
	unsigned char dataout[2] = { 0x17, 0x00 };
	bool doreset = false;
	
	*success = false;
	/*--------------------------*/
	/* Startup interrupt thread */
	/*--------------------------*/
	pthread_barrier_init(&dev->barrier, NULL, 2);

	/* Setup an interrupt handler, this is need for the control transfer */
	pthread_create(&dev->thread, NULL, interrupt_thread, dev);
						
	/* Wait here for the read thread to be initialized. */
	pthread_barrier_wait(&dev->barrier);

	dev->thread_init = true;
	
	/*-------------------------*/
	/* Request status of mouse */
	/*-------------------------*/
	r = libusb_control_transfer(dev->usb_devh, LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_ENDPOINT_IN | LIBUSB_RECIPIENT_INTERFACE, 0x01, 0x317, dev->intf_num, datain, 2, TIME_OUT_VAL);
	if (r > 0) {
		if (datain[0] == 0x17 && datain[1] == 0x00) {
			logprintf("Microsoft mouse already in X.org Windows compatibility mode.\n");
			*success = true;
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
		return r;
	}

	if (doreset) {
		/*---------------------------*/
		/* Send reset codes to mouse */
		/*---------------------------*/
		r = libusb_control_transfer(dev->usb_devh, LIBUSB_RECIPIENT_INTERFACE | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_ENDPOINT_OUT, 0x09, 0x317, dev->intf_num, dataout, 2, TIME_OUT_VAL);
		if (r == 2) {
			logprintf("Successfully set mouse in X Windows compatibility mode.\n");
			*success = true;
		} else {
			logprintf("Unexpected return code from feature reset.");
			print_usb_error(r);
			return r;
		}
	}
	return r;
}


static int mouse_open(struct hid_device *dev)
{
	int r;
	
	/*-----------------*/
	/* Open the device */
	/*-----------------*/
	r = libusb_open(dev->usb_dev, &dev->usb_devh);
	if (r != 0) {
		logprintf("Failed to open usb device.");
		print_usb_error(r);
		return r;
	}
	
	/*-------------------------------------------------------------*/
	/* Check for active kernel driver, if so, detach kernel driver */
	/*-------------------------------------------------------------*/
	if (libusb_kernel_driver_active(dev->usb_devh, dev->intf_num) == 1) {
		r = libusb_detach_kernel_driver(dev->usb_devh, dev->intf_num);
		if (r == 0) {
			dev->kerneldriver = true;
		} else {
			dev->kerneldriver = false;
			logprintf("Cannot open mouse, failed to detach kernel driver.");
			print_usb_error(r);
			return r;
		}
	}
		
	/*------------------------------------------------------------------*/
	/* Claim interface, a necessary library routine before sending data */
	/*------------------------------------------------------------------*/
	r = libusb_claim_interface(dev->usb_devh, dev->intf_num);
	if (r == 0) {
		dev->claimed = true;
	} else {
		dev->claimed = false;
		logprintf("Cannot open mouse and claim interface.");
		print_usb_error(r);
		return r;
	}
	return r;
}


struct intf_enumerate {
	struct list_head list;
	struct libusb_config_descriptor *conf_desc;
	bool got_conf_desc;
};


static void enum_hid_cleanup(struct intf_enumerate *intf_enum)
{
	if (intf_enum->got_conf_desc == true) {
		libusb_free_config_descriptor(intf_enum->conf_desc);
		intf_enum->got_conf_desc = false;
	}
}


static bool add_hid_intf(struct intf_enumerate *intf_enum, struct libusb_device *usb_dev, struct libusb_device_descriptor *usb_desc, const struct libusb_interface_descriptor *intf_desc)
{
	struct hid_device *dev;
	const struct libusb_endpoint_descriptor *ep;
	int i;
	
	dev = (struct hid_device*) malloc(sizeof(struct hid_device));
	if (dev == NULL) {
		return false;
	}
	memset(dev, 0, sizeof(struct hid_device));
	
	list_add_tail(&dev->list, &intf_enum->list);

	dev->usb_dev = usb_dev;
	dev->usb_desc = usb_desc;
	dev->current_intf = intf_desc;
	dev->intf_num = intf_desc->bInterfaceNumber;
	dev->intf_protocol = intf_desc->bInterfaceProtocol;
	dev->intf_subclass = intf_desc->bInterfaceSubClass;

	/* -------------------------------------*/
	/* Find the INPUT and OUTPUT endpoints. */
	/* -------------------------------------*/
	dev->max_packet_size = 255;
	for (i = 0; i < intf_desc->bNumEndpoints; i++) {
		ep = &intf_desc->endpoint[i];

		/* Determine the type and direction of this endpoint. */
		bool is_interrupt = (ep->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK)
							== LIBUSB_TRANSFER_TYPE_INTERRUPT ? true : false;
		bool is_output = (ep->bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK)
						== LIBUSB_ENDPOINT_OUT ? true : false;
		bool is_input = (ep->bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK)
						== LIBUSB_ENDPOINT_IN ? true : false;

		/* Decide whether to use it for intput or output. */
		if (dev->input_endpoint == 0 && is_interrupt && is_input) {
			/* Use this endpoint for INPUT */
			dev->input_endpoint = ep->bEndpointAddress;
			dev->max_packet_size = ep->wMaxPacketSize;
		}
		if (dev->output_endpoint == 0 && is_interrupt && is_output) {
			/* Use this endpoint for OUTPUT */
			dev->output_endpoint = ep->bEndpointAddress;
		}
	}
	return true;
}


static int enum_hid_interfaces(struct intf_enumerate *intf_enum, struct libusb_device *usb_dev, struct libusb_device_descriptor *usb_desc)
{
	struct libusb_config_descriptor *conf_desc = NULL;
	const struct libusb_interface *intf;
	const struct libusb_interface_descriptor *intf_desc;
	int r, j, k;

	INIT_LIST_HEAD(&intf_enum->list);

	r = libusb_get_active_config_descriptor(usb_dev, &conf_desc);
	if (r != 0) {
		logprintf("Cannot get active configuration descriptor.");
		print_usb_error(r);
		return r;
	}
	intf_enum->conf_desc = conf_desc;
	intf_enum->got_conf_desc = true;
	
	for (j = 0; j  < conf_desc->bNumInterfaces; j++) {
		intf = &conf_desc->interface[j];
		for (k = 0; k < intf->num_altsetting; k++) {
			intf_desc = &intf->altsetting[k];
			if (intf_desc->bInterfaceClass == LIBUSB_CLASS_HID) {
				add_hid_intf(intf_enum, usb_dev, usb_desc, intf_desc);
			}
		}
	}
	return 0;
}


static int print_model_name(struct hid_device *dev)
{
	unsigned char language_in[256];
	unsigned char model_name[256];
	uint16_t langid = 0;
	int r;

	/*-------------------------*/
	/* Request mouse language  */
	/*-------------------------*/
	memset(language_in, 0, sizeof(language_in));
	r = libusb_get_string_descriptor(dev->usb_devh, 0x00, 0x0000, language_in, 255);
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
	r = libusb_get_string_descriptor(dev->usb_devh, dev->usb_desc->iProduct, langid, model_name, 255);	
	if (r > 2 && r <= 255) {
		char model_name_utf[256];
		char * model_name_ptr = (char*) &model_name[2];
		char * model_name_utf_ptr = model_name_utf;
		size_t inbytes_left = r - 2;
 		size_t outbytes_left = sizeof(model_name_utf);	
		iconv_t iconv_ptr = iconv_open("UTF-8", "UCS2");
		
		memset(model_name_utf, 0, sizeof(model_name_utf));
		if (iconv_ptr) {
			iconv(iconv_ptr, &model_name_ptr, &inbytes_left, &model_name_utf_ptr, &outbytes_left);
			logprintf("Model Name: %s", model_name_utf);
			iconv_close(iconv_ptr);
		}
	} else {
		logprintf("Failed to get model name from mouse.");
		print_usb_error(r);
	}
	return r;
}


static void hid_cleanup(struct hid_device *dev) 
{
	if (dev->thread_init) {
		/* Cause interrupt_thread() to stop. */
		dev->shutdown_thread = true;

		/* Wait for interrupt_thread() to end. */
		pthread_join(dev->thread, NULL);
		
		pthread_barrier_destroy(&dev->barrier);
	}
	if (dev->claimed) libusb_release_interface(dev->usb_devh, dev->intf_num);
	if (dev->kerneldriver) libusb_attach_kernel_driver(dev->usb_devh, dev->intf_num);
	if (dev->usb_devh) libusb_close(dev->usb_devh);
}


static int print_usb_error(int error_value)
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
		logprintf("Cannot open Microsoft device: insufficient permissions.");
		logprintf("HINT: This program needs to run with root permissions!");
		break;
	}
	logprintf("libusb returned: %i", error_value);
	return error_value;
}


static bool ms_probe_mouse_x17(struct libusb_device *usb_dev, struct libusb_device_descriptor *usb_desc, int *status)
{
	struct intf_enumerate intf_enum;
	struct list_head *pos;
	struct hid_device *hid_dev, *hid_previous = NULL;
	struct hid_report_enum *report_enum_ptr;
	struct hid_report *report_ptr;
	/* struct hid_field *field_ptr; */
	bool success = false;

	memset(&intf_enum, 0, sizeof(intf_enum));
	*status = enum_hid_interfaces(&intf_enum, usb_dev, usb_desc);
	if (*status != 0) {
		return false;
	}
	list_for_each(pos, &intf_enum.list) {
		hid_dev = list_entry(pos, struct hid_device, list);
		if (hid_previous) {
			hid_cleanup(hid_previous);
		}
		hid_previous = hid_dev;

		if (print_interfaces) {
			logprintf("Found HID interface: Interface number %i, Interface subclass %i, interface protocol %i", 
					  (int) hid_dev->intf_num,
					  (int) hid_dev->intf_subclass,
					  (int) hid_dev->intf_protocol);
		}
		if (hid_dev->intf_protocol == USB_INTERFACE_PROTOCOL_MOUSE) {
			/* Eeek! A mouse!!! */
			logprintf("Mouse interface found, interface number=%i", (int) hid_dev->intf_num);
		} else if (hid_dev->intf_protocol == 0) {
			/* Could be a mouse */
			logprintf("Possible mouse interface found, interface number=%i", (int) hid_dev->intf_num);
		} else {
			/* If we get here than this probably isn't a mouse */
			continue;
		}
		*status = mouse_open(hid_dev);
		if (*status == LIBUSB_ERROR_ACCESS) {
            logprintf("Permission problem opening mouse.");
            break;
        } else if (*status != 0) {
			logprintf("Failed to open mouse, skipping to next device...");
            continue;
		}
		print_model_name(hid_dev);
		*status = usbhid_parse(hid_dev);
		if (*status != 0) {
			logprintf("Failed to parse HID reports, skipping to next device...");
			continue;
		}
		report_enum_ptr = &hid_dev->report_enum[HID_FEATURE_REPORT];
		report_ptr = report_enum_ptr->report_id_hash[0x17];
		if (report_ptr == NULL) {
			logprintf("Smooth scroll feature NOT found in HID reports.");
			continue;
		}
		/* Some wired Microsoft mice do not describe this field 
		   as a mouse related field so I'm commenting this out... 
		field_ptr = report_ptr->field[0];
		if (field_ptr && field_ptr->logical == HID_GD_MOUSE) { 
		*/
		logprintf("Smooth scroll feature *FOUND* in HID reports.");
		*status = reset_feature_x17(hid_dev, &success);
		if (success == true && print_interfaces == false) { 
			/* success! quit out! */
			break;
		}
	}
	if (hid_previous) {
		hid_cleanup(hid_previous);
	}
	enum_hid_cleanup(&intf_enum);
	return success;
}


void resetmsmice_set_logger(void (*new_logger)(const char*, void*), void* data)
{
  logger = new_logger;
  logger_data = data;
}


int resetmsmice(int busnum, int devnum, int options)
{
	int busnum_set = (options & RESETMSMICE_BUS_NUM_SET);
  int devnum_set = (options & RESETMSMICE_DEV_NUM_SET);
	int daemonize = (options & RESETMSMICE_DAEMONIZE);
	int only_known_models = (options & RESETMSMICE_ONLY_KNOWN_MODELS);
	libusb_device *usb_dev, **usb_devs;
	struct libusb_device_descriptor desc;
	pid_t pID;
	ssize_t cnt;
	int problem_devices = 0;
	int retval;

  print_interfaces = (options & RESETMSMICE_PRINT_INTERFACES);

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

	char logname[64];
	snprintf(logname, sizeof(logname), "resetmsmice[%i]", getpid());
	openlog(logname, LOG_CONS, LOG_USER);
	
	if (devnum_set) {
		logprintf("Checking for X.org compatibility mode on usb bus %i device %i...", busnum, devnum);
	} else {
		logprintf("Checking for X.org compatibility mode on all Microsoft usb mice plugged into the system...");
	}
	
	retval = libusb_init(&usb_context);
	if (retval < 0) {
		logprintf("Error initializing usb system: libusb returned value: %X", retval);
		return retval;
	}

	cnt = libusb_get_device_list(usb_context, &usb_devs);
	if (cnt < 0) {
		logprintf("Error getting usb device list: libusb returned value: %X", cnt);
		return cnt;
	}

	int i;
	int currentBus, currentDev;
	for (i = 0; i < cnt; i++) {
		usb_dev = usb_devs[i];
		if (usb_dev == NULL) break; // this shouldn't happen

		currentBus = (int) libusb_get_bus_number(usb_dev);
		currentDev = (int) libusb_get_device_address(usb_dev);

		if (busnum_set && devnum_set) {
			if (currentBus != busnum || currentDev != devnum)
				continue;
		}
		
		retval = libusb_get_device_descriptor(usb_dev, &desc);
		if (retval < 0) {
			logprintf("Failed to get device descriptor on usb device #%i", i);
			logprintf("libusb returned: %i", retval);
			break;
		}

		/*---------------------------------------------*/
		/* Skip to next device if not vendor id        */
		/*---------------------------------------------*/
		bool foundmatch = false;
		int x;
		for (x = 0; x < KNOWN_MICE_IDS; x++) {
			/* If we match the vendor id check the device out, */
			/* unless specified to only check out known models. */
			if ((desc.idVendor == mousetype[x].vendor_id && 
				only_known_models == false) || 
				(desc.idVendor == mousetype[x].vendor_id &&
				desc.idProduct == mousetype[x].model_id)) {
					foundmatch = true;
					problem_devices++;
					break;
			}
		}
		
		if (foundmatch) {
			printf("---------------------------------------------------\n");
			logprintf("Vendor Id: %04X  Product Id: %04X  Release: %i", 
				(int) desc.idVendor, (int) desc.idProduct, 
				(int) desc.bcdDevice);
			logprintf("Bus: %i  Device Number: %i", currentBus, currentDev);
			/* logprintf("Vendor Name: %s", mousetype[x].vendor_name); */
            if (mousetype[x].probe(usb_dev, &desc, &retval) == false && retval == LIBUSB_ERROR_ACCESS) break;
		}
	}
	libusb_free_device_list(usb_devs, 1);

	libusb_exit(usb_context);

	if (problem_devices == 0) {
		if (devnum_set) {
			logprintf("USB device at location %i:%i not known to be incompatible with X.org", busnum, devnum);
		} else {
			logprintf("No known X.org or Mac OS X problematic mice attached to system.");
		}
	}
	return retval; /* This program returns a positive value on success, otherwise a libusb return code */
}

