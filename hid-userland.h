/*  Large parts of this code copied from the Linux kernel. The parts I have written go under the same LICENSE.
 *  Copyright (c) (smaller portions of this header) 2012 Paul Richards
 *  Copyright (c) 1999 Andreas Gal
 *  Copyright (c) 2000-2001 Vojtech Pavlik
 *  Copyright (c) 2006-2007 Jiri Kosina
 *
 * Any questions please send to me and not the kernel writers above. paulrichards321@gmail.com
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Should you need to contact me, the author, you can do so either by
 * e-mail - mail your message to <vojtech@ucw.cz>, or by paper mail:
 * Vojtech Pavlik, Simunkova 1594, Prague 8, 182 00 Czech Republic
 */

/*
 * We parse each description item into this structure. Short items data
 * values are expanded to 32-bit signed int, long items contain a pointer
 * into the data area.
 */

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

#define __u8 uint8_t
#define __s8 int8_t
#define __u16 uint16_t
#define __s16 int16_t
#define __le16 uint16_t
#define __u32 uint32_t
#define __s32 int32_t
#define __le32 uint32_t
#define __u64 uint64_t
#define __s64 int64_t
#define __le64 uint64_t

#define u8 uint8_t
#define s8 int8_t
#define u16 uint16_t
#define s16 int16_t
#define u32 uint32_t
#define s32 int32_t
#define u64 uint64_t
#define s64 int64_t

#define max_t(type, x, y) ({            \
    type __max1 = (x);          \
    type __max2 = (y);          \
    __max1 > __max2 ? __max1: __max2; })


struct list_head {
    struct list_head *next, *prev;
};

#define USB_INTERFACE_SUBCLASS_BOOT 1
#define USB_INTERFACE_PROTOCOL_KEYBOARD 1
#define USB_INTERFACE_PROTOCOL_MOUSE 2

struct hid_item {
	unsigned  format;
	__u8      size;
	__u8      type;
	__u8      tag;
	union {
	    __u8   u8;
	    __s8   s8;
	    __u16  u16;
	    __s16  s16;
	    __u32  u32;
	    __s32  s32;
	    __u8  *longdata;
	} data;
};

/*
 * HID report item format
 */

#define HID_ITEM_FORMAT_SHORT	0
#define HID_ITEM_FORMAT_LONG	1

/*
 * Special tag indicating long items
 */

#define HID_ITEM_TAG_LONG	15

/*
 * HID report descriptor item type (prefix bit 2,3)
 */

#define HID_ITEM_TYPE_MAIN		0
#define HID_ITEM_TYPE_GLOBAL		1
#define HID_ITEM_TYPE_LOCAL		2
#define HID_ITEM_TYPE_RESERVED		3

/*
 * HID report descriptor main item tags
 */

#define HID_MAIN_ITEM_TAG_INPUT			8
#define HID_MAIN_ITEM_TAG_OUTPUT		9
#define HID_MAIN_ITEM_TAG_FEATURE		11
#define HID_MAIN_ITEM_TAG_BEGIN_COLLECTION	10
#define HID_MAIN_ITEM_TAG_END_COLLECTION	12

/*
 * HID report descriptor main item contents
 */

#define HID_MAIN_ITEM_CONSTANT		0x001
#define HID_MAIN_ITEM_VARIABLE		0x002
#define HID_MAIN_ITEM_RELATIVE		0x004
#define HID_MAIN_ITEM_WRAP		0x008
#define HID_MAIN_ITEM_NONLINEAR		0x010
#define HID_MAIN_ITEM_NO_PREFERRED	0x020
#define HID_MAIN_ITEM_NULL_STATE	0x040
#define HID_MAIN_ITEM_VOLATILE		0x080
#define HID_MAIN_ITEM_BUFFERED_BYTE	0x100

/*
 * HID report descriptor collection item types
 */

#define HID_COLLECTION_PHYSICAL		0
#define HID_COLLECTION_APPLICATION	1
#define HID_COLLECTION_LOGICAL		2

/*
 * HID report descriptor global item tags
 */

