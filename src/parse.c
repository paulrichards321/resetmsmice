/* Modified code taken from the linux kernel, my changes go under the same license. Here's the copyright and license:
   Copyright (C) 1997, 1998, 2000, 2002, 2003, 2006, 2007, 2008, 2010, 2011
   Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

/* Thanks to Sverker Wiberg and Marc Ruiz Altisent for bug fixes */
/* and anyone else I forgot to mention I apologize. */ 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libusb-1.0/libusb.h>
#include <errno.h>
#include "pthread-misc.h"
#include "hid-userland.h"

const int hid_debug = 1;

int __usb_get_extra_descriptor(const unsigned char *buffer, unsigned size,
    unsigned char type, void **ptr);
#define usb_get_extra_descriptor(ifpoint, type, ptr) \
                __usb_get_extra_descriptor((ifpoint)->extra, \
                (ifpoint)->extra_length, \
                type, (void **)ptr)

/* All standard descriptors have these 2 fields at the beginning */
struct usb_descriptor_header {
	__u8 bLength;
	__u8 bDescriptorType;
} __attribute__ ((packed));

/*-------------------------------------------------------------------*/
/*
 * __usb_get_extra_descriptor() finds a descriptor of specific type in the
 * extra field of the interface and endpoint descriptor structs.
 */

int __usb_get_extra_descriptor(const unsigned char *buffer, unsigned size,
                   unsigned char type, void **ptr)
{
    struct usb_descriptor_header *header;

    while (size >= sizeof(struct usb_descriptor_header)) {
        header = (struct usb_descriptor_header *)buffer;

        if (header->bLength < 2) {
            logprintf("bogus descriptor, type %d length %d\n",
                header->bDescriptorType,
                header->bLength);
            return -1;
        }

        if (header->bDescriptorType == type) {
            *ptr = header;
            return 0;
        }

        buffer += header->bLength;
        size -= header->bLength;
    }
    return -1;
}


/*static int hid_get_class_descriptor(struct usb_device *dev, int ifnum,
		unsigned char type, void *buf, int size)*/
static int hid_get_class_descriptor(struct libusb_device_handle *devh, int ifnum,
		unsigned char type, void *buf, int size)
{
	int result, retries = 4;

	memset(buf, 0, size);

	do {
		/* result = usb_control_msg(dev, usb_rcvctrlpipe(dev, 0),
				USB_REQ_GET_DESCRIPTOR, USB_RECIP_INTERFACE | USB_DIR_IN,
				(type << 8), ifnum, buf, size, USB_CTRL_GET_TIMEOUT);*/
		result = libusb_control_transfer(devh, LIBUSB_ENDPOINT_IN | LIBUSB_RECIPIENT_INTERFACE, USB_REQ_GET_DESCRIPTOR, (type << 8), ifnum, buf, size, TIME_OUT_VAL);
		retries--;
	} while (result < size && retries);
	return result;
}


