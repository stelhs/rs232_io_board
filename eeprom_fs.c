/*
 * eeprom.c
 *
 *  Created on: 15 июля 2016 г.
 *      Author: Michail Kurochkin
 */

#include <string.h>
#include <avr/eeprom.h>
#include "types.h"
#include "board.h"
#include "eeprom_fs.h"

#define EEPROM_FS_MARKER 0x1234
#define EEPROM_FILE_MARKER 0x55
#define EEPROM_MAX_SIZE 1024

struct eeprom_file {
	u8 marker;
	char filename[8];
	u16 size;
	u16 crc;
};


static u16 crc16(u8 *buf, u16 size)
{
	u16 crc = 0xFFFF;
	u8 i;

	while(size--){
		crc ^= *buf++ << 8;

		for(i = 0; i < 8; i++)
			crc = crc & 0x8000 ? ( crc << 1 ) ^ 0x1021 : crc << 1;
	}

	return crc;
}


static void eeprom_write_block_safe(u8 *source, u8 *dest, u16 size)
{
	u16 i;

	for(i = 0; i < size; i++)
		if(source[i] != eeprom_read_byte(dest + i)) {
			CLEAR_WATCHDOG();
			eeprom_write_byte(dest + i, source[i]);
		}
}


/**
 * Check file system marker
 * @return 0 if marker is present
 */
static int check_fs_marker(void)
{
	u16 marker;

	eeprom_read_block(&marker, 0, 2);
	if (marker != EEPROM_FS_MARKER)
		return -EIO;

	return 0;
}

/**
 * Init file system
 */
void eeprom_init_fs(void)
{
	int rc;
	rc = check_fs_marker();

	if (!rc)
		return;

	eeprom_fs_format();
	return;
}


/**
 * Find and return struct eeprom_file form EEPROM file system
 * @param filename - finded file
 * @param file - return copy of struct eeprom_file from EEPROM
 * @return eeprom pointer to struct eeprom_file
 */
static int eeprom_file_find(char *filename, struct eeprom_file *file)
{
	struct eeprom_file f;
	int counter = 2; // after marker

	while (counter < EEPROM_MAX_SIZE) {
		eeprom_read_block(&f, (void *)counter, sizeof(struct eeprom_file));
		if (f.marker != EEPROM_FILE_MARKER)
			break;

		if (strcmp(f.filename, filename) == 0) {
			memcpy(file, &f, sizeof(f));
			return counter;
		}

		counter += sizeof(struct eeprom_file);
		counter += f.size;
	}
	return -ENODATA;
}


/**
 * Return EEPROM pointer to free space
 */
static int eeprom_get_free_space_pointer(void)
{
	struct eeprom_file f;
	int counter = 2; // after marker

	while (counter < EEPROM_MAX_SIZE) {
		eeprom_read_block(&f, (void *)counter, sizeof(struct eeprom_file));
		if (f.marker != EEPROM_FILE_MARKER)
			return counter;

		counter += sizeof(struct eeprom_file);
		counter += f.size;
	}
	return 0;
}


/**
 * format EEPROM partiton
 */
void eeprom_fs_format(void)
{
	struct blob {
		u16 marker;
		u8 empty;
	}blb;

	blb.marker = EEPROM_FS_MARKER;
	blb.empty = 0;
	eeprom_write_block_safe((u8 *)&blb, NULL, sizeof(struct blob));
}


/**
 * Read file from eeprom
 * @param filename - readed file
 * @param returned_data - pointer to date file
 * @return 0 if success
 */
int eeprom_read_file(char *filename, u8 *returned_data)
{
	int rc;
	int counter = 2; // after marker
	struct eeprom_file f;
	u8 file_data[128];
	u16 crc;

	rc = check_fs_marker();
	if (rc)
		return rc;

	rc = eeprom_file_find(filename, &f);
	if (rc < 0)
		return rc;

	counter = rc + sizeof f;
	eeprom_read_block(file_data, (void *)counter, f.size);
	crc = crc16(file_data, f.size);
	if (crc != f.crc)
		return -ECORRUPT;

	memcpy(returned_data, file_data, f.size);
		return f.size;

	return 0;
}


/**
 * Create new file on EEPROM
 * @param filename - new file name
 * @param size - size of new file
 * @return 0 - if success
 */
int eeprom_create_file(char *filename, int size)
{
	int rc;
	int counter;
	struct eeprom_file f;

	rc = check_fs_marker();
	if (rc)
		return rc;

	/* check if file exist */
	rc = eeprom_file_find(filename, &f);
	if (rc > 0)
		return rc;

	rc = eeprom_get_free_space_pointer();
	if(!rc)
		return -ENOSPC;
	counter = rc;

	strcpy(f.filename, filename);
	f.marker = EEPROM_FILE_MARKER;
	f.size = size;
	eeprom_write_block_safe((u8 *)&f, (void *)counter, sizeof(f));
	return 0;
}


/**
 * Write file into EEPROM
 * @param filename
 * @param data
 * @return 0 - if success
 */
int eeprom_write_file(char *filename, u8 *data)
{
	struct eeprom_file f;
	int counter;
	int rc;

	rc = check_fs_marker();
	if (rc)
		return rc;

	rc = eeprom_file_find(filename, &f);
	if (rc < 0)
		return rc;
	counter = rc;

	f.crc = crc16(data, f.size);
	eeprom_write_block_safe((u8 *)&f, (void *)counter, sizeof f);
	counter += sizeof f;
	eeprom_write_block_safe(data, (void *)counter, f.size);

	return 0;
}