#define HID_GLOBAL_ITEM_TAG_USAGE_PAGE		0
#define HID_GLOBAL_ITEM_TAG_LOGICAL_MINIMUM	1
#define HID_GLOBAL_ITEM_TAG_LOGICAL_MAXIMUM	2
#define HID_GLOBAL_ITEM_TAG_PHYSICAL_MINIMUM	3
#define HID_GLOBAL_ITEM_TAG_PHYSICAL_MAXIMUM	4
#define HID_GLOBAL_ITEM_TAG_UNIT_EXPONENT	5
#define HID_GLOBAL_ITEM_TAG_UNIT		6
#define HID_GLOBAL_ITEM_TAG_REPORT_SIZE		7
#define HID_GLOBAL_ITEM_TAG_REPORT_ID		8
#define HID_GLOBAL_ITEM_TAG_REPORT_COUNT	9
#define HID_GLOBAL_ITEM_TAG_PUSH		10
#define HID_GLOBAL_ITEM_TAG_POP			11

/*
 * HID report descriptor local item tags
 */

#define HID_LOCAL_ITEM_TAG_USAGE		0
#define HID_LOCAL_ITEM_TAG_USAGE_MINIMUM	1
#define HID_LOCAL_ITEM_TAG_USAGE_MAXIMUM	2
#define HID_LOCAL_ITEM_TAG_DESIGNATOR_INDEX	3
#define HID_LOCAL_ITEM_TAG_DESIGNATOR_MINIMUM	4
#define HID_LOCAL_ITEM_TAG_DESIGNATOR_MAXIMUM	5
#define HID_LOCAL_ITEM_TAG_STRING_INDEX		7
#define HID_LOCAL_ITEM_TAG_STRING_MINIMUM	8
#define HID_LOCAL_ITEM_TAG_STRING_MAXIMUM	9
#define HID_LOCAL_ITEM_TAG_DELIMITER		10

/*
 * HID usage tables
 */

#define HID_USAGE_PAGE		0xffff0000

#define HID_UP_UNDEFINED	0x00000000
#define HID_UP_GENDESK		0x00010000
#define HID_UP_SIMULATION	0x00020000
#define HID_UP_GENDEVCTRLS	0x00060000
#define HID_UP_KEYBOARD		0x00070000
#define HID_UP_LED		0x00080000
#define HID_UP_BUTTON		0x00090000
#define HID_UP_ORDINAL		0x000a0000
#define HID_UP_CONSUMER		0x000c0000
#define HID_UP_DIGITIZER	0x000d0000
#define HID_UP_PID		0x000f0000
#define HID_UP_HPVENDOR         0xff7f0000
#define HID_UP_MSVENDOR		0xff000000
#define HID_UP_CUSTOM		0x00ff0000
#define HID_UP_LOGIVENDOR	0xffbc0000

#define HID_USAGE		0x0000ffff

#define HID_GD_POINTER		0x00010001
#define HID_GD_MOUSE		0x00010002
#define HID_GD_JOYSTICK		0x00010004
#define HID_GD_GAMEPAD		0x00010005
#define HID_GD_KEYBOARD		0x00010006
#define HID_GD_KEYPAD		0x00010007
#define HID_GD_MULTIAXIS	0x00010008
#define HID_GD_X		0x00010030
#define HID_GD_Y		0x00010031
#define HID_GD_Z		0x00010032
#define HID_GD_RX		0x00010033
#define HID_GD_RY		0x00010034
#define HID_GD_RZ		0x00010035
#define HID_GD_SLIDER		0x00010036
#define HID_GD_DIAL		0x00010037
#define HID_GD_WHEEL		0x00010038
#define HID_GD_HATSWITCH	0x00010039
#define HID_GD_BUFFER		0x0001003a
#define HID_GD_BYTECOUNT	0x0001003b
#define HID_GD_MOTION		0x0001003c
#define HID_GD_START		0x0001003d
#define HID_GD_SELECT		0x0001003e
#define HID_GD_VX		0x00010040
#define HID_GD_VY		0x00010041
#define HID_GD_VZ		0x00010042
#define HID_GD_VBRX		0x00010043
#define HID_GD_VBRY		0x00010044
#define HID_GD_VBRZ		0x00010045
#define HID_GD_VNO		0x00010046
#define HID_GD_FEATURE		0x00010047
#define HID_GD_UP		0x00010090
#define HID_GD_DOWN		0x00010091
#define HID_GD_RIGHT		0x00010092
#define HID_GD_LEFT		0x00010093

