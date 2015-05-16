/*
 * Toshiba Bluetooth Enable Driver
 *
 * Copyright (C) 2009 Jes Sorensen <Jes.Sorensen@gmail.com>
 *
 * Thanks to Matthew Garrett for background info on ACPI innards which
 * normal people aren't meant to understand :-)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Note the Toshiba Bluetooth RFKill switch seems to be a strange
 * fish. It only provides a BT event when the switch is flipped to
 * the 'on' position. When flipping it to 'off', the USB device is
 * simply pulled away underneath us, without any BT event being
 * delivered.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <acpi/acpi_bus.h>
#include <acpi/acpi_drivers.h>

MODULE_AUTHOR("Jes Sorensen <Jes.Sorensen@gmail.com>");
MODULE_DESCRIPTION("Toshiba Laptop ACPI Bluetooth Enable Driver");
MODULE_LICENSE("GPL");


static int toshiba_bt_rfkill_add(struct acpi_device *device);
static int toshiba_bt_rfkill_remove(struct acpi_device *device, int type);
static void toshiba_bt_rfkill_notify(struct acpi_device *device, u32 event);
static int toshiba_bt_resume(struct acpi_device *device);

static const struct acpi_device_id bt_device_ids[] = {
	{ "TOS6205", 0},
	{ "", 0},
};
MODULE_DEVICE_TABLE(acpi, bt_device_ids);

static struct acpi_driver toshiba_bt_rfkill_driver = {
	.name =		"Toshiba BT",
	.class =	"Toshiba",
	.ids =		bt_device_ids,
	.ops =		{
				.add =		toshiba_bt_rfkill_add,
				.remove =	toshiba_bt_rfkill_remove,
				.notify =	toshiba_bt_rfkill_notify,
				.resume =	toshiba_bt_resume,
			},
	.owner = 	THIS_MODULE,
};


static int toshiba_bluetooth_enable(acpi_handle handle)
{
	acpi_status res1, res2;
	u64 result;

	/*
                                                      
                                                       
             
  */
	res1 = acpi_evaluate_integer(handle, "BTST", NULL, &result);
	if (ACPI_FAILURE(res1))
		return res1;
	if (!(result & 0x01))
		return 0;

	pr_info("Re-enabling Toshiba Bluetooth\n");
	res1 = acpi_evaluate_object(handle, "AUSB", NULL, NULL);
	res2 = acpi_evaluate_object(handle, "BTPO", NULL, NULL);
	if (!ACPI_FAILURE(res1) || !ACPI_FAILURE(res2))
		return 0;

	pr_warn("Failed to re-enable Toshiba Bluetooth\n");

	return -ENODEV;
}

static void toshiba_bt_rfkill_notify(struct acpi_device *device, u32 event)
{
	toshiba_bluetooth_enable(device->handle);
}

static int toshiba_bt_resume(struct acpi_device *device)
{
	return toshiba_bluetooth_enable(device->handle);
}

static int toshiba_bt_rfkill_add(struct acpi_device *device)
{
	acpi_status status;
	u64 bt_present;
	int result = -ENODEV;

	/*
                                                          
                                                             
                                                         
  */
	status = acpi_evaluate_integer(device->handle, "_STA", NULL,
				       &bt_present);

	if (!ACPI_FAILURE(status) && bt_present) {
		pr_info("Detected Toshiba ACPI Bluetooth device - "
			"installing RFKill handler\n");
		result = toshiba_bluetooth_enable(device->handle);
	}

	return result;
}

static int __init toshiba_bt_rfkill_init(void)
{
	int result;

	result = acpi_bus_register_driver(&toshiba_bt_rfkill_driver);
	if (result < 0) {
		ACPI_DEBUG_PRINT((ACPI_DB_ERROR,
				  "Error registering driver\n"));
		return result;
	}

	return 0;
}

static int toshiba_bt_rfkill_remove(struct acpi_device *device, int type)
{
	/*          */
	return 0;
}

static void __exit toshiba_bt_rfkill_exit(void)
{
	acpi_bus_unregister_driver(&toshiba_bt_rfkill_driver);
}

module_init(toshiba_bt_rfkill_init);
module_exit(toshiba_bt_rfkill_exit);