int usbhid_parse(struct hid_device *hid)
{
	/* struct usb_interface *intf = to_usb_interface(hid->dev.parent);
	struct usb_host_interface *interface = intf->cur_altsetting;
	struct usb_device *dev = interface_to_usbdev (intf); */
	const struct libusb_interface_descriptor *interface = hid->current_intf;
	struct hid_descriptor *hdesc;
	/* __u32 quirks = 0; */
	unsigned int rsize = 0;
	char *rdesc;
	int ret, n, i;

	hid->collection = calloc(HID_DEFAULT_NUM_COLLECTIONS*
			sizeof(struct hid_collection), 1);
	if (hid->collection == NULL) {
		logprintf("couldn't allocate hid collection memory\n");
		return -ENOMEM;	
	}
	hid->collection_size = HID_DEFAULT_NUM_COLLECTIONS;

	for (i = 0; i < HID_REPORT_TYPES; i++)
		INIT_LIST_HEAD(&hid->report_enum[i].report_list);
	
	/* TODO:
	quirks = usbhid_lookup_quirk(le16_to_cpu(dev->descriptor.idVendor),
			le16_to_cpu(dev->descriptor.idProduct));

	if (quirks & HID_QUIRK_IGNORE)
		return -ENODEV;
	*/

	/* Many keyboards and mice don't like to be polled for reports,
	 * so we will always set the HID_QUIRK_NOGET flag for them. */
	/*
	if (interface->desc.bInterfaceSubClass == USB_INTERFACE_SUBCLASS_BOOT) {
		if (interface->desc.bInterfaceProtocol == USB_INTERFACE_PROTOCOL_KEYBOARD ||
			interface->desc.bInterfaceProtocol == USB_INTERFACE_PROTOCOL_MOUSE)
				quirks |= HID_QUIRK_NOGET;
	}*/

	if (usb_get_extra_descriptor(interface, HID_DT_HID, &hdesc) &&
	    (!interface->bNumEndpoints ||
	     usb_get_extra_descriptor(&interface->endpoint[0], HID_DT_HID, &hdesc))) {
		logprintf("class descriptor not present\n");
		return -EINVAL;
	}
	
	hid->version = le16_to_cpu(hdesc->bcdHID);
	hid->country = hdesc->bCountryCode;

	for (n = 0; n < hdesc->bNumDescriptors; n++)
		if (hdesc->desc[n].bDescriptorType == HID_DT_REPORT)
			rsize = le16_to_cpu(hdesc->desc[n].wDescriptorLength);

	if (!rsize || rsize > HID_MAX_DESCRIPTOR_SIZE) {
		logprintf("weird size of report descriptor (%u)\n", rsize);
		return -EINVAL;
	}

	if (!(rdesc = calloc(rsize, 1))) {
		logprintf("couldn't allocate rdesc memory\n");
		return -ENOMEM;
	}

	/* Not sure if this is needed: ??? */
	/* hid_set_idle(dev, interface->desc.bInterfaceNumber, 0, 0); */

	ret = hid_get_class_descriptor(hid->usb_devh, interface->bInterfaceNumber,
			HID_DT_REPORT, rdesc, rsize);
	if (ret < 0) {
		logprintf("reading report descriptor failed\n");
		free(rdesc);
		goto err;
	}

	ret = hid_parse_report(hid, (__u8*)rdesc, rsize);
	free(rdesc);
	if (ret) {
		logprintf("parsing report descriptor failed\n");
		goto err;
	}

	/* hid->quirks |= quirks; */

	return 0;
err:
	return ret;
}


int hid_dump_report(char* rdesc, int size)
{
	int i;
	printf("rdesc dump:\n");
	for (i = 0; i < size; i++) {
		printf("%2X ", (unsigned int) ((unsigned char*)rdesc)[i]);
	}
	printf("\n");
	return 0;
}


/*
 * Register a new report for a device.
 */

struct hid_report *hid_register_report(struct hid_device *device, unsigned type, unsigned id)
{
	struct hid_report_enum *report_enum = device->report_enum + type;
	struct hid_report *report;

	if (report_enum->report_id_hash[id])
		return report_enum->report_id_hash[id];

	report = calloc(sizeof(struct hid_report), 1);
	if (!report)
		return NULL;

	if (id != 0)
		report_enum->numbered = 1;

	report->id = id;
	report->type = type;
	report->size = 0;
	report->device = device;
	report_enum->report_id_hash[id] = report;

	list_add_tail(&report->list, &report_enum->report_list);

	return report;
}

/*
 * Register a new field for this report.
 */

static struct hid_field *hid_register_field(struct hid_report *report, unsigned usages, unsigned values)
{
	struct hid_field *field;

	if (report->maxfield >= HID_MAX_FIELDS) {
		hid_err(report->device, "too many fields in report\n");
		return NULL;
	}

	field = calloc((sizeof(struct hid_field) +
			 usages * sizeof(struct hid_usage) +
			 values * sizeof(unsigned)), 1);
	if (!field)
		return NULL;

	field->index = report->maxfield++;
	report->field[field->index] = field;
	field->usage = (struct hid_usage *)(field + 1);
	field->value = (s32 *)(field->usage + usages);
	field->report = report;

	return field;
}

/*
 * Open a collection. The type/usage is pushed on the stack.
 */

