#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/limits.h>
#include <dirent.h>
#include <linux/input.h>

#include "common.h"
#include "recovery_lib.h"
#include "minui/minui.h"
#include "recovery_ui.h"
#include "roots.h"
#include "firmware.h"
#include "install.h"
#include <string.h>

#include "nandroid_menu.h"

void append(char* s, char c) //Helper function - used to append a character to a string
{
        int len = strlen(s);
        s[len] = c;
        s[len+1] = '\0';
}

char *replace_str(char *str, char *orig, char *rep) // Helper function - search and replace within in string
{
  static char buffer[4096];
  char *p;

  if(!(p = strstr(str, orig)))  // Is 'orig' even in 'str'?
    return str;

  strncpy(buffer, str, p-str); // Copy characters from 'str' start to 'orig' st$
  buffer[p-str] = '\0';

  sprintf(buffer+(p-str), "%s%s", rep, p+strlen(orig));

  return buffer;
}

void choose_file_menu(char* sdpath, char* ext1, char *ext2, char* ext3, char* ext4, char* ext5)
{
	int ext1_l = strlen(ext1);
	int ext2_l = strlen(ext2);
	int ext3_l = strlen(ext3);
	int ext4_l = strlen(ext4);
	int ext5_l = strlen(ext5);
	
    static char* headers[] = { "Choose item or press POWER to return",
			       "",
			       NULL };
    
    char path[PATH_MAX] = "";
    DIR *dir;
    struct dirent *de;
    int total = 0;
    int i;
    char** files;
    char** list;

    if (ensure_root_path_mounted("SDCARD:") != 0) {
	LOGE ("Can't mount /sdcard\n");
	return;
    }

    dir = opendir(sdpath);
    if (dir == NULL) {
		LOGE("Couldn't open directory");
		LOGE("Please make sure it exists!");
		return;
    }
    
    while ((de=readdir(dir)) != NULL) {
	if ((de->d_name[0] == '.' && de->d_name[1] == '.') || (opendir(de->d_name) != NULL) || (strcmp(de->d_name+strlen(de->d_name)-ext1_l,ext1)==0 || strcmp(de->d_name+strlen(de->d_name)-ext2_l,ext2)==0) || strcmp(de->d_name+strlen(de->d_name)-ext3_l,ext3)==0 || strcmp(de->d_name+strlen(de->d_name)-ext4_l,ext4)==0 || strcmp(de->d_name+strlen(de->d_name)-ext5_l,ext5)==0) {
			total++;
		}
	}
	

    if (total==0) {
		LOGE("No files found!\n");
		if(closedir(dir) < 0) {
			LOGE("Failed to close directory\n");
	    return;
		}
    }
    else {
		files = (char**) malloc((total+1)*sizeof(char*));
		files[total]=NULL;

		list = (char**) malloc((total+1)*sizeof(char*));
		list[total]=NULL;
	
		rewinddir(dir);

		i = 0;
		while ((de = readdir(dir)) != NULL) {
			if ((de->d_name[0] == '.' && de->d_name[1] == '.') || (opendir(de->d_name) != NULL) || (strcmp(de->d_name+strlen(de->d_name)-ext1_l,ext1)==0 || strcmp(de->d_name+strlen(de->d_name)-ext2_l,ext2)==0) || strcmp(de->d_name+strlen(de->d_name)-ext3_l,ext3)==0 || strcmp(de->d_name+strlen(de->d_name)-ext4_l,ext4)==0 || strcmp(de->d_name+strlen(de->d_name)-ext5_l,ext5)==0) {
				files[i] = (char*) malloc(strlen(sdpath)+strlen(de->d_name)+1);
				strcpy(files[i], sdpath);
				strcat(files[i], de->d_name);
				list[i] = (char*) malloc(strlen(de->d_name)+1);
				if (opendir(de->d_name) != NULL) { //if is a dir, add / to it
					append(de->d_name, '/');
				}
				strcpy(list[i], de->d_name);				
					i++;				
			}
		}	

		if (closedir(dir) <0) {
			LOGE("Failure closing directory\n");
			return;
		}

		int chosen_item = -1;
		while (chosen_item < 0) {
			char* folder;
			chosen_item = get_menu_selection(headers, list, 1, chosen_item<0?0:chosen_item);
			if (chosen_item >= 0 && chosen_item != ITEM_BACK ) {
				
				if (opendir(files[chosen_item]) == NULL) {
					preinstall_menu(files[chosen_item]);
				} 
				if (opendir(files[chosen_item]) != NULL) {
					folder = files[chosen_item];
					append(folder, '/'); // add forward slash to string	
					choose_file_menu(folder, ".zip", ".tar", ".tgz", "boot.img", "rec.img");
				} 
			} 
		} 
	}
}


