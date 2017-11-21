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

/*
 * Bluetooth communication
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

#include "in_bluetooth.h"
#include "in_smadata2plus.h"

void in_bluetooth_connect(struct bluetooth_inverter * inv) {
	struct sockaddr_rc addr = { 0 };

	inv->l2_packet_send_count = 1;

	inv->socket_fd = 0;
	// allocate a socket
	inv->socket_fd = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);

	// set the connection parameters (who to connect to)
	addr.rc_family = AF_BLUETOOTH;
	addr.rc_channel = (uint8_t) 1;
	str2ba(inv->macaddr, &addr.rc_bdaddr);

	// connect to server
	inv->socket_status = connect(inv->socket_fd, (struct sockaddr *) &addr,
			sizeof(addr));

	if (inv->socket_status < 0) {
		////////////printf("[BT] Connection to inverter %s failed: %s\n", inv->macaddr, strerror(errno));
	}

}

int in_bluetooth_write(struct bluetooth_inverter * inv, unsigned char * buffer,
		int len) {
	char buffer_hex[len * 3];
	int status = write(inv->socket_fd, buffer, len);

	buffer_hex_dump(buffer_hex, buffer, len);
	///////////////////////printf("[BT] Sent %d bytes: %s\n", len, buffer_hex);

	return status;

}

int in_bluetooth_connect_read(struct bluetooth_inverter * inv) {

    char buffer_hex[BUFSIZ * 3];
    int count, result;
    fd_set readset;
    struct timeval timeout;
    int maxWait = 2;
    timeout.tv_sec = maxWait;
    timeout.tv_usec = 0;

    do {
        FD_ZERO(&readset);
        FD_SET(inv->socket_fd, &readset);
        result = select(inv->socket_fd + 1, &readset, NULL, NULL, &timeout);
    } while (result == -1 && errno == EINTR);

    if (result > 0) {
        if (FD_ISSET(inv->socket_fd, &readset)) {
            /* The socket_fd has data available to be read */
            count = read(inv->socket_fd, inv->buffer, BUFSIZ);
            if (count > 0) {
                buffer_hex_dump(buffer_hex, inv->buffer, count);
                /////////////////////printf("[BT] Received %d bytes: %s\n", count, buffer_hex);
            }

            inv->buffer_len = count;
            inv->buffer_position = 0;

            return count;
        }
    }
    else if (result < 0) {
        /* An error ocurred, just print it to stdout */
        printf("Error on select(): %s\n", strerror(errno));
    } else {
        printf("No data within %d seconds\n", maxWait);
        exit(1);
    }
    return -1;
}

/* Get my mac address */
void in_bluetooth_get_my_address(struct bluetooth_inverter * inv,
		unsigned char * addr) {

	/* Get my Mac */
	struct sockaddr_rc mymac = { 0 };
	//struct sockaddr mymac;
	unsigned int mymac_size = sizeof(mymac);

	getsockname(inv->socket_fd,(struct sockaddr *) &mymac, &mymac_size);

	/* Copy to Buffer */
	memcpy(addr, &mymac.rc_bdaddr, 6);

	/* Reverse Buffer */
	buffer_reverse(addr, 6);

	/* Log my BT MAC */
	char buffer_hex[6*3];

	buffer_hex_dump(buffer_hex, addr, 6);
	///log_debug("[BT] My MAC: %s", buffer_hex);
}

/* fetch one byte from stream */
char in_bluetooth_get_byte(struct bluetooth_inverter * inv) {

	/* Check if its neccessary to fetch new buffer content */
	while (inv->buffer_len == 0 || inv->buffer_len <= inv->buffer_position) {
		in_bluetooth_connect_read(inv);
	}

	return inv->buffer[inv->buffer_position++];

}

/* fetch multiple bytes */
void in_bluetooth_get_bytes(struct bluetooth_inverter * inv,
		unsigned char *buffer, int count) {
	for (int i = 0; i < count; ++i) {
		if (buffer == NULL)
			in_bluetooth_get_byte(inv);
		else
			buffer[i] = in_bluetooth_get_byte(inv);
	}
}