static int open_collection(struct hid_parser *parser, unsigned type)
{
	struct hid_collection *collection;
	unsigned usage;

	usage = parser->local.usage[0];

	if (parser->collection_stack_ptr == HID_COLLECTION_STACK_SIZE) {
		hid_err(parser->device, "collection stack overflow\n");
		return -1;
	}

	if (parser->device->maxcollection == parser->device->collection_size) {
		collection = calloc(sizeof(struct hid_collection) *
				parser->device->collection_size * 2, 1);
		if (collection == NULL) {
			hid_err(parser->device, "failed to reallocate collection array\n");
			return -1;
		}
		memcpy(collection, parser->device->collection,
			sizeof(struct hid_collection) *
			parser->device->collection_size);
		memset(collection + parser->device->collection_size, 0,
			sizeof(struct hid_collection) *
			parser->device->collection_size);
		free(parser->device->collection);
		parser->device->collection = collection;
		parser->device->collection_size *= 2;
	}

	parser->collection_stack[parser->collection_stack_ptr++] =
		parser->device->maxcollection;

	collection = parser->device->collection +
		parser->device->maxcollection++;
	collection->type = type;
	collection->usage = usage;
	collection->level = parser->collection_stack_ptr - 1;

	if (type == HID_COLLECTION_APPLICATION)
		parser->device->maxapplication++;

	return 0;
}

/*
 * Close a collection.
 */

static int close_collection(struct hid_parser *parser)
{
	if (!parser->collection_stack_ptr) {
		hid_err(parser->device, "collection stack underflow\n");
		return -1;
	}
	parser->collection_stack_ptr--;
	return 0;
}

/*
 * Climb up the stack, search for the specified collection type
 * and return the usage.
 */

static unsigned hid_lookup_collection(struct hid_parser *parser, unsigned type)
{
	struct hid_collection *collection = parser->device->collection;
	int n;

	for (n = parser->collection_stack_ptr - 1; n >= 0; n--) {
		unsigned index = parser->collection_stack[n];
		if (collection[index].type == type)
			return collection[index].usage;
	}
	return 0; /* we know nothing about this usage type */
}

/*
 * Add a usage to the temporary parser table.
 */

static int hid_add_usage(struct hid_parser *parser, unsigned usage)
{
	if (parser->local.usage_index >= HID_MAX_USAGES) {
		hid_err(parser->device, "usage index exceeded\n");
		return -1;
	}
	parser->local.usage[parser->local.usage_index] = usage;
	parser->local.collection_index[parser->local.usage_index] =
		parser->collection_stack_ptr ?
		parser->collection_stack[parser->collection_stack_ptr - 1] : 0;
	parser->local.usage_index++;
	return 0;
}

/*
 * Register a new field for this report.
 */

static int hid_add_field(struct hid_parser *parser, unsigned report_type, unsigned flags)
{
	struct hid_report *report;
	struct hid_field *field;
	int usages;
	unsigned offset;
	int i;

	report = hid_register_report(parser->device, report_type, parser->global.report_id);
	if (!report) {
		hid_err(parser->device, "hid_register_report failed\n");
		return -1;
	}

	if (parser->global.logical_maximum < parser->global.logical_minimum) {
		hid_err(parser->device, "logical range invalid %d %d\n",
				parser->global.logical_minimum, parser->global.logical_maximum);
		return -1;
	}

	offset = report->size;
	report->size += parser->global.report_size * parser->global.report_count;

	if (!parser->local.usage_index) /* Ignore padding fields */
		return 0;

	usages = max_t(int, parser->local.usage_index, parser->global.report_count);

	field = hid_register_field(report, usages, parser->global.report_count);
	if (!field)
		return 0;

	field->physical = hid_lookup_collection(parser, HID_COLLECTION_PHYSICAL);
	field->logical = hid_lookup_collection(parser, HID_COLLECTION_LOGICAL);
	field->application = hid_lookup_collection(parser, HID_COLLECTION_APPLICATION);

	for (i = 0; i < usages; i++) {
		int j = i;
		/* Duplicate the last usage we parsed if we have excess values */
		if (i >= parser->local.usage_index)
			j = parser->local.usage_index - 1;
		field->usage[i].hid = parser->local.usage[j];
		field->usage[i].collection_index =
			parser->local.collection_index[j];
	}

	field->maxusage = usages;
	field->flags = flags;
	field->report_offset = offset;
	field->report_type = report_type;
	field->report_size = parser->global.report_size;
	field->report_count = parser->global.report_count;
	field->logical_minimum = parser->global.logical_minimum;
	field->logical_maximum = parser->global.logical_maximum;
	field->physical_minimum = parser->global.physical_minimum;
	field->physical_maximum = parser->global.physical_maximum;
	field->unit_exponent = parser->global.unit_exponent;
	field->unit = parser->global.unit;

	return 0;
}

