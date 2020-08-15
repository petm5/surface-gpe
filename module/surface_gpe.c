// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Surface GPE/Lid driver to enable wakeup from suspend via the lid.
 *
 * Conventionally, wake-up events for a specific device, e.g. the lid device,
 * are managed via the ACPI _PRW field. While this does not seem strictly
 * necessary based on ACPI spec, the kernel disables GPE wakeups to avoid
 * non-wakeup interrupts preventing suspend by default and only enables GPEs
 * associated via the _PRW field with a wake-up capable device.
 *
 * As there is no _PRW field on the lid device of MS Surface devices, we have
 * to enable those GPEs ourselves as a workaround.
 *
 * Link: https://lkml.org/lkml/2018/12/17/224
 *
 * Microsoft Surface devices
 */

#include <linux/acpi.h>
#include <linux/dmi.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>


struct surface_lid_device {
	const char *acpi_path;
	const u32 gpe_number;
};


static const struct surface_lid_device lid_device_l17 = {
	.acpi_path = "\\_SB.LID0",
	.gpe_number = 0x17,
};

static const struct surface_lid_device lid_device_l4D = {
	.acpi_path = "\\_SB.LID0",
	.gpe_number = 0x4D,
};

static const struct surface_lid_device lid_device_l4F = {
	.acpi_path = "\\_SB.LID0",
	.gpe_number = 0x4F,
};

static const struct surface_lid_device lid_device_l57 = {
	.acpi_path = "\\_SB.LID0",
	.gpe_number = 0x57,
};


// Note: When changing this don't forget to change the MODULE_ALIAS below.
static const struct dmi_system_id dmi_lid_device_table[] = {
	{
		.ident = "Surface Pro 4",
		.matches = {
			DMI_EXACT_MATCH(DMI_SYS_VENDOR, "Microsoft Corporation"),
			DMI_EXACT_MATCH(DMI_PRODUCT_NAME, "Surface Pro 4"),
		},
		.driver_data = (void *)&lid_device_l17,
	},
	{
		.ident = "Surface Pro 5",
		.matches = {
			/*
			 * We match for SKU here due to generic product name
			 * "Surface Pro".
			 */
			DMI_EXACT_MATCH(DMI_SYS_VENDOR, "Microsoft Corporation"),
			DMI_EXACT_MATCH(DMI_PRODUCT_SKU, "Surface_Pro_1796"),
		},
		.driver_data = (void *)&lid_device_l4F,
	},
	{
		.ident = "Surface Pro 5 (LTE)",
		.matches = {
			/*
			 * We match for SKU here due to generic product name
			 * "Surface Pro"
			 */
			DMI_EXACT_MATCH(DMI_SYS_VENDOR, "Microsoft Corporation"),
			DMI_EXACT_MATCH(DMI_PRODUCT_SKU, "Surface_Pro_1807"),
		},
		.driver_data = (void *)&lid_device_l4F,
	},
	{
		.ident = "Surface Pro 6",
		.matches = {
			DMI_EXACT_MATCH(DMI_SYS_VENDOR, "Microsoft Corporation"),
			DMI_EXACT_MATCH(DMI_PRODUCT_NAME, "Surface Pro 6"),
		},
		.driver_data = (void *)&lid_device_l4F,
	},
	{
		.ident = "Surface Pro 7",
		.matches = {
			DMI_EXACT_MATCH(DMI_SYS_VENDOR, "Microsoft Corporation"),
			DMI_EXACT_MATCH(DMI_PRODUCT_NAME, "Surface Pro 7"),
		},
		.driver_data = (void *)&lid_device_l4D,
	},
	{
		.ident = "Surface Book 1",
		.matches = {
			DMI_EXACT_MATCH(DMI_SYS_VENDOR, "Microsoft Corporation"),
			DMI_EXACT_MATCH(DMI_PRODUCT_NAME, "Surface Book"),
		},
		.driver_data = (void *)&lid_device_l17,
	},
	{
		.ident = "Surface Book 2",
		.matches = {
			DMI_EXACT_MATCH(DMI_SYS_VENDOR, "Microsoft Corporation"),
			DMI_EXACT_MATCH(DMI_PRODUCT_NAME, "Surface Book 2"),
		},
		.driver_data = (void *)&lid_device_l17,
	},
	{
		.ident = "Surface Book 3",
		.matches = {
			DMI_EXACT_MATCH(DMI_SYS_VENDOR, "Microsoft Corporation"),
			DMI_EXACT_MATCH(DMI_PRODUCT_NAME, "Surface Book 3"),
		},
		.driver_data = (void *)&lid_device_l4D,
	},
	{
		.ident = "Surface Laptop 1",
		.matches = {
			DMI_EXACT_MATCH(DMI_SYS_VENDOR, "Microsoft Corporation"),
			DMI_EXACT_MATCH(DMI_PRODUCT_NAME, "Surface Laptop"),
		},
		.driver_data = (void *)&lid_device_l57,
	},
	{
		.ident = "Surface Laptop 2",
		.matches = {
			DMI_EXACT_MATCH(DMI_SYS_VENDOR, "Microsoft Corporation"),
			DMI_EXACT_MATCH(DMI_PRODUCT_NAME, "Surface Laptop 2"),
		},
		.driver_data = (void *)&lid_device_l57,
	},
	{
		.ident = "Surface Laptop 3 (Intel 13\")",
		.matches = {
			/*
			 * We match for SKU here due to different vairants: The
			 * AMD (15") version does not rely on GPEs.
			 */
			DMI_EXACT_MATCH(DMI_SYS_VENDOR, "Microsoft Corporation"),
			DMI_EXACT_MATCH(DMI_PRODUCT_SKU, "Surface_Laptop_3_1867:1868"),
		},
		.driver_data = (void *)&lid_device_l4D,
	},
	{ }
};


