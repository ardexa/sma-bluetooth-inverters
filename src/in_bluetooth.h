/*
 *  OpenSunny -- OpenSource communication with SMA Readers
 *
 *  Copyright (C) 2012 Christian Simon <simon@swine.de>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program (see the file COPYING included with this
 *  distribution); if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef OPENSUNNY_IN_BLUETOOTH_H_
#define OPENSUNNY_IN_BLUETOOTH_H_

#include <stdio.h>
#include <string>	

using namespace std;

struct bluetooth_inverter {
	char name[32];
	char macaddr[18];
	unsigned char password[13];
	int socket_fd;
	int socket_status;
	unsigned char buffer[BUFSIZ];
	int buffer_len;
	int buffer_position;
	int l2_packet_send_count;
	unsigned int serial;
	struct smadata2_model *model;
};

/* level1 packet */
struct smadata2_l1_packet {
	int length;
	unsigned char checksum;
	unsigned char src[6];
	unsigned char dest[6];
	int cmd_code;
	unsigned char content[BUFSIZ];
};

/* level2 packet */
struct smadata2_l2_packet {
	unsigned char src[6];
	unsigned char dest[6];
	unsigned char ctrl1;
	unsigned char ctrl2;
	unsigned char archcd;
	unsigned char zero;
	unsigned char c;
	unsigned char content[BUFSIZ];
	int content_length;
};

/* smadata2 value */
struct smadata2_value {
	char name[64];
	char unit[5];
	float factor;
	unsigned long value;
	int timestamp;
	int r_value_pos;
	int r_value_len;
	int r_timestamp_pos;

};

/* smadata2 query */
struct smadata2_query {
	unsigned char q_ctrl1;
	unsigned char q_ctrl2;
	unsigned char q_archcd;
	unsigned char q_zero;
	unsigned char q_c;
	unsigned char q_content[128];
	int q_content_length;
	unsigned char r_ctrl1;
	unsigned char r_ctrl2;
	struct smadata2_value values[12];
	int value_count;
};

/* smadata2 model */
struct smadata2_model {
	char name[64];
	unsigned char code[2];
	int max_power[4];
	int model_count;
};

/* This is used by the vector to store all the data that is collected */
struct vec_data {
	string name;
	float value;
	string units;
};


void in_bluetooth_connect(struct bluetooth_inverter * inv);
int in_bluetooth_connect_read(struct bluetooth_inverter * inv);
char in_bluetooth_get_byte(struct bluetooth_inverter * inv);
void in_bluetooth_get_bytes(struct bluetooth_inverter * inv,
		unsigned char *buffer, int count);
int in_bluetooth_write(struct bluetooth_inverter * inv, unsigned char * buffer,
		int len);
void in_bluetooth_get_my_address(struct bluetooth_inverter * inv,
		unsigned char * addr);

#endif /* OPENSUNNY_IN_BLUETOOTH_H_ */