/*
 * Read data value from item.
 */

static u32 item_udata(struct hid_item *item)
{
	switch (item->size) {
	case 1: return item->data.u8;
	case 2: return item->data.u16;
	case 4: return item->data.u32;
	}
	return 0;
}

static s32 item_sdata(struct hid_item *item)
{
	switch (item->size) {
	case 1: return item->data.s8;
	case 2: return item->data.s16;
	case 4: return item->data.s32;
	}
	return 0;
}

/*
 * Process a global item.
 */

static int hid_parser_global(struct hid_parser *parser, struct hid_item *item)
{
	switch (item->tag) {
	case HID_GLOBAL_ITEM_TAG_PUSH:

		if (parser->global_stack_ptr == HID_GLOBAL_STACK_SIZE) {
			hid_err(parser->device, "global environment stack overflow\n");
			return -1;
		}

		memcpy(parser->global_stack + parser->global_stack_ptr++,
			&parser->global, sizeof(struct hid_global));
		return 0;

	case HID_GLOBAL_ITEM_TAG_POP:

		if (!parser->global_stack_ptr) {
			hid_err(parser->device, "global environment stack underflow\n");
			return -1;
		}

		memcpy(&parser->global, parser->global_stack +
			--parser->global_stack_ptr, sizeof(struct hid_global));
		return 0;

	case HID_GLOBAL_ITEM_TAG_USAGE_PAGE:
		parser->global.usage_page = item_udata(item);
		return 0;

	case HID_GLOBAL_ITEM_TAG_LOGICAL_MINIMUM:
		parser->global.logical_minimum = item_sdata(item);
		return 0;

	case HID_GLOBAL_ITEM_TAG_LOGICAL_MAXIMUM:
		if (parser->global.logical_minimum < 0)
			parser->global.logical_maximum = item_sdata(item);
		else
			parser->global.logical_maximum = item_udata(item);
		return 0;

	case HID_GLOBAL_ITEM_TAG_PHYSICAL_MINIMUM:
		parser->global.physical_minimum = item_sdata(item);
		return 0;

	case HID_GLOBAL_ITEM_TAG_PHYSICAL_MAXIMUM:
		if (parser->global.physical_minimum < 0)
			parser->global.physical_maximum = item_sdata(item);
		else
			parser->global.physical_maximum = item_udata(item);
		return 0;

	case HID_GLOBAL_ITEM_TAG_UNIT_EXPONENT:
		parser->global.unit_exponent = item_sdata(item);
		return 0;

	case HID_GLOBAL_ITEM_TAG_UNIT:
		parser->global.unit = item_udata(item);
		return 0;

	case HID_GLOBAL_ITEM_TAG_REPORT_SIZE:
		parser->global.report_size = item_udata(item);
		if (parser->global.report_size > 96) {
			hid_err(parser->device, "invalid report_size %d\n",
					parser->global.report_size);
			return -1;
		}
		return 0;

	case HID_GLOBAL_ITEM_TAG_REPORT_COUNT:
		parser->global.report_count = item_udata(item);
		if (parser->global.report_count > HID_MAX_USAGES) {
			hid_err(parser->device, "invalid report_count %d\n",
					parser->global.report_count);
			return -1;
		}
		return 0;

	case HID_GLOBAL_ITEM_TAG_REPORT_ID:
		parser->global.report_id = item_udata(item);
		if (parser->global.report_id == 0) {
			hid_err(parser->device, "report_id 0 is invalid\n");
			return -1;
		}
		return 0;

	default:
		hid_err(parser->device, "unknown global tag 0x%x\n", item->tag);
		return -1;
	}
}

/*
 * Process a local item.
 */

