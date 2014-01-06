#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
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
	{ 0xd4733cff, "module_layout" },
	{ 0x7e9ebb05, "kernel_thread" },
	{ 0x33d169c9, "_copy_from_user" },
	{ 0x2bf8d577, "sleep_on_timeout" },
	{ 0xd7bd3af2, "add_wait_queue" },
	{ 0xffd5a395, "default_wake_function" },
	{ 0x48eb0c0d, "__init_waitqueue_head" },
	{ 0x96661662, "class_unregister" },
	{ 0x5792fc3, "device_destroy" },
	{ 0x71abefe1, "device_create" },
	{ 0x6bc3fbc0, "__unregister_chrdev" },
	{ 0x93dbbb06, "__class_register" },
	{ 0x112acbc6, "__register_chrdev" },
	{ 0x6fe340df, "kmem_cache_alloc_trace" },
	{ 0x62c1ade7, "malloc_sizes" },
	{ 0x2da418b5, "copy_to_user" },
	{ 0x999e8297, "vfree" },
	{ 0x77b0b954, "put_page" },
	{ 0xd62c833f, "schedule_timeout" },
	{ 0xc37fa30c, "kunmap" },
	{ 0x474d2702, "kmap" },
	{ 0xbc1afedf, "up_write" },
	{ 0xba1c1701, "get_user_pages" },
	{ 0x61b5ade0, "down_write" },
	{ 0x95435001, "current_task" },
	{ 0xd6ee688f, "vmalloc" },
	{ 0xdd71a3fb, "get_task_mm" },
	{ 0xf56cf385, "pid_task" },
	{ 0xa7536cc8, "find_get_pid" },
	{ 0xdc43a9c8, "daemonize" },
	{ 0x37a0cba, "kfree" },
	{ 0x4141f80, "__tracepoint_module_get" },
	{ 0x50eedeb8, "printk" },
	{ 0xebebf1d1, "module_put" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

