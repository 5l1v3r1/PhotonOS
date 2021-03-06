/* Basic input/output
 *
 * Copyright (c) 2015 Feraru Mihail (mihailferaru2000@gmail.com).
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include <system.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <phapi.h>

#include <i386/handlers.h>
#include <kernel/vga.h>
#include <kernel/heap.h>
#include <kernel/io.h>
#include <kernel/process.h>
#include <drivers/keyboard.h>
#include <fs/vfs.h>
#include <fs/fcntl.h>

extern struct file_operations fops;
extern struct file **opened_files;
extern int file_table_size;
extern char kernel_space_open;
extern process_t *current_process;
extern char cwd[4096];
extern struct dentry *cwd_de;

/* output byte */
void outb(size_t ad, uint8_t v)
{
    asm volatile("outb %%al, %%dx" :: "d" (ad), "a" (v));;
}
/* output word */
void outw(size_t ad, uint16_t v)
{
    asm volatile("outw %%ax, %%dx" :: "d" (ad), "a" (v));
}
/* output word */
void outl(size_t ad, size_t v)
{
    asm volatile("outl %%eax, %%dx" : : "d" (ad), "a" (v));
}
/* input byte */
uint8_t inb(size_t ad)
{
    uint8_t _v;
    asm volatile("inb %%dx, %%al" : "=a" (_v) : "d" (ad));
    return _v;
}
/* input word */
uint16_t inw(size_t ad)
{
    uint16_t _v;
    asm volatile("inw %%dx, %%ax" : "=a" (_v) : "d" (ad));
    return _v;
}
/* input word */
size_t inl(size_t ad)
{
    size_t _v;
    asm volatile("inl %%dx, %%eax" : "=a" (_v) : "d" (ad));
    return _v;
}

 
void init_serial() {
   outb(COM_PORT + 1, 0x00);    // Disable all interrupts
   outb(COM_PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
   outb(COM_PORT + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
   outb(COM_PORT + 1, 0x00);    //                  (hi byte)
   outb(COM_PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
   outb(COM_PORT + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
   outb(COM_PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

int serial_received() {
   return inb(COM_PORT + 5) & 1;
}

char serial_read_char() {
   while (serial_received() == 0);

   return inb(COM_PORT);
}

int serial_read(char *buf, size_t len)
{
    for (size_t i = 0; i < len; i++)
        buf[i] = serial_read_char();
    return 0;
}
int is_transmit_empty() {
   return inb(COM_PORT + 5) & 0x20;
}

int serial_write_char(char a) {
   while (is_transmit_empty() == 0);

   outb(COM_PORT,a);
   return 0;
}

int serial_write(const char *buf, size_t len)
{
    for ( size_t i = 0; i < len; i++ )
        serial_write_char((char) ((const char*) buf)[i]);
    return 0;
}


void print_regs(registers_t *regs)
{
    printk("\n\rPushed by CPU:\n\r");
    printk("ss: %x uesp: %x eflags: %x cs: %x eip: %x\n\r",
        regs->ss,
        regs->uesp,
        regs->eflags,
        regs->cs,
        regs->eip
    );
    printk("int_no: %x err_code: %x\n\r", regs->int_no, regs->err_code);
    printk("Pushed by pusha:\n\r");
    printk("eax: %x ecx: %x edx: %x ebx: %x kesp: %x\n\r",
        regs->eax,
        regs->ecx,
        regs->edx,
        regs->ebx,
        regs->kesp
    );
    printk("ebp: %x esi: %x edi: %x\n\r",
        regs->ebp,
        regs->esi,
        regs->edi
    );
    printk("Others:\n\r");
    printk("es: %x ds: %x fs: %x gs: %x\n\r",
        regs->es,
        regs->ds,
        regs->fs,
        regs->gs
    );
    printk("cr2: %x cr3: %x\n\r",
        regs->cr2,
        regs->cr3
    );
}

// TTY should be separated into another module
// print and read functions should be separated too
// I/O should contain only port I/Os, but I'm too lazy 

// Generic functions for standard I/O
// A very bad choice to do this directly :(

// When reading from keyboard, the 'ENTER' key returns the character '\n'
// When reading from the serial port, it returns the character '\r'
// When reading over the network is again '\r' what da fac
// Same problem for other characters like backspace

#if defined(_TEXTMODE)
#define DEL '\b'
#elif defined(_SERIALMODE)
#define DEL '\x7f'
#endif

#define LF '\n'
#define CR '\r'
#define BACK '\x08'


int tty_write_char(const char c) {
#if defined(_TEXTMODE)
    return vga_write_char(c); 
#elif defined(_SERIALMODE)
    // handle special terminal characters
    if (c == DEL) {    
        serial_write_char(BACK);
        serial_write_char(' ');
        serial_write_char(BACK);
        return 0;
    }

    return serial_write_char(c);
#else
#error "No terminal mode set."
#endif
}

int tty_write(const char *buf, size_t len) {
#if defined(_TEXTMODE)
    return vga_write(buf, len);
#elif defined(_SERIALMODE)
    return serial_write(buf, len);
#else
#error "No terminal mode set."
#endif
}

char tty_read_char() {
#if defined(_TEXTMODE)
    return kb_read_char(); 
#elif defined(_SERIALMODE)
    return serial_read_char();
#else
#error "No terminal mode set."
#endif
}

int tty_read(char *buf, size_t len) {
#if defined(_TEXTMODE)
    return kb_read(buf, len);
#elif defined(_SERIALMODE)
    return serial_read(buf, len);
#else
#error "No terminal mode set."
#endif
}

#if defined(_SERIALMODE) && defined(_TEXTMODE)
#error "You can't set standard I/O to both serial and VGA text mode."
#endif

int vga_write_char(const char c)
{
    vga_putchar(c);
    return 0;
}

char kb_read_char()
{
    return kb_getchar();
}

int vga_write(const char *buf, size_t len)
{
    for ( size_t i = 0; i < len; i++ )
        vga_write_char((int) ((const char*) buf)[i]);
    return 0;
}

int kb_read(char *buf, size_t len)
{
    for (size_t i = 0; i < len; i++)
        buf[i] = kb_read_char();
    return 0;
}

int getchark()
{
    int c = tty_read_char();
    tty_write_char(c);

    return c;
}

char *getsk(char *str)
{
    int c = tty_read_char();
    int i = 0;
    while (c != LF && c != CR) {
        if (c != DEL) {
            str[i++] = c;
            tty_write_char(c);
        } else if (c == DEL && i > 0) {
            str[--i] = 0;
            tty_write_char(DEL);
        }
        c = tty_read_char();
    }
    str[i] = '\0';
    tty_write_char('\n');
    return str;
}

int putsk(const char* string)
{
    int i = 0;
    for (i = 0; string[i] != '\0'; i++) {
        tty_write_char(string[i]);
    }
    tty_write_char('\n');
    return i;
}

int printk(const char* format, ...)
{
    va_list parameters;
    va_start(parameters, format);

    int written = 0;
    size_t amount;
    bool rejected_bad_specifier = false;

    while ( *format != '\0' ) {
        if ( *format != '%' ) {
        print_c:
            amount = 1;
            while ( format[amount] && format[amount] != '%' )
                amount++;
            tty_write(format, amount);
            format += amount;
            written += amount;
            continue;
        }

        const char* format_begun_at = format;

        if ( *(++format) == '%' )
            goto print_c;

        if ( rejected_bad_specifier ) {
        incomprehensible_conversion:
            rejected_bad_specifier = true;
            format = format_begun_at;
            goto print_c;
        }
        if ( *format == 'c' ) {
            format++;
            char c = (char) va_arg(parameters, int);
            tty_write(&c, sizeof(c));
        } else if ( *format == 's' ) {
            format++;
            const char* s = va_arg(parameters, const char*);
            tty_write(s, strlen(s));
        } else if ( *format == 'd' ) {
            format++;
            int n = va_arg(parameters, int);
            if (n) {
                char s[intlen(n, 10)];
                itoa(s, n, 10);
                tty_write(s, strlen(s));
            } else {
                tty_write_char('0');
            }
        } else if ( *format == 'x') {
            format++;
            int n = va_arg(parameters, int);
            if (n) {
                char s[intlen(n, 16)];
                itoa(s, n, 16);
                tty_write(s, strlen(s));
            } else {
                printk("0x0");
            }
        } else {
            goto incomprehensible_conversion;
        }
    }

    va_end(parameters);
    return written;
}

// Kernel space file functions
int kcreate(const char *pathname, int flags)
{
    char path[4096];
    memset(path, 0, 4096);
    strcpy(path, pathname);

    if (path[0] != '/') {
        relative_to_absolute(path);
    }
    path_tokens *tokens = tokenize_path(path);
    tokens->n -= 1;

    struct dentry *de = get_dentry_by_tokens(tokens);
    if (de == NULL) {
        return -1;
    }
    inode_t *inode = de->inode;
    if (inode->flags != FS_DIRECTORY) {
        return -1;
    }
    if (inode->create == NULL ) {
        return -1;
    }

    for (int i = strlen(path) - 1; i >= 0; i--) {
        if (path[i] == '/' && i == 0) {
            path[1] = '\0';
            break;
        } else if (path[i] == '/') {
            path[i] = '\0';
            break;
        }
    }

    char save_cwd[4096];
    memcpy(save_cwd, cwd, strlen(cwd) + 1);

    memcpy(cwd, path, strlen(path) + 1);
    cwd_de = get_dentry_by_path(cwd);

    int ret = inode->create(inode, tokens->tokens[tokens->n], flags);
    destroy_tokens(tokens);

    memcpy(cwd, save_cwd, strlen(save_cwd) + 1);
    cwd_de = get_dentry_by_path(cwd);

    return ret;
}

int kmkdir(const char *pathname)
{
    char path[4096];
    memset(path, 0, 4096);
    strcpy(path, pathname);

    if (path[0] != '/') {
        relative_to_absolute(path);
    }
    path_tokens *tokens = tokenize_path(path);
    tokens->n -= 1;

    struct dentry *de = get_dentry_by_tokens(tokens);
    if (de == NULL) {
        return -1;
    }
    inode_t *inode = de->inode;
    if (inode->flags != FS_DIRECTORY) {
        return -1;
    }
    if (inode->create == NULL ) {
        return -1;
    }

    for (int i = strlen(path) - 1; i >= 0; i--) {
        if (path[i] == '/' && i == 0) {
            path[1] = '\0';
            break;
        } else if (path[i] == '/') {
            path[i] = '\0';
            break;
        }
    }

    char save_cwd[4096];
    memcpy(save_cwd, cwd, strlen(cwd) + 1);

    memcpy(cwd, path, strlen(path) + 1);
    cwd_de = get_dentry_by_path(cwd);

    int ret = inode->mkdir(inode, tokens->tokens[tokens->n]);
    destroy_tokens(tokens);

    memcpy(cwd, save_cwd, strlen(save_cwd) + 1);
    cwd_de = get_dentry_by_path(cwd);

    return ret;
}

int kopen(const char *pathname, int flags)
{
    kernel_space_open = 1;

    file *f = kmalloc(sizeof(file), 0, 0);
    f->f_dentry = get_dentry_by_path(pathname);

    if (f->f_dentry == NULL) {
        if (flags == O_WRONLY || flags == O_RDWR) {
            int ret = kcreate(pathname, FS_FILE);
            if (ret == -1) {
                kfree(f);
                return -1;
            }
            f->f_dentry = get_dentry_by_path(pathname);
        } else {
            kfree(f);
            return -1;
        }
    }

    f->f_op = &fops;
    f->f_mode = (mode_t) flags;
    f->f_pos = 0;
    f->f_uid = 0;
    f->f_gid = 0;
    f->f_version = 0;
    f->f_dentry->inode->open_flags = flags;
    return f->f_op->open(f->f_dentry->inode, f);
}

int kwrite(int fd, char *buf, size_t count)
{
    if (fd >= file_table_size || buf == NULL) {
        return -1;
    }
    file *f = opened_files[fd];

    if (f == NULL) {
        return -1;
    }
    if (f->f_mode != O_WRONLY && f->f_mode != O_RDWR && f->f_mode != O_APPEND) {
        return -1;
    }

    loff_t off = (loff_t) f->f_dentry->inode->offset;
    if (f->f_mode == O_APPEND) {
        f->f_dentry->inode->offset = f->f_dentry->inode->length;
        off = (loff_t) f->f_dentry->inode->offset;
    }
    return f->f_op->write(f, buf, count, &off);
}

size_t kread(int fd, void *buf, size_t count)
{
    if (fd >= file_table_size || buf == NULL) {
        return -1;
    }

    file *f = opened_files[fd];

    if (f == NULL) {
        return -1;
    }
    if (f->f_mode != O_RDONLY && f->f_mode != O_RDWR) {
        return -1;
    }

    loff_t off = (loff_t) f->f_dentry->inode->offset;
    return f->f_op->read(f, buf, count, &off);
}

int kclose(int fd)
{
    if (fd >= file_table_size) {
        return -1;
    }

    file *f = opened_files[fd];
    opened_files[fd] = NULL;
    inode_rewind(f->f_dentry->inode);
    kfree(f);

    return 0;
}

int krewind(int fd)
{
    if (fd >= file_table_size) {
        return -1;
    }

    file *f = opened_files[fd];
    inode_rewind(f->f_dentry->inode);

    return 0;
}

DIR *kopendir(char *pathname)
{
    int fd = kopen(pathname, O_RDONLY);
    if (fd == -1) {
        return NULL;
    }

    struct file *f = opened_files[fd];
    inode_t *inode = f->f_dentry->inode;

    if (inode->flags != FS_DIRECTORY) {
        return NULL;
    }

    DIR *d = kmalloc(sizeof(DIR), 0, 0);
    d->fd = fd;
    d->pos = 0;

    return d;
}

int kclosedir(DIR *d)
{
    kclose(d->fd);
    kfree(d);

    return 0;
}

struct dentry *kreaddir(DIR *d)
{
    struct file *f = opened_files[d->fd];
    struct dentry *de = f->f_op->readdir(f, d);

    return de;
}

// User space file functions
int open(const char *pathname, int flags)
{
    kernel_space_open = 0;

    file *f = kmalloc(sizeof(file), 0, 0);
    f->f_dentry = get_dentry_by_path(pathname);
    f->f_op = &fops;
    f->f_mode = (mode_t) flags;
    f->f_pos = 0;
    f->f_uid = 0;
    f->f_gid = 0;
    f->f_version = 0;
    f->f_dentry->inode->open_flags = flags;

    return f->f_op->open(f->f_dentry->inode, f);
}

int read(int fd, char *buf, size_t count)
{
    if (fd >= current_process->file_table_size || buf == NULL) {
        return -1;
    }

    file *f = current_process->opened_files[fd];

    if (f == NULL) {
        return -1;
    }

    loff_t off = (loff_t) f->f_dentry->inode->offset;
    return f->f_op->read(f, buf, count, &off);
}

int write(int fd, char *buf, size_t count)
{
    printk("unused %x %s %x\n", fd, buf, count);
    return -1;
}

int close(int fd)
{
    if (fd >= current_process->file_table_size) {
        return -1;
    }

    file *f = current_process->opened_files[fd];
    current_process->opened_files[fd] = NULL;
    inode_rewind(f->f_dentry->inode);
    kfree(f);

    return 0;
}