#define HID_DC_BATTERYSTRENGTH	0x00060020

#define HID_DG_DIGITIZER	0x000d0001
#define HID_DG_PEN		0x000d0002
#define HID_DG_LIGHTPEN		0x000d0003
#define HID_DG_TOUCHSCREEN	0x000d0004
#define HID_DG_TOUCHPAD		0x000d0005
#define HID_DG_STYLUS		0x000d0020
#define HID_DG_PUCK		0x000d0021
#define HID_DG_FINGER		0x000d0022
#define HID_DG_TIPPRESSURE	0x000d0030
#define HID_DG_BARRELPRESSURE	0x000d0031
#define HID_DG_INRANGE		0x000d0032
#define HID_DG_TOUCH		0x000d0033
#define HID_DG_UNTOUCH		0x000d0034
#define HID_DG_TAP		0x000d0035
#define HID_DG_TABLETFUNCTIONKEY	0x000d0039
#define HID_DG_PROGRAMCHANGEKEY	0x000d003a
#define HID_DG_INVERT		0x000d003c
#define HID_DG_TIPSWITCH	0x000d0042
#define HID_DG_TIPSWITCH2	0x000d0043
#define HID_DG_BARRELSWITCH	0x000d0044
#define HID_DG_ERASER		0x000d0045
#define HID_DG_TABLETPICK	0x000d0046
/*
 * as of May 20, 2009 the usages below are not yet in the official USB spec
 * but are being pushed by Microsft as described in their paper "Digitizer
 * Drivers for Windows Touch and Pen-Based Computers"
 */
#define HID_DG_CONFIDENCE	0x000d0047
#define HID_DG_WIDTH		0x000d0048
#define HID_DG_HEIGHT		0x000d0049
#define HID_DG_CONTACTID	0x000d0051
#define HID_DG_INPUTMODE	0x000d0052
#define HID_DG_DEVICEINDEX	0x000d0053
#define HID_DG_CONTACTCOUNT	0x000d0054
#define HID_DG_CONTACTMAX	0x000d0055

/*
 * HID report types --- Ouch! HID spec says 1 2 3!
 */

#define HID_INPUT_REPORT	0
#define HID_OUTPUT_REPORT	1
#define HID_FEATURE_REPORT	2

/*
 * HID connect requests
 */

#define HID_CONNECT_HIDINPUT		0x01
#define HID_CONNECT_HIDINPUT_FORCE	0x02
#define HID_CONNECT_HIDRAW		0x04
#define HID_CONNECT_HIDDEV		0x08
#define HID_CONNECT_HIDDEV_FORCE	0x10
#define HID_CONNECT_FF			0x20
#define HID_CONNECT_DEFAULT	(HID_CONNECT_HIDINPUT|HID_CONNECT_HIDRAW| \
		HID_CONNECT_HIDDEV|HID_CONNECT_FF)

/*
 * HID device quirks.
 */

/* 
 * Increase this if you need to configure more HID quirks at module load time
 */
#define MAX_USBHID_BOOT_QUIRKS 4

#define HID_QUIRK_INVERT			0x00000001
#define HID_QUIRK_NOTOUCH			0x00000002
#define HID_QUIRK_IGNORE			0x00000004
#define HID_QUIRK_NOGET				0x00000008
#define HID_QUIRK_HIDDEV_FORCE			0x00000010
#define HID_QUIRK_BADPAD			0x00000020
#define HID_QUIRK_MULTI_INPUT			0x00000040
#define HID_QUIRK_HIDINPUT_FORCE		0x00000080
#define HID_QUIRK_MULTITOUCH			0x00000100
#define HID_QUIRK_SKIP_OUTPUT_REPORTS		0x00010000
#define HID_QUIRK_FULLSPEED_INTERVAL		0x10000000
#define HID_QUIRK_NO_INIT_REPORTS		0x20000000
#define HID_QUIRK_NO_IGNORE			0x40000000
#define HID_QUIRK_NO_INPUT_SYNC			0x80000000

/*
 * This is the global environment of the parser. This information is
 * persistent for main-items. The global environment can be saved and
 * restored with PUSH/POP statements.
 */

struct hid_global {
	unsigned usage_page;
	__s32    logical_minimum;
	__s32    logical_maximum;
	__s32    physical_minimum;
	__s32    physical_maximum;
	__s32    unit_exponent;
	unsigned unit;
	unsigned report_id;
	unsigned report_size;
	unsigned report_count;
};

/*
 * This is the local environment. It is persistent up the next main-item.
 */

#define HID_MAX_USAGES			12288
#define HID_DEFAULT_NUM_COLLECTIONS	16

struct hid_local {
	unsigned usage[HID_MAX_USAGES]; /* usage array */
	unsigned collection_index[HID_MAX_USAGES]; /* collection index array */
	unsigned usage_index;
	unsigned usage_minimum;
	unsigned delimiter_depth;
	unsigned delimiter_branch;
};

/*
 * This is the collection stack. We climb up the stack to determine
 * application and function of each field.
 */

struct hid_collection {
	unsigned type;
	unsigned usage;
	unsigned level;
};

struct hid_usage {
	unsigned  hid;			/* hid usage code */
	unsigned  collection_index;	/* index into collection array */
	/* hidinput data */
	__u16     code;			/* input driver code */
	__u8      type;			/* input driver type */
	__s8	  hat_min;		/* hat switch fun */
	__s8	  hat_max;		/* ditto */
	__s8	  hat_dir;		/* ditto */
};

struct hid_input;

struct hid_field {
	unsigned  physical;		/* physical usage for this field */
	unsigned  logical;		/* logical usage for this field */
	unsigned  application;		/* application usage for this field */
	struct hid_usage *usage;	/* usage table for this function */
	unsigned  maxusage;		/* maximum usage index */
	unsigned  flags;		/* main-item flags (i.e. volatile,array,constant) */
	unsigned  report_offset;	/* bit offset in the report */
	unsigned  report_size;		/* size of this field in the report */
	unsigned  report_count;		/* number of this field in the report */
	unsigned  report_type;		/* (input,output,feature) */
	__s32    *value;		/* last known value(s) */
	__s32     logical_minimum;
	__s32     logical_maximum;
	__s32     physical_minimum;
	__s32     physical_maximum;
	__s32     unit_exponent;
	unsigned  unit;
	struct hid_report *report;	/* associated report */
	unsigned index;			/* index into report->field[] */
	/* hidinput data */
	struct hid_input *hidinput;	/* associated input structure */
	__u16 dpad;			/* dpad input code */
};

#define HID_MAX_FIELDS 128

struct hid_report {
	struct list_head list;
	unsigned id;					/* id of this report */
	unsigned type;					/* report type */
	struct hid_field *field[HID_MAX_FIELDS];	/* fields of the report */
	unsigned maxfield;				/* maximum valid field index */
	unsigned size;					/* size of the report (bits) */
	struct hid_device *device;			/* associated device */
};

struct hid_report_enum {
	unsigned numbered;
	struct list_head report_list;
	struct hid_report *report_id_hash[256];
};

#define HID_REPORT_TYPES 3

#define HID_MIN_BUFFER_SIZE	64		/* make sure there is at least a packet size of space */
#define HID_MAX_BUFFER_SIZE	4096		/* 4kb */
#define HID_CONTROL_FIFO_SIZE	256		/* to init devices with >100 reports */
#define HID_OUTPUT_FIFO_SIZE	64

struct hid_control_fifo {
	unsigned char dir;
	struct hid_report *report;
	char *raw_report;
};

struct hid_output_fifo {
	struct hid_report *report;
	char *raw_report;
};

#define HID_CLAIMED_INPUT	1
#define HID_CLAIMED_HIDDEV	2
#define HID_CLAIMED_HIDRAW	4

#define HID_STAT_ADDED		1
#define HID_STAT_PARSED		2

struct hid_input {
	struct list_head list;
	struct hid_report *report;
	struct input_dev *input;
};

enum hid_type {
	HID_TYPE_OTHER = 0,
	HID_TYPE_USBMOUSE,
	HID_TYPE_USBNONE
};

struct hid_driver;
struct hid_ll_driver;


#define HID_GLOBAL_STACK_SIZE 4
#define HID_COLLECTION_STACK_SIZE 4