static int hid_parser_local(struct hid_parser *parser, struct hid_item *item)
{
	__u32 data;
	unsigned n;

	data = item_udata(item);

	switch (item->tag) {
	case HID_LOCAL_ITEM_TAG_DELIMITER:

		if (data) {
			/*
			 * We treat items before the first delimiter
			 * as global to all usage sets (branch 0).
			 * In the moment we process only these global
			 * items and the first delimiter set.
			 */
			if (parser->local.delimiter_depth != 0) {
				hid_err(parser->device, "nested delimiters\n");
				return -1;
			}
			parser->local.delimiter_depth++;
			parser->local.delimiter_branch++;
		} else {
			if (parser->local.delimiter_depth < 1) {
				hid_err(parser->device, "bogus close delimiter\n");
				return -1;
			}
			parser->local.delimiter_depth--;
		}
		return 1;

	case HID_LOCAL_ITEM_TAG_USAGE:

		if (parser->local.delimiter_branch > 1) {
			dbg_hid("alternative usage ignored\n");
			return 0;
		}

		if (item->size <= 2)
			data = (parser->global.usage_page << 16) + data;

		return hid_add_usage(parser, data);

	case HID_LOCAL_ITEM_TAG_USAGE_MINIMUM:

		if (parser->local.delimiter_branch > 1) {
			dbg_hid("alternative usage ignored\n");
			return 0;
		}

		if (item->size <= 2)
			data = (parser->global.usage_page << 16) + data;

		parser->local.usage_minimum = data;
		return 0;

	case HID_LOCAL_ITEM_TAG_USAGE_MAXIMUM:

		if (parser->local.delimiter_branch > 1) {
			dbg_hid("alternative usage ignored\n");
			return 0;
		}

		if (item->size <= 2)
			data = (parser->global.usage_page << 16) + data;

		for (n = parser->local.usage_minimum; n <= data; n++)
			if (hid_add_usage(parser, n)) {
				dbg_hid("hid_add_usage failed\n");
				return -1;
			}
		return 0;

	default:

		dbg_hid("unknown local item tag 0x%x\n", item->tag);
		return 0;
	}
	return 0;
}

/*
 * Process a main item.
 */

static int hid_parser_main(struct hid_parser *parser, struct hid_item *item)
{
	__u32 data;
	int ret;

	data = item_udata(item);

	switch (item->tag) {
	case HID_MAIN_ITEM_TAG_BEGIN_COLLECTION:
		ret = open_collection(parser, data & 0xff);
		break;
	case HID_MAIN_ITEM_TAG_END_COLLECTION:
		ret = close_collection(parser);
		break;
	case HID_MAIN_ITEM_TAG_INPUT:
		ret = hid_add_field(parser, HID_INPUT_REPORT, data);
		break;
	case HID_MAIN_ITEM_TAG_OUTPUT:
		ret = hid_add_field(parser, HID_OUTPUT_REPORT, data);
		break;
	case HID_MAIN_ITEM_TAG_FEATURE:
		ret = hid_add_field(parser, HID_FEATURE_REPORT, data);
		break;
	default:
		hid_err(parser->device, "unknown main item tag 0x%x\n", item->tag);
		ret = 0;
	}

	memset(&parser->local, 0, sizeof(parser->local));	/* Reset the local parser environment */

	return ret;
}

/*
 * Process a reserved item.
 */

static int hid_parser_reserved(struct hid_parser *parser, struct hid_item *item)
{
	dbg_hid("reserved item type, tag 0x%x\n", item->tag);
	return 0;
}

/*
 * Free a report and all registered fields. The field->usage and
 * field->value table's are allocated behind the field, so we need
 * only to free(field) itself.
 */

static void hid_free_report(struct hid_report *report)
{
	unsigned n;

	for (n = 0; n < report->maxfield; n++)
		free(report->field[n]);
	free(report);
}

/*
 * Free a device structure, all reports, and all fields.
 */

/* static void hid_device_release(struct device *dev) */
void hid_device_release(struct hid_device *device)
{
	/* struct hid_device *device = container_of(dev, struct hid_device, dev); */
	unsigned i, j;

	for (i = 0; i < HID_REPORT_TYPES; i++) {
		struct hid_report_enum *report_enum = device->report_enum + i;

		for (j = 0; j < 256; j++) {
			struct hid_report *report = report_enum->report_id_hash[j];
			if (report)
				hid_free_report(report);
		}
	}

	if (device->rdesc)
		free(device->rdesc);
	if (device->collection)
		free(device->collection);
	free(device);
}