static int surface_lid_enable_wakeup(const struct surface_lid_device *dev,
				     bool enable)
{
	int action = enable ? ACPI_GPE_ENABLE : ACPI_GPE_DISABLE;
	int status;

	status = acpi_set_gpe_wake_mask(NULL, dev->gpe_number, action);
	if (status)
		return -EFAULT;

	return 0;
}


static int surface_gpe_suspend(struct device *dev)
{
	const struct surface_lid_device *ldev;

	ldev = dev_get_drvdata(dev);
	return surface_lid_enable_wakeup(ldev, true);
}

static int surface_gpe_resume(struct device *dev)
{
	const struct surface_lid_device *ldev;

	ldev = dev_get_drvdata(dev);
	return surface_lid_enable_wakeup(ldev, false);
}

static SIMPLE_DEV_PM_OPS(surface_gpe_pm, surface_gpe_suspend, surface_gpe_resume);


static int surface_gpe_probe(struct platform_device *pdev)
{
	struct surface_lid_device *dev = pdev->dev.platform_data;
	acpi_handle lid_handle;
	int status;

	if (!dev)
		return -ENODEV;

	status = acpi_get_handle(NULL, (acpi_string)dev->acpi_path, &lid_handle);
	if (status)
		return -EFAULT;

	status = acpi_mark_gpe_for_wake(NULL, dev->gpe_number);
	if (status)
		return -EFAULT;

	status = acpi_enable_gpe(NULL, dev->gpe_number);
	if (status)
		return -EFAULT;

	status = surface_lid_enable_wakeup(dev, false);
	if (status) {
		acpi_disable_gpe(NULL, dev->gpe_number);
		return status;
	}

	platform_set_drvdata(pdev, dev);
	return 0;
}

static int surface_gpe_remove(struct platform_device *pdev)
{
	struct surface_lid_device *dev = platform_get_drvdata(pdev);

	/* restore default behavior without this module */
	surface_lid_enable_wakeup(dev, false);
	acpi_disable_gpe(NULL, dev->gpe_number);

	platform_set_drvdata(pdev, NULL);
	return 0;
}

static struct platform_driver surface_gpe = {
	.probe = surface_gpe_probe,
	.remove = surface_gpe_remove,
	.driver = {
		.name = "surface_gpe",
		.pm = &surface_gpe_pm,
		.probe_type = PROBE_PREFER_ASYNCHRONOUS,
	},
};


static struct platform_device *surface_gpe_device;

static int __init surface_gpe_init(void)
{
	const struct dmi_system_id *match;
	const struct surface_lid_device *lid;

	struct platform_device *pdev;
	int status;

	surface_gpe_device = NULL;

	match = dmi_first_match(dmi_lid_device_table);
	if (!match)
		return 0;

	lid = match->driver_data;

	status = platform_driver_register(&surface_gpe);
	if (status)
		return status;

	pdev = platform_device_alloc("surface_gpe", PLATFORM_DEVID_NONE);
	if (!pdev) {
		platform_driver_unregister(&surface_gpe);
		return -ENOMEM;
	}

	status = platform_device_add_data(pdev, lid, sizeof(*lid));
	if (status) {
		platform_device_put(pdev);
		platform_driver_unregister(&surface_gpe);
		return status;
	}

	status = platform_device_add(pdev);
	if (status) {
		platform_device_put(pdev);
		platform_driver_unregister(&surface_gpe);
		return status;
	}

	surface_gpe_device = pdev;
	return 0;
}

static void __exit surface_gpe_exit(void)
{
	if (!surface_gpe_device)
		return;

	platform_device_unregister(surface_gpe_device);
	platform_driver_unregister(&surface_gpe);
}

module_init(surface_gpe_init);
module_exit(surface_gpe_exit);

MODULE_AUTHOR("Maximilian Luz <luzmaximilian@gmail.com>");
MODULE_DESCRIPTION("Surface GPE/Lid Driver");
MODULE_VERSION("0.1");
MODULE_LICENSE("GPL");

MODULE_ALIAS("dmi:*:svnMicrosoftCorporation:pnSurfacePro:*");
MODULE_ALIAS("dmi:*:svnMicrosoftCorporation:pnSurfacePro4:*");
MODULE_ALIAS("dmi:*:svnMicrosoftCorporation:pnSurfacePro6:*");
MODULE_ALIAS("dmi:*:svnMicrosoftCorporation:pnSurfacePro7:*");
MODULE_ALIAS("dmi:*:svnMicrosoftCorporation:pnSurfaceBook:*");
MODULE_ALIAS("dmi:*:svnMicrosoftCorporation:pnSurfaceBook2:*");
MODULE_ALIAS("dmi:*:svnMicrosoftCorporation:pnSurfaceBook3:*");
MODULE_ALIAS("dmi:*:svnMicrosoftCorporation:pnSurfaceLaptop:*");
MODULE_ALIAS("dmi:*:svnMicrosoftCorporation:pnSurfaceLaptop2:*");
MODULE_ALIAS("dmi:*:svnMicrosoftCorporation:pnSurfaceLaptop3:*");