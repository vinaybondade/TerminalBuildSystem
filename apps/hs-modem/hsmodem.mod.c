#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x958ac972, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x3ce4ca6f, __VMLINUX_SYMBOL_STR(disable_irq) },
	{ 0x9416e1d8, __VMLINUX_SYMBOL_STR(__request_region) },
	{ 0x697674db, __VMLINUX_SYMBOL_STR(cdev_del) },
	{ 0x12da5bb2, __VMLINUX_SYMBOL_STR(__kmalloc) },
	{ 0x6ceee038, __VMLINUX_SYMBOL_STR(cdev_init) },
	{ 0xff178f6, __VMLINUX_SYMBOL_STR(__aeabi_idivmod) },
	{ 0x38f386ba, __VMLINUX_SYMBOL_STR(single_open) },
	{ 0x85f74b00, __VMLINUX_SYMBOL_STR(iomem_resource) },
	{ 0x97255bdf, __VMLINUX_SYMBOL_STR(strlen) },
	{ 0xb955fd46, __VMLINUX_SYMBOL_STR(single_release) },
	{ 0xf7802486, __VMLINUX_SYMBOL_STR(__aeabi_uidivmod) },
	{ 0x7046a29f, __VMLINUX_SYMBOL_STR(seq_printf) },
	{ 0xb044b9c, __VMLINUX_SYMBOL_STR(device_destroy) },
	{ 0x28cc25db, __VMLINUX_SYMBOL_STR(arm_copy_from_user) },
	{ 0x7485e15e, __VMLINUX_SYMBOL_STR(unregister_chrdev_region) },
	{ 0x91715312, __VMLINUX_SYMBOL_STR(sprintf) },
	{ 0x9f14c3d2, __VMLINUX_SYMBOL_STR(seq_read) },
	{ 0x4295ab47, __VMLINUX_SYMBOL_STR(__platform_driver_register) },
	{ 0xf4fa543b, __VMLINUX_SYMBOL_STR(arm_copy_to_user) },
	{ 0x28d57121, __VMLINUX_SYMBOL_STR(proc_remove) },
	{ 0xfce72eb6, __VMLINUX_SYMBOL_STR(PDE_DATA) },
	{ 0xfa2a45e, __VMLINUX_SYMBOL_STR(__memzero) },
	{ 0x5f754e5a, __VMLINUX_SYMBOL_STR(memset) },
	{ 0x4d42f047, __VMLINUX_SYMBOL_STR(dev_err) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0x20c55ae0, __VMLINUX_SYMBOL_STR(sscanf) },
	{ 0x79c5a9f0, __VMLINUX_SYMBOL_STR(ioremap) },
	{ 0xec62c1b3, __VMLINUX_SYMBOL_STR(hsFifo_done) },
	{ 0xc1733a5c, __VMLINUX_SYMBOL_STR(platform_get_resource) },
	{ 0x5d98390f, __VMLINUX_SYMBOL_STR(device_create) },
	{ 0xd6b8e852, __VMLINUX_SYMBOL_STR(request_threaded_irq) },
	{ 0x2196324, __VMLINUX_SYMBOL_STR(__aeabi_idiv) },
	{ 0x716ddb01, __VMLINUX_SYMBOL_STR(cdev_add) },
	{ 0x2e370e32, __VMLINUX_SYMBOL_STR(_dev_info) },
	{ 0x2ab3cc9d, __VMLINUX_SYMBOL_STR(__release_region) },
	{ 0x17219689, __VMLINUX_SYMBOL_STR(hsFifo_get_user_msg) },
	{ 0xba3cd76e, __VMLINUX_SYMBOL_STR(proc_create_data) },
	{ 0x822137e2, __VMLINUX_SYMBOL_STR(arm_heavy_mb) },
	{ 0xfcec0987, __VMLINUX_SYMBOL_STR(enable_irq) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0xa46f2f1b, __VMLINUX_SYMBOL_STR(kstrtouint) },
	{ 0x9d669763, __VMLINUX_SYMBOL_STR(memcpy) },
	{ 0x65582c2e, __VMLINUX_SYMBOL_STR(sysfs_create_groups) },
	{ 0xedc03953, __VMLINUX_SYMBOL_STR(iounmap) },
	{ 0x588a4a6d, __VMLINUX_SYMBOL_STR(hsFifo_put_user_msg) },
	{ 0x1d0dc4af, __VMLINUX_SYMBOL_STR(sysfs_remove_groups) },
	{ 0x87bbc96a, __VMLINUX_SYMBOL_STR(class_destroy) },
	{ 0xefd6cf06, __VMLINUX_SYMBOL_STR(__aeabi_unwind_cpp_pr0) },
	{ 0xb81960ca, __VMLINUX_SYMBOL_STR(snprintf) },
	{ 0xf8523cf0, __VMLINUX_SYMBOL_STR(platform_driver_unregister) },
	{ 0x80c177bb, __VMLINUX_SYMBOL_STR(of_property_read_variable_u32_array) },
	{ 0x4e45940c, __VMLINUX_SYMBOL_STR(devm_kmalloc) },
	{ 0x69e6e9a5, __VMLINUX_SYMBOL_STR(__class_create) },
	{ 0x29537c9e, __VMLINUX_SYMBOL_STR(alloc_chrdev_region) },
	{ 0xf20dabd8, __VMLINUX_SYMBOL_STR(free_irq) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=hsfifo";

MODULE_ALIAS("of:N*T*Chisky,hs-modem-1.0");
MODULE_ALIAS("of:N*T*Chisky,hs-modem-1.0C*");

MODULE_INFO(srcversion, "08560B405300E2A375FCB1E");
