#include <system.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <phapi.h>

#include <kernel/io.h>
#include <kernel/shell.h>
#include <kernel/ui.h>
#include <kernel/vga.h>
#include <fs/vfs.h>

char user[20];
char machine[30];
char custom_prompt = 0;
char prompt_msg[256];

void prompt() {
	char cmd[4096];
	while (true) {
		memset(cmd, 0, 4096);
		if (custom_prompt) {
			printk("%s ", prompt_msg);
		} else {
			printk("%s@%s:%s# ", user, machine, get_cwd());
		}
		getsk(cmd);
		if (cmd[0] != 0) {
		    if(shell(cmd)) {
		        printk("Command '%s' not found.\n", cmd);
		    }
		}
	}
}

void welcome() {
    vga_setcolor(COLOR_LIGHT_GREY, COLOR_GREEN);
    clear_vga();

    wstr_color("      |\\     /|(  ____ \\( \\      (  ____ \\(  ___  )(       )(  ____ \\\n", COLOR_RED);
    wstr_color("      | )   ( || (    \\/| (      | (    \\/| (   ) || () () || (    \\/\n", COLOR_RED);
    wstr_color("      | | _ | || (__    | |      | |      | |   | || || || || (\\\n", COLOR_RED);
    wstr_color("      | |( )| ||  __)   | |      | |      | |   | || |(_)| ||  __)\\\n", COLOR_RED);
    wstr_color("      | || || || (      | |      | |      | |   | || |   | || (\\\n", COLOR_RED);
    wstr_color("      | () () || (____/\\| (____/\\| (____/\\| (___) || )   ( || (____/\\\n", COLOR_RED);
    wstr_color("      (_______)(_______/(_______/(_______/(_______)|/     \\|(_______/\n\n", COLOR_RED);

    wstr_color("                           \\__   __/(  ___  )\n", COLOR_YELLOW);
    wstr_color("                              ) (   | (   ) |\n", COLOR_YELLOW);
    wstr_color("                              | |   | |   | |\n", COLOR_YELLOW);
    wstr_color("                              | |   | |   | |\n", COLOR_YELLOW);
    wstr_color("                              | |   | |   | |\n", COLOR_YELLOW);
    wstr_color("                              | |   | (___) |\n", COLOR_YELLOW);
    wstr_color("                              )_(   (_______)\n\n", COLOR_YELLOW);

    wstr_color(" _______           _______ _________ _______  _        _______  _______ \n", COLOR_BLUE);
    wstr_color("(  ____ )|\\     /|(  ___  )\\__   __/(  ___  )( (    /|(  ___  )(  ____ \\\n", COLOR_BLUE);
    wstr_color("| (    )|| )   ( || (   ) |   ) (   | (   ) ||  \\  ( || (   ) || (    \\/\n", COLOR_BLUE);
    wstr_color("| (____)|| (___) || |   | |   | |   | |   | ||   \\ | || |   | || (_____ \n", COLOR_BLUE);
    wstr_color("|  _____)|  ___  || |   | |   | |   | |   | || (\\ \\) || |   | |(_____  )\n", COLOR_BLUE);
    wstr_color("| (      | (   ) || |   | |   | |   | |   | || | \\   || |   | |      ) |\n", COLOR_BLUE);
    wstr_color("| )      | )   ( || (___) |   | |   | (___) || )  \\  || (___) |/\\____) |\n", COLOR_BLUE);
    wstr_color("|/       |/     \\|(_______)   )_(   (_______)|/    )_)(_______)\\_______)\n", COLOR_BLUE);

    printk("                                                        by Feraru Mihail");

    getchark();
    vga_setcolor(COLOR_LIGHT_GREY, COLOR_BLACK);
    clear_vga();
}

extern enum vga_color default_bg;
extern enum vga_color default_fg;

void logo()
{
	int bg = default_bg;
	int fg = default_fg;
    vga_setcolor(COLOR_LIGHT_GREY, COLOR_RED);
    wstr_color(" _______           _______ _________ _______  _        _______  _______          ", COLOR_BLUE);
    wstr_color("(  ____ )|\\     /|(  ___  )\\__   __/(  ___  )( (    /|(  ___  )(  ____ \\         ", COLOR_BLUE);
    wstr_color("| (    )|| )   ( || (   ) |   ) (   | (   ) ||  \\  ( || (   ) || (    \\/         ", COLOR_BLUE);
    wstr_color("| (____)|| (___) || |   | |   | |   | |   | ||   \\ | || |   | || (_____          ", COLOR_BLUE);
    wstr_color("|  _____)|  ___  || |   | |   | |   | |   | || (\\ \\) || |   | |(_____  )         ", COLOR_BLUE);
    wstr_color("| (      | (   ) || |   | |   | |   | |   | || | \\   || |   | |      ) |         ", COLOR_BLUE);
    wstr_color("| )      | )   ( || (___) |   | |   | (___) || )  \\  || (___) |/\\____) |         ", COLOR_BLUE);
    wstr_color("|/       |/     \\|(_______)   )_(   (_______)|/    )_)(_______)\\_______)         ", COLOR_BLUE);
    vga_setcolor(fg, bg);
}

void login()
{
    //logo();
    printk("Log in please.\n");
    printk("Username: ");
    getsk(user);
    printk("Machine: ");
    getsk(machine);
    printk("Loged in!\n");
}
