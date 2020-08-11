# Linux kernel driver for GPE/Lid of Microsoft Surface Devices

 Surface GPE/Lid driver to enable wakeup from suspend via the lid.

 Conventionally, wake-up events for a specific device, e.g. the lid device, are managed via the ACPI `_PRW` field.
 While this does not seem strictly necessary based on ACPI spec, since commit [`941d3e41da7f86bdb9dcc1977c2bcc6b89bfe47 (ACPI: EC / PM: Disable non-wakeup GPEs for suspend-to-idle)`](https://lkml.org/lkml/2018/12/17/224) the kernel disables GPE wakeups to avoid non-wakeup interrupts preventing suspend by default and only enables GPEs associated via the `_PRW` field with a wake-up capable device.

 As there is no `_PRW` field on the lid device of MS Surface devices, we have to enable those GPEs ourselves as a workaround.

## Testing and Installing

Note: You normally do not need to manually install these module if you are already using a kernel from https://github.com/linux-surface/linux-surface.

### Build/Test the module

You can build the module by running `make` inside the `module/` directory.
After that, you can load the module by running `insmod surface_gpe.ko` and remove it by running `rmmod surface_gpe`.

### Permanently install the module

If you want to permanently install the module (or ensure it is loaded during boot), you can run `make dkms-install`.
To uninstall it, run `make dkms-uninstall`.
In case you've installed a patched kernel already contiaining the in-kernel version of this module, dkms should detect this and override the in-kernel module with the externally built one.
This should get reverted by uninstalling the module via the command above.
