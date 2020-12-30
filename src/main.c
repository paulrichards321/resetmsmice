#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <locale.h>
#include <ctype.h>
#include "resetmsmice.h"
#include "basictypes.h"

static const char* syntax1 = 
  "Syntax: resetmsmice [OPTIONS]\n";

static const char* syntax2 = 
  "If it is run with no options, check all Microsoft mice plugged into the\n"
  "system. If run with -k flag, only check known models that have this issue.\n"
  "All messages are printed on screen unless run as a daemon. Will always try\n"
  "to log to syslog.\n"
  "This program does not need root privileges to talk to the mouse as long\n"
  "as it's udev file is installed and the group ms-usb is created.\n\n"
  "The following are optional arguments, which are used to run smoother when\n"
  " called through startup scripts or udev:\n"
  "  -k, --only_known_models  only check for known models, don't look at the\n"
  "                           hid reports.\n"
  "  -i, --print_interfaces   useful for debugging, print out details of each\n"
  "                           HID interface\n"
  "  -b, --busnum=NUMBER      only check the device on this usb bus, and this\n"
  "  -d, --devnum=NUMBER      usb device number (useful with udev)\n"
  "  -u, --daemon             detach from the console and run in a subprocess,\n"
  "                           to return control to caller immediately\n"
  "                           (useful with startup scripts)\n"
  "  -h, --help               this help screen.\n"
  "\nReport bugs to the writer of this program:\n"
  "      Paul F. Richards (paulrichards321@gmail.com)";


void print_syntax(void)
{
  puts(syntax1);
  puts(resetmsmice_about());
  puts(syntax2);
}

int main(int argc, char** argv)
{
  int options = 0;
	int busnum = 0, devnum = 0;
	bool busnum_set = false, devnum_set = false;
	bool daemonize = false;
	bool only_known_models = false;
  bool print_interfaces = false;
	int choice;
	int option_index = 0;
	static struct option long_options[] =
	{
		{"only_known_models", no_argument, 0, 'k'},
		{"print_interfaces", no_argument, 0, 'i'},
		{"busnum", required_argument, 0, 'b'},
		{"devnum", required_argument, 0, 'd'},
		{"daemon", no_argument, 0, 'u'},
		{"help", no_argument, 0, 'h'},
		{0, 0, 0, 0}
	};
	const char* short_options = "kb:d:uih";
	
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
				print_syntax();
				exit(1);
			}
			break;
		case 'd':
			if (isdigit(optarg[0])) {
				devnum = atoi(optarg);
				devnum_set = true;
			} else {
				print_syntax();
				exit(1);
			}
			break;
		case 'u':
			daemonize = true;
			break;
		case 'k':
			only_known_models = true;
			break;
		case 'i':
			print_interfaces = true;
			break;
		case 'h':
			print_syntax();
			exit(0);
		default:
			print_syntax();
			exit(1);
		}
		choice = getopt_long(argc, argv, short_options, long_options, &option_index);
	}
	
	/* Make sure user supplied both the bus number and device number and not just one or the other... */
	if (devnum_set || busnum_set) {
		if (devnum_set == false || busnum_set == false) {
			puts("Please supply BOTH bus and device number.");
			print_syntax();
			exit(1);
		}
	}
  if (devnum_set) options |= RESETMSMICE_DEV_NUM_SET;
  if (busnum_set) options |= RESETMSMICE_BUS_NUM_SET;
  if (daemonize) options |= RESETMSMICE_DAEMONIZE;
  if (only_known_models) options |= RESETMSMICE_ONLY_KNOWN_MODELS;
  if (print_interfaces) options |= RESETMSMICE_PRINT_INTERFACES;

  return resetmsmice(busnum, devnum, options);
}