struct hid_parser {
	struct hid_global     global;
	struct hid_global     global_stack[HID_GLOBAL_STACK_SIZE];
	unsigned              global_stack_ptr;
	struct hid_local      local;
	unsigned              collection_stack[HID_COLLECTION_STACK_SIZE];
	unsigned              collection_stack_ptr;
	struct hid_device    *device;
};

struct hid_class_descriptor {
	__u8  bDescriptorType;
	__le16 wDescriptorLength;
} __attribute__ ((packed));

struct hid_descriptor {
	__u8  bLength;
	__u8  bDescriptorType;
	__le16 bcdHID;
	__u8  bCountryCode;
	__u8  bNumDescriptors;

	struct hid_class_descriptor desc[1];
} __attribute__ ((packed));

#define HID_DEVICE(b, ven, prod) \
	.bus = (b), \
	.vendor = (ven), .product = (prod)

#define HID_USB_DEVICE(ven, prod)	HID_DEVICE(BUS_USB, ven, prod)
#define HID_BLUETOOTH_DEVICE(ven, prod)	HID_DEVICE(BUS_BLUETOOTH, ven, prod)

#define HID_REPORT_ID(rep) \
	.report_type = (rep)
#define HID_USAGE_ID(uhid, utype, ucode) \
	.usage_hid = (uhid), .usage_type = (utype), .usage_code = (ucode)
/* we don't want to catch types and codes equal to 0 */
#define HID_TERMINATOR		(HID_ANY_ID - 1)

struct hid_report_id {
	__u32 report_type;
};
struct hid_usage_id {
	__u32 usage_hid;
	__u32 usage_type;
	__u32 usage_code;
};



#define USB_TYPE_CLASS (0x01 << 5)
#define HID_DT_HID (USB_TYPE_CLASS | 0x01)
#define HID_DT_REPORT (USB_TYPE_CLASS | 0x02)
#define USB_REQ_GET_DESCRIPTOR 0x06
#define HID_MAX_DESCRIPTOR_SIZE     4096

#if BYTE_ORDER == LITTLE_ENDIAN
#define le16_to_cpu(val) (val)
#define le32_to_cpu(val) (val)
#endif
#if BYTE_ORDER == BIG_ENDIAN
#define le16_to_cpu(val) fn_bswap_16(val)
#define le32_to_cpu(val) fn_bswap_32(val)
#endif

/* Swap bytes in 16 bit value.  */
#define bswap_constant_16(x) \
     ((unsigned short int) ((((x) >> 8) & 0xff) | (((x) & 0xff) << 8)))

/* Swap bytes in 32 bit value.  */
#define bswap_constant_32(x) \
     ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) |           \
      (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))

static inline unsigned short int fn_bswap_16(unsigned short int bsx)
{
  return bswap_constant_16(bsx);
}

static inline unsigned int fn_bswap_32(unsigned int bsx)
{
  return bswap_constant_32(bsx);
}

void *memdup(void *src, size_t size);

#define TIME_OUT_VAL 1000 // in milliseconds


struct hid_device {
	struct list_head list;
	__u8 *rdesc;
	unsigned rsize;
	struct hid_collection *collection;	/* List of HID collections */
	unsigned collection_size;			/* Number of allocated hid_collections */
	unsigned maxcollection;				/* Number of parsed collections */
	unsigned maxapplication;			/* Number of applications */
	__u16 bus;						/* BUS ID */
    __u32 vendor;					/* Vendor ID */
    __u32 product;					/* Product ID */
    __u32 version;					/* HID version */
    enum hid_type type;					/* device type (mouse, kbd, ...) */
    unsigned country;					/* HID country */
	struct hid_report_enum report_enum[HID_REPORT_TYPES];

	struct libusb_device *usb_dev;
	struct libusb_device_descriptor *usb_desc;
	const struct libusb_interface_descriptor *current_intf;
	struct libusb_device_handle *usb_devh;

	struct libusb_transfer *transfer;
	__u8 intf_num;
	__u8 intf_protocol;
	__u8 intf_subclass;
	__u8 input_endpoint;
	__u8 output_endpoint;
	__u16 max_packet_size;

	/* interrupt thread objects */
	pthread_t thread;
	pthread_mutex_t mutex; /* Protects input_reports */
	pthread_cond_t condition;
	pthread_barrier_t barrier; /* Ensures correct startup sequence */