int install_img(char* filename, char* partition) {

    ui_print("\n-- Flash image from...\n");
	ui_print(filename);
	ui_print("\n");
	ensure_root_path_mounted("SDCARD:");	
	
    char* argv[] = { "/sbin/flash_image",
		     partition,
		     filename,
		     NULL };

    char* envp[] = { NULL };

    int status = runve("/sbin/flash_image",argv,envp,1);
  
	ui_print("\nFlash from sdcard complete.");
	ui_print("\nThanks for using RZrecovery.\n");
	return 0;
}

void install_update_package(char* filename) {
	char* extension = (filename+strlen(filename)-3);
	char* boot_extension = (filename+strlen(filename)-8);
	char* rec_extension = (filename+strlen(filename)-7);
	if (strcmp(extension, "zip") == 0 ) {
		ui_print("\nZIP detected.\n");
		install_update_zip(filename);
	} 
	if (strcmp(extension, "tar") == 0 || strcmp(extension, "tgz") == 0 ) { 
		ui_print("\nTAR detected.\n");
		install_rom_from_tar(filename);
	}
	if (strcmp(rec_extension, "rec.img") == 0 ) {
		ui_print("\nRECOVERY IMAGE detected.\n");
		install_img(filename, "recovery");
	}
	if (strcmp(boot_extension, "boot.img") == 0 ) {
		ui_print("\nBOOT IMAGE detected.\n");
		install_img(filename, "boot");
	}
	return;
}
	
int install_update_zip(char* filename) {

	char *path = NULL;

	puts(filename);
	path = replace_str(filename, "/sdcard/", "SDCARD:");
	ui_print("\n-- Install update.zip from sdcard...\n");
	set_sdcard_update_bootloader_message();
	ui_print("Attempting update from...\n");
	ui_print(filename);
	ui_print("\n");
	int status = install_package(path);
	if (status != INSTALL_SUCCESS) {
		ui_set_background(BACKGROUND_ICON_ERROR);
		ui_print("Installation aborted.\n");
	} else if (!ui_text_visible()) {
		return 0;  // reboot if logs aren't visible
	} else {
	if (firmware_update_pending()) {
	    ui_print("\nReboot via menu to complete\ninstallation.\n");
	} else {
	    ui_print("\nInstall from sdcard complete.\n");
		ui_print("\nThanks for using RZrecovery.\n");
		}
	}   
	return 0;
}

int install_rom_from_tar(char* filename)
{
    ui_print("Attempting to install ROM from ");
    ui_print(filename);
    ui_print("...\n");
  
    char* argv[] = { "/sbin/nandroid-mobile.sh",
		     "--install-rom",
		     filename,
		     "--progress",
		     NULL };

    char* envp[] = { NULL };
  
    int status = runve("/sbin/nandroid-mobile.sh",argv,envp,1);
    if(!WIFEXITED(status) || WEXITSTATUS(status)!=0) {
		ui_printf_int("ERROR: install exited with status %d\n",WEXITSTATUS(status));
		return WEXITSTATUS(status);
    } else {
		ui_print("(done)\n");
    }
		ui_reset_progress();
		return 0;
}

void preinstall_menu(char* filename) {

    char* headers[] = { "Preinstall Menu",
			"Choose options and select filename to install.",
			" ",
			NULL };
  
    char* items[] = { "Abort Install",
			  "Preinstall Backup",
			  "Wipe /data",
			  filename,
		      NULL };
#define ITEM_NO 		0
#define ITEM_BACKUP 	1
#define ITEM_WIPE 		2
#define ITEM_INSTALL 	3

		int chosen_item = -1;
		while (chosen_item != ITEM_BACK) {
		chosen_item = get_menu_selection(headers,items,1,chosen_item<0?0:chosen_item);
		switch(chosen_item) {
			case ITEM_NO:
			chosen_item = ITEM_BACK;
			return;
		case ITEM_BACKUP:
			ui_print("Backing up before installing...\n");
			nandroid_backup("preinstall",BSD|PROGRESS);
			break;
		case ITEM_WIPE:
			wipe_partition(ui_text_visible(), "Are you sure?", "Yes - wipe DATA", "data");
			break;
		case ITEM_INSTALL:
			install_update_package(filename);		
			break;
		}
	}
}