/*
 * Fetch a report description item from the data stream. We support long
 * items, though they are not used yet.
 */

static u8 *fetch_item(__u8 *start, __u8 *end, struct hid_item *item)
{
	u8 b;

	if ((end - start) <= 0)
		return NULL;

	b = *start++;

	item->type = (b >> 2) & 3;
	item->tag  = (b >> 4) & 15;

	if (item->tag == HID_ITEM_TAG_LONG) {

		item->format = HID_ITEM_FORMAT_LONG;

		if ((end - start) < 2)
			return NULL;

		item->size = *start++;
		item->tag  = *start++;

		if ((end - start) < item->size)
			return NULL;

		item->data.longdata = start;
		start += item->size;
		return start;
	}

	item->format = HID_ITEM_FORMAT_SHORT;
	item->size = b & 3;

	switch (item->size) {
	case 0:
		return start;

	case 1:
		if ((end - start) < 1)
			return NULL;
		item->data.u8 = *start++;
		return start;

	case 2:
		if ((end - start) < 2)
			return NULL;
		item->data.u16 = get_unaligned_le16(start);
		start = (__u8 *)((__le16 *)start + 1);
		return start;

	case 3:
		item->size++;
		if ((end - start) < 4)
			return NULL;
		item->data.u32 = get_unaligned_le32(start);
		start = (__u8 *)((__le32 *)start + 1);
		return start;
	}

	return NULL;
}

/**
 * hid_parse_report - parse device report
 *
 * @device: hid device
 * @start: report start
 * @size: report size
 *
 * Parse a report description into a hid_device structure. Reports are
 * enumerated, fields are attached to these reports.
 * 0 returned on success, otherwise nonzero error value.
 */
int hid_parse_report(struct hid_device *device, __u8 *start,
		unsigned size)
{
	struct hid_parser *parser;
	struct hid_item item;
	__u8 *end;
	int ret;
	static int (*dispatch_type[])(struct hid_parser *parser,
				      struct hid_item *item) = {
		hid_parser_main,
		hid_parser_global,
		hid_parser_local,
		hid_parser_reserved
	};

	/*
	if (device->driver->report_fixup)
		start = device->driver->report_fixup(device, start, &size);
	*/

	device->rdesc = memdup(start, size);
	if (device->rdesc == NULL)
		return -ENOMEM;
	device->rsize = size;

	parser = calloc(sizeof(struct hid_parser), 1);
	if (!parser) {
		ret = -ENOMEM;
		goto err;
	}

	parser->device = device;

	end = start + size;
	ret = -EINVAL;
	while ((start = fetch_item(start, end, &item)) != NULL) {

		if (item.format != HID_ITEM_FORMAT_SHORT) {
			hid_err(device, "unexpected long global item\n");
			goto err;
		}

		if (dispatch_type[item.type](parser, &item)) {
			hid_err(device, "item %u %u %u %u parsing failed\n",
				item.format, (unsigned)item.size,
				(unsigned)item.type, (unsigned)item.tag);
			goto err;
		}

		if (start == end) {
			if (parser->collection_stack_ptr) {
				hid_err(device, "unbalanced collection at end of report description\n");
				goto err;
			}
			if (parser->local.delimiter_depth) {
				hid_err(device, "unbalanced delimiter at end of report description\n");
				goto err;
			}
			free(parser);
			return 0;
		}
	}

	hid_err(device, "item fetching failed at offset %d\n", (int)(end - start));
err:
	free(parser);
	return ret;
}

/*
 * Convert a signed n-bit integer to signed 32-bit integer. Common
 * cases are done through the compiler, the screwed things has to be
 * done by hand.
 */

static s32 snto32(__u32 value, unsigned n)
{
	switch (n) {
	case 8:  return ((__s8)value);
	case 16: return ((__s16)value);
	case 32: return ((__s32)value);
	}
	return value & (1 << (n - 1)) ? value | (-1 << n) : value;
}

/*
 * Convert a signed 32-bit integer to a signed n-bit integer.
 */

