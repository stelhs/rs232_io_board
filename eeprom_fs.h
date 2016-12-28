/*
 * eeprom.h
 *
 *  Created on: 15 июля 2016 г.
 *      Author: Michail Kurochkin
 */
#ifndef EEPROM_FS_H_
#define EEPROM_FS_H_

void eeprom_fs_format(void);
int eeprom_read_file(char *filename, u8 *returned_data);
int eeprom_create_file(char *filename, int size);
int eeprom_write_file(char *filename, u8 *data);
void eeprom_init_fs(void);

#endif /* EEPROM_FS_H_ */