	bool claimed;
	bool kerneldriver;
	bool thread_init;
	bool shutdown_thread;
};

#define dbg_hid(format, arg...)						\
do {									\
	if (hid_debug)							\
		printf(format, ##arg);	\
} while (0)

#define hid_printk(level, hid, fmt, arg...)		\
	print(level, &(hid)->dev, fmt, ##arg)
#define hid_emerg(hid, fmt, arg...)			\
	printf(fmt, ##arg)
#define hid_crit(hid, fmt, arg...)			\
	printf(fmt, ##arg)
#define hid_alert(hid, fmt, arg...)			\
	printf(fmt, ##arg)
#define hid_err(hid, fmt, arg...)			\
	printf(fmt, ##arg)
#define hid_notice(hid, fmt, arg...)			\
	printf(fmt, ##arg)
#define hid_warn(hid, fmt, arg...)			\
	printf(fmt, ##arg)
#define hid_info(hid, fmt, arg...)			\
	printf(fmt, ##arg)
#define hid_dbg(hid, fmt, arg...)			\
	printf(fmt, ##arg)

/* Below macro taken from include/linux/hid-debug.h */
#define hid_dump_input(a,b,c)       do { } while (0)

/* Below unaligned functions taken from include/linux/unaligned/le_byteshift.h */
static inline u16 __get_unaligned_le16(const u8 *p)
{
    return p[0] | p[1] << 8;
}

static inline u32 __get_unaligned_le32(const u8 *p)
{
    return p[0] | p[1] << 8 | p[2] << 16 | p[3] << 24;
}

static inline u64 __get_unaligned_le64(const u8 *p)
{
    return (u64)__get_unaligned_le32(p + 4) << 32 |
           __get_unaligned_le32(p);
}

static inline void __put_unaligned_le16(u16 val, u8 *p)
{
    *p++ = val;
    *p++ = val >> 8;
}

static inline void __put_unaligned_le32(u32 val, u8 *p)
{
    __put_unaligned_le16(val >> 16, p + 2);
    __put_unaligned_le16(val, p);
}

static inline void __put_unaligned_le64(u64 val, u8 *p)
{
    __put_unaligned_le32(val >> 32, p + 4);
    __put_unaligned_le32(val, p);
}

static inline u16 get_unaligned_le16(const void *p)
{
    return __get_unaligned_le16((const u8 *)p);
}

static inline u32 get_unaligned_le32(const void *p)
{
    return __get_unaligned_le32((const u8 *)p);
}

static inline u64 get_unaligned_le64(const void *p)
{
    return __get_unaligned_le64((const u8 *)p);
}

static inline void put_unaligned_le16(u16 val, void *p)
{
    __put_unaligned_le16(val, p);
}

static inline void put_unaligned_le32(u32 val, void *p)
{
    __put_unaligned_le32(val, p);
}

static inline void put_unaligned_le64(u64 val, void *p)
{
    __put_unaligned_le64(val, p);
}

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#define container_of(ptr, type, member) ({          \
    const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
    (type *)( (char *)__mptr - offsetof(type,member) );})

/**
 * list_entry - get the struct for this entry
 * @ptr:	the &struct list_head pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 */
#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)

/**
 * list_for_each	-	iterate over a list
 * @pos:	the &struct list_head to use as a loop cursor.
 * @head:	the head for your list.
 */
#define list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * list_for_each_entry_safe - iterate over list of given type safe against removal of list entry
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 */
#define list_for_each_entry_safe(pos, n, head, member)			\
	for (pos = list_entry((head)->next, typeof(*pos), member),	\
		n = list_entry(pos->member.next, typeof(*pos), member);	\
	     &pos->member != (head); 					\
	     pos = n, n = list_entry(n->member.next, typeof(*n), member))

static inline void INIT_LIST_HEAD(struct list_head *list)
{
	list->next = list;
	list->prev = list;
}

/*
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_add(struct list_head *new,
                  struct list_head *prev,
                  struct list_head *next)
{
    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}


/**
 * list_add_tail - add a new entry
 * @new: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
    __list_add(new, head->prev, head);
}


int usbhid_parse(struct hid_device*);
void hid_device_release(struct hid_device*);
void logprintf(const char *format, ...);
int hid_parse_report(struct hid_device *device, __u8 *start,
		unsigned size);

