#include <psp2/kernel/modulemgr.h>
#include <psp2/uvl.h>

#include <stdio.h>
#include <string.h>

#define MAX_LOADED_MODS 256

const char* output_dir = "cache0:/modules";

void do_dump_from_info(int uid, Psp2LoadedModuleInfo *info) {
	char filename[2048] = {0};
	int i;
	FILE *fout;
	for (i = 0; i < 4; ++i) {
		if (info->segments[i].vaddr == NULL) {
			uvl_printf("Segment #%x is empty, skipping\n", i);
			continue;
		}
		snprintf(filename, sizeof(filename), "%s/0x%08x_0x%08x_%s_%d.bin", output_dir, uid, (unsigned)info->segments[i].vaddr, info->module_name, i);
		uvl_printf("Dumping segment #%x to %s\n", i, filename);
		if (!(fout = fopen(filename, "wb"))) {
			uvl_printf("Failed to open the file for writing.\n");
			continue;
		}
		fwrite(info->segments[i].vaddr, info->segments[i].memsz, 1, fout);
		fclose(fout);
	}
}

void dump_module(int uid) {
	int ret, opt = sizeof(opt), reload_mod;
	Psp2LoadedModuleInfo info = {0};
	info.size = sizeof(info);
	if ((ret = sceKernelGetModuleInfo(uid, &info)) < 0) {
		uvl_printf("Failed to get module info for UID %08x, ret %08x\n", uid, ret);
		return;
	}
	uvl_printf("Module: '%s'. Attempting to reload '%s'.\n", info.module_name, info.path);

	// taken from UVL
	if (strncmp(info.path, "ux0:/patch/", 11) == 0) {
		char reload_mod_path[2048] = {0};
		char* mod_filename = strchr(info.path + 11, '/');
		strcpy(reload_mod_path, "app0:");
		strcpy(reload_mod_path + 5, mod_filename);

		uvl_printf("Module path for reloading changed to: %s", reload_mod_path);

		reload_mod = sceKernelLoadModule(reload_mod_path, 0, &opt);
	} else {
		reload_mod = sceKernelLoadModule(info.path, 0, &opt);
	}

	if (reload_mod > 0) {
		uvl_printf("Successfully reloaded the module\n");
		info.size = sizeof(info);
		if ((ret = sceKernelGetModuleInfo(reload_mod, &info)) < 0)
			uvl_printf("Failed to get info for the reloaded module, ret %08x\n");
		else
			do_dump_from_info(reload_mod, &info);
		uvl_printf("Unloading reloaded module\n");
		sceKernelUnloadModule(reload_mod);
	} else {
		uvl_printf("Failed to reload the module, NIDs will remain poisoned\n");
		do_dump_from_info(uid, &info);
	}
}

int main(int argc, char *argv[]) {
	int ret, i;
	int mod_list[MAX_LOADED_MODS];
	int num_loaded = MAX_LOADED_MODS;

	uvl_printf("modump\n");

	if ((ret = sceIoDopen(output_dir)) < 0) {
		ret = sceIoMkdir(output_dir, 0600);
		if (ret == 0) {
			uvl_printf("Created the output directory.\n");
		} else {
			uvl_printf("Failed to create the output directory '%s', ret %08x\n", output_dir, ret);
			goto fail;
		}
	} else {
		sceIoDclose(ret);
	}

	if ((ret = sceKernelGetModuleList(0xFF, mod_list, &num_loaded)) < 0) {
		uvl_printf("Failed to get module list, ret %08x\n", ret);
		goto fail;
	}

	uvl_printf("Got 0x%08x modules, starting the dump\n", num_loaded);
	for (i = 0; i < num_loaded; ++i)
		dump_module(mod_list[i]);

	uvl_printf("All good\n");
	return 0;
fail:
	uvl_printf("Something failed.\n");
	return -1;
}