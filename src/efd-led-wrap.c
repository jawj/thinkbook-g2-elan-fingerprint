/*
 * LED shim for the proprietary ELAN TOD plugin (libfprint-2-tod1-elan.so).
 *
 * The plugin never turns on the sensor's LED. The open-source elan driver
 * sends LED-on (0x40 0x31, no response) before each wait-for-finger
 * (0x40 0x3f); its stop command (0x00 0x0b), which the plugin does send,
 * turns the LED off again. The plugin's USB submit calls are renamed with
 * patchelf to the efdwrap_* functions below, which inject LED-on ahead of
 * each wait-for-finger and otherwise pass everything through unchanged.
 */
#include <string.h>
#include <gusb.h>
#include <libusb.h>

#define EP_CMD_OUT 0x01

static const guint8 wait_finger_cmd[] = { 0x40, 0x3f };
static guint8 led_on_cmd[] = { 0x40, 0x31 };

static gboolean
is_wait_finger (guint8 endpoint, const guint8 *data, gsize length)
{
	return endpoint == EP_CMD_OUT && length == sizeof (wait_finger_cmd) &&
	       memcmp (data, wait_finger_cmd, sizeof (wait_finger_cmd)) == 0;
}

void
efdwrap_g_usb_device_bulk_transfer_async (GUsbDevice *device, guint8 endpoint,
					  guint8 *data, gsize length, guint timeout,
					  GCancellable *cancellable,
					  GAsyncReadyCallback callback, gpointer user_data)
{
	if (is_wait_finger (endpoint, data, length)) {
		gsize actual;

		/* Best effort: never let the LED affect authentication */
		g_usb_device_bulk_transfer (device, EP_CMD_OUT, led_on_cmd, sizeof (led_on_cmd),
					    &actual, 200, NULL, NULL);
	}
	g_usb_device_bulk_transfer_async (device, endpoint, data, length, timeout,
					  cancellable, callback, user_data);
}

int
efdwrap_libusb_submit_transfer (struct libusb_transfer *transfer)
{
	if (transfer->type == LIBUSB_TRANSFER_TYPE_BULK &&
	    is_wait_finger (transfer->endpoint, transfer->buffer, transfer->length)) {
		int actual;

		libusb_bulk_transfer (transfer->dev_handle, EP_CMD_OUT, led_on_cmd,
				      sizeof (led_on_cmd), &actual, 200);
	}
	return libusb_submit_transfer (transfer);
}