static u32 s32ton(__s32 value, unsigned n)
{
	s32 a = value >> (n - 1);
	if (a && a != -1)
		return value < 0 ? 1 << (n - 1) : (1 << (n - 1)) - 1;
	return value & ((1 << n) - 1);
}

/*
 * Extract/implement a data field from/to a little endian report (bit array).
 *
 * Code sort-of follows HID spec:
 *     http://www.usb.org/developers/devclass_docs/HID1_11.pdf
 *
 * While the USB HID spec allows unlimited length bit fields in "report
 * descriptors", most devices never use more than 16 bits.
 * One model of UPS is claimed to report "LINEV" as a 32-bit field.
 * Search linux-kernel and linux-usb-devel archives for "hid-core extract".
 */

/*
static __u32 extract(const struct hid_device *hid, __u8 *report,
		     unsigned offset, unsigned n)
{
	u64 x;

	if (n > 32)
		hid_warn(hid, "extract() called with n (%d) > 32!\n",
			 n);

	report += offset >> 3;  // adjust byte index
	offset &= 7;            // now only need bit offset into one byte
	x = get_unaligned_le64(report);
	x = (x >> offset) & ((1ULL << n) - 1);  // extract bit field
	return (u32) x;
}
*/

/*
 * "implement" : set bits in a little endian bit stream.
 * Same concepts as "extract" (see comments above).
 * The data mangled in the bit stream remains in little endian
 * order the whole time. It make more sense to talk about
 * endianness of register values by considering a register
 * a "cached" copy of the little endiad bit stream.
 */
static void implement(const struct hid_device *hid, __u8 *report,
		      unsigned offset, unsigned n, __u32 value)
{
	u64 x;
	u64 m = (1ULL << n) - 1;

	if (n > 32)
		hid_warn(hid, "%s() called with n (%d) > 32!\n",
			 __func__, n);

	if (value > m)
		hid_warn(hid, "%s() called with too large value %d!\n",
			 __func__, value);
	/* WARN_ON(value > m); */
	value &= m;

	report += offset >> 3;
	offset &= 7;

	x = get_unaligned_le64(report);
	x &= ~(m << offset);
	x |= ((u64)value) << offset;
	put_unaligned_le64(x, report);
}

/*
 * Search an array for a value.
 */
/*
static int search(__s32 *array, __s32 value, unsigned n)
{
	while (n--) {
		if (*array++ == value)
			return 0;
	}
	return -1;
}
*/

/*
 * Output the field into the report.
 */

static void hid_output_field(const struct hid_device *hid,
			     struct hid_field *field, __u8 *data)
{
	unsigned count = field->report_count;
	unsigned offset = field->report_offset;
	unsigned size = field->report_size;
	unsigned n;

	for (n = 0; n < count; n++) {
		if (field->logical_minimum < 0)	/* signed values */
			implement(hid, data, offset + n * size, size,
				  s32ton(field->value[n], size));
		else				/* unsigned values */
			implement(hid, data, offset + n * size, size,
				  field->value[n]);
	}
}

/*
 * Create a report.
 */

void hid_output_report(struct hid_report *report, __u8 *data)
{
	unsigned n;

	if (report->id > 0)
		*data++ = report->id;

	memset(data, 0, ((report->size - 1) >> 3) + 1);
	for (n = 0; n < report->maxfield; n++)
		hid_output_field(report->device, report->field[n], data);
}

/*
 * Set a field value. The report this field belongs to has to be
 * created and transferred to the device, to set this value in the
 * device.
 */

int hid_set_field(struct hid_field *field, unsigned offset, __s32 value)
{
	unsigned size = field->report_size;

	hid_dump_input(field->report->device, field->usage + offset, value);

	if (offset >= field->report_count) {
		hid_err(field->report->device, "offset (%d) exceeds report_count (%d)\n",
				offset, field->report_count);
		return -1;
	}
	if (field->logical_minimum < 0) {
		if (value != snto32(s32ton(value, size), size)) {
			hid_err(field->report->device, "value %d is out of range\n", value);
			return -1;
		}
	}
	field->value[offset] = value;
	return 0;
}


void *memdup(void *src, size_t size)
{
	void *dest = malloc(size);
	if (dest) {
		memcpy(dest, src, size);
	}
	return dest;
}
 
