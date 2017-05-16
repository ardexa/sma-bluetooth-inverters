/* 
* Copyright (c) 2013-2017 Ardexa Pty Ltd
*
* This code is licensed under GPL v3
*
*/

#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include "utils.hpp"
#include "arguments.hpp"


using namespace std;

/* Global variables. */ 
int g_debug = DEFAULT_DEBUG_VALUE;

/* Our private functions */
int read_bt(unsigned char *buf, int buf_size);
unsigned char *check_header(unsigned char *buf, int len);
unsigned int get_header_cmd(unsigned char *buf, int len);
int create_header(int cmd, unsigned char *buf, int size);
int write_bt(unsigned char *buf, int msg_size);
int dump_hex(char *logstr, unsigned char *buf, int length, int force);
int send_packet(unsigned char *buf, int len);
int read_packet();
int pack_smanet2_data(unsigned char *buf, int size, unsigned char *src, unsigned int num);
int handle_init_1(void);
int handle_get_signal_strength(int *signal_strength);
int handle_sma_bt_request(string request_name, unsigned char c1, unsigned char c2, unsigned char request_src[], unsigned int request_src_size, unsigned long *result_long);
int handle_logon(string password);
unsigned char *retry_send_receive(int bytes_send, string log_str);
int build_data_packet(unsigned char *buf, int size, unsigned char c1, unsigned char c2,
                      unsigned char a, unsigned char b, unsigned char c);
int add_fcs_checksum(unsigned char *buf, int size);



/* Structures for reading and creating  package headers */
/* Note: this structure must be byte-aligned, we use pragma to stop the browser from aligning with extra empty bytes */
#pragma pack (push)       /* Push current alignment to compiler stack */
#pragma pack (1)          /* Force byte padding (instead of word or long padding */
typedef struct {
  unsigned char  sync;
  unsigned short len;
  unsigned char  chksum;
  bdaddr_t       src_addr;
  bdaddr_t       dst_addr;
  unsigned short cmd;
} packet_hdr;
#pragma pack (pop)        /* Restore original alignment from compiler stack */


/* Local storage Bluetooth information and buffers */
typedef struct {
    int                 sock;                  /* Socket with SunnyBoy */
    struct sockaddr_rc  src_addr;              /* the local Bluetooth Addres in binary format  */
    struct sockaddr_rc  dst_addr;              /* the remote Bluetooth Addres in binary format */
    unsigned char       netid;                 /* The netid, part of the SMA messages */
    int                 signal;                /* Signal strength, max = 255 */
    unsigned int        fcs_checksum;          /* Final checksum for messages */
    unsigned char       packet_send_counter;   /* Keeps track of the amount of data packages sent */
    unsigned char       snd_buf[1024];
    unsigned char       rcv_buf[1024];
} _comms;



/* Global declarations */
_comms comms = { 0,{0}, {0}, 1, 0, 0xffff, 0, {0}, {0} };


/* FCS checksum table */
unsigned int fcstab[256] = {
   0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
   0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
   0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
   0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
   0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
   0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
   0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
   0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
   0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
   0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
   0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
   0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
   0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
   0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
   0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
   0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
   0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
   0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
   0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
   0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
   0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
   0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
   0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
   0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
   0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
   0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
   0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
   0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
   0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
   0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
   0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
   0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};

/* Definitions for building packages */
#define HDLC_SYNC           0x7e    /* Sync character, denotes start of message and surrounds data */
#define HDLC_ESC            0x7d    /* Escape character, indicates modification of next byte */
#define DATA_CHK            0xa0    /* It seems that this byte at position 32 indicates valid data */

/* Packet header offsets */
#define PKT_OFF_LEN1        1
#define PKT_OFF_LEN2        2
#define PKT_OFF_CMD         16
#define PKT_OFF_DATASTART   18
/* Packet command types */
#define CMD_DATA            0x0001  /* Data */
#define CMD_INITIALISE      0x0002  /* Initialise */
#define CMD_BT_SIGNAL_REQ   0x0003  /* Request Bluetooth signal */
#define CMD_BT_SIGNAL_RSP   0x0004  /* Response Bluetooth signal */
#define CMD_0005            0x0005
#define CMD_IDATA           0x0008  /* Incomplete data (continued in next package) */
#define CMD_000A            0x000A
#define CMD_000C            0x000C

/* Data indexes */
#define IDX_NETID            4
#define IDX_LOGONFAIL       24
#define IDX_DATA_CHK        14  /* It seems this always contains xa0 with a valid data package */
#define IDX_BT_SIGNAL        4
#define IDX_DATETIME        45
#define IDX_VALUE           49

/* Message contents */
unsigned char fourzeros[]           = { 0, 0, 0, 0 };
unsigned char long_one[]            = { 0x01, 0x00, 0x00, 0x00 };
unsigned char sixff[]               = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

unsigned char smanet2dataheader[]   = { 0xff, 0x03, 0x60, 0x65 };
unsigned char smanet2init1[]        = { 0x00, 0x04, 0x70, 0x00 };
unsigned char smanet2getBT[]        = { 0x05, 0x00 };
unsigned char smanet2logon[]        = { 0x80, 0x0c, 0x04, 0xfd, 0xff, 0x07, 0x00, 0x00, 0x00, 0x84, 0x03, 0x00, 0x00 };

/* The following arrays are for retrieving data from thje inverter. Note
	1. the first item is the 'command group' ...ie; 0x80
	2. Columns 6+7 and 10+11 are repeated
 */
unsigned char smanet2totalWh[]      = { 0x80, 0x00, 0x02, 0x00, 0x54, 0x00, 0x01, 0x26, 0x00, 0xff, 0x01, 0x26, 0x00 };
unsigned char smanet2curWatt[]      = { 0x80, 0x00, 0x02, 0x00, 0x51, 0x00, 0x3f, 0x26, 0x00, 0xff, 0x3f, 0x26, 0x00, 0x0e };
unsigned char smanet2netVolt[]      = { 0x80, 0x00, 0x02, 0x00, 0x51, 0x00, 0x48, 0x46, 0x00, 0xff, 0x48, 0x46, 0x00, 0x0e };
unsigned char smanet2netAmp[]       = { 0x80, 0x00, 0x02, 0x00, 0x51, 0x00, 0x50, 0x46, 0x00, 0xff, 0x50, 0x46, 0x00, 0x0e };
unsigned char smanet2netFreq[]      = { 0x80, 0x00, 0x02, 0x00, 0x51, 0x00, 0x57, 0x46, 0x00, 0xff, 0x57, 0x46, 0x00, 0x0e };
unsigned char smanet2pvVolt[]       = { 0x80, 0x00, 0x02, 0x80, 0x63, 0x00, 0x1F, 0x45, 0x00, 0xff, 0x1f, 0x45, 0x00 }; 
unsigned char smanet2pvAmpere[]     = { 0x80, 0x00, 0x02, 0x80, 0x63, 0x00, 0x21, 0x45, 0x00, 0xff, 0x21, 0x45, 0x00 }; 




/* Main function */
int main(int argc, char **argv) {
	int status, result;
	string device_name = "", serial_number = "";

	/* If not run as root, exit */
	if (check_root() == false) {
		cout << "This program must be run as root" << endl;
		return 1;
	}

	/* Check for existence of PID file */
	if (!check_pid_file()) {
		return 2;
	}

	/* This class object defines the initial configuration parameters */     
	arguments arguments_list;     
	result = arguments_list.initialize(argc, argv);     
	if (result != 0) {         
		cerr << "Incorrect arguments" << endl;         
		return 3;     
	}     

	/* set the global debug value. this won't change during runtime */     
	g_debug = arguments_list.get_debug();

	/* start the timer */
	time_t start = time(nullptr);

	/* If this is a discovery run, then print all SMA BT available devices and exit. Do NOT try and read from the devices, since some of them 
      may not be SMA products */
	if (arguments_list.get_discovery()) {
		cout << "This is a discovery run to find all local Bluetooth devices, whether they are SMA or not." << endl;
		get_bt_name("", &device_name);
		time_t endi = time(nullptr);  
		cout << "Query took: " << (endi-start) << " Seconds\n" << endl;  
		return 0; 
	}

    /* Get the local address */
    struct hci_dev_info di;
    int dev_id = hci_get_route(NULL);
    if ( hci_devinfo(dev_id, &di) >= 0 ) {
        comms.src_addr.rc_family = AF_BLUETOOTH;
        comms.src_addr.rc_channel = (uint8_t) 1;
        memcpy(&comms.src_addr.rc_bdaddr, &di.bdaddr, sizeof(bdaddr_t));
    }
	else {
		cout << "Local Bluetooth address not found. " << endl;
		return 2;
	}


	/* this for loop will iterate through each address in the vector of BT addresses */
	for(unsigned int i = 0; i <= arguments_list.addresses.size() - 1; i++) {

		/* allocate a sock */
		comms.sock = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
		if (comms.sock <= 0) {
			cout << "ERROR opening bluetooth socket" << endl;
			return 3;
		}

		if (g_debug >= 1) cout << "\nProcessing BT address: " << arguments_list.addresses[i] << endl;
		/* Get the address of the devices were are interested in */
		get_bt_name(arguments_list.addresses[i], &device_name);

		/* Get the serial number out of the device name */
		serial_number = get_serial(device_name);
		if (g_debug >= 1) cout << "Serial number: " << serial_number << endl;

		 /* set the connection parameters (who to connect to) */
		 comms.dst_addr.rc_family = AF_BLUETOOTH;
		 comms.dst_addr.rc_channel = (uint8_t) 1;
		 /* str2ba takes an string of the form ``XX:XX:XX:XX:XX:XX", where each XX is a hexadecimal number 
		   specifying an octet of the 48-bit address, and packs it into a 6-byte bdaddr_t. ba2str */
		 str2ba(arguments_list.addresses[i].c_str(), &comms.dst_addr.rc_bdaddr );
		
		string current_date = get_current_date();
		int handle_status = 0; 
		/* connect to server */
		status = connect(comms.sock, (struct sockaddr *)&comms.dst_addr, sizeof(comms.dst_addr));
		if(status == 0) {
			string header = "# Datetime, serial number, BT signal strength (%), Energy (Wh), AC Power (W), Net Volts (V), Net Amps (A), Grid Freq (Hz)";
			int signal_strength = 0;
			unsigned long energy = 0; 		/* Result will be in Wh */
			unsigned long AC_power = 0; 	/* Result will be in W */
			unsigned long grid_freq = 0; 	/* Result will be in Hz * 100 */
			unsigned long net_volts = 0; 	/* Result will be in V * 100 */
			unsigned long net_amps = 0; 	/* Result will be in A * 100 */
			bool error = false;

			handle_status = handle_init_1();
			if (handle_status > 0) error = true;
			handle_status = handle_get_signal_strength(&signal_strength);
			if (handle_status > 0) error = true;
			handle_status = handle_logon(arguments_list.get_password());
			if (handle_status > 0) error = true;
			handle_status = handle_sma_bt_request("handle_energy", 0x09, 0xa0, smanet2totalWh, sizeof(smanet2totalWh), &energy);
			if (handle_status > 0) error = true;
			handle_status = handle_sma_bt_request("handle_AC_power", 0x09, 0xa1, smanet2curWatt, sizeof(smanet2curWatt), &AC_power);
			if (handle_status > 0) error = true;
			handle_status = handle_sma_bt_request("handle_net_frequency", 0x09, 0xa0, smanet2netFreq, sizeof(smanet2netFreq), &grid_freq);
			if (handle_status > 0) error = true;
			handle_status = handle_sma_bt_request("handle_net_voltage", 0x09, 0xa1, smanet2netVolt, sizeof(smanet2netVolt), &net_volts);
			if (handle_status > 0) error = true;
			handle_status = handle_sma_bt_request("handle_net_amps", 0x09, 0xa1, smanet2netAmp, sizeof(smanet2netAmp), &net_amps);
			if (handle_status > 0) error = true;

			/* If any errors, don't log anything. Process the next address */
			if (error) {
				if (g_debug > 0) cout << "One of the calls returned invalid" << endl;
				continue;
			}

			/* If volts, grid freq or amps are an INT value of 16777215, it means it is max as FFFF. Set to 0 */
			if (grid_freq > 16777200) grid_freq = 0;
			if (net_volts > 16777200) net_volts = 0;
			if (net_amps > 16777200) net_amps = 0;
				
			float net_volts_fl = net_volts/100.0;
			float grid_freq_fl = grid_freq/100.0;
			float net_amps_fl = net_amps/100.0;
			/* round values to 2 decimal places */
			stringstream stream_volts, stream_amps, stream_freq;
		 	stream_volts << fixed << setprecision(2) << net_volts_fl;
		 	string net_volts_str = stream_volts.str();
		 	stream_amps << fixed << setprecision(2) << net_amps_fl;
		 	string net_amps_str = stream_amps.str();
		 	stream_freq << fixed << setprecision(2) << grid_freq_fl;
		 	string grid_freq_str= stream_freq.str();

			string datetime = get_current_datetime();
			string line = datetime + "," + serial_number + "," + to_string(signal_strength) + "," + to_string(energy) + "," + to_string(AC_power) + "," + net_volts_str + "," + net_amps_str + "," + grid_freq_str;
			if (g_debug >= 1) {
				cout << header << endl;
				cout << line << endl;
			}

			
			/* Log the line based on the inverter serial number, in the logging directory */
			string full_dir = arguments_list.get_log_directory() + "/" + serial_number;
			/* log to a date and to a 'latest' file */
			log_line(full_dir, current_date + ".csv", line, header, true);

			/* close connection */
			close(comms.sock);

		}
		else { 
		 	if (g_debug > 0) cout << "SMA BT connection to address: " <<  arguments_list.addresses[i] << " failed" << endl; 
		}
	}


	/* stop the timer */
	time_t end = time(nullptr);
	if (g_debug > 0) cout << "Query took: " << (end-start) << " Seconds\n" << endl;
	/* remove the PID file */
	remove_pid_file();

   return 0;
}


/*******************************************
Function handle_init_1 - send the first initialisation string
Globals: comms structure
returns: 0 if succesfull
*******************************************/
int handle_init_1(void) {

    unsigned char *p = comms.snd_buf;
    int len, bytes_read, cmd, size=sizeof(comms.snd_buf);
    unsigned char *payload;

    /* Start by reading from BlueTooth */
    bytes_read = read_bt(comms.rcv_buf, sizeof(comms.rcv_buf));
    payload = check_header(comms.rcv_buf, bytes_read);
    if(payload) {
      comms.netid = payload[IDX_NETID];          
    }
    else {
      if (g_debug > 1) printf("\nSend init 1: received invalid message header\n");
      return 1;
    }
    if (g_debug > 1)  printf("\nSend init 1\n");

    len = create_header(CMD_INITIALISE, p, size);
    p += len; size -= len;
    len = pack_smanet2_data(p, size, smanet2init1, sizeof(smanet2init1));
    p += len; size -= len;
    len = pack_smanet2_data(p, size, &comms.netid, 1);
    p += len; size -= len;
    len = pack_smanet2_data(p, size, fourzeros, sizeof(fourzeros));
    p += len; size -= len;
    len = pack_smanet2_data(p, size, long_one, sizeof(long_one));
    p += len; size -= len;
    if(send_packet(comms.snd_buf, p - comms.snd_buf)) {
        /* Wait for the correct answer */
        while( (bytes_read = read_bt(comms.rcv_buf, sizeof(comms.rcv_buf))) > 0 ) {
            cmd = get_header_cmd(comms.rcv_buf, bytes_read);
            if(cmd == CMD_0005)
                break;
        }
        if (cmd != CMD_0005) {
            printf("No valid response from handle_init_1\n");
            return 1;
        }
    }
    else {
        if (g_debug > 0) printf("Could not send data of handle_init_1\n");
        return 1;
    }
    return 0;
}

/*******************************************
Function handle_get_signal_strength - send the first initialisation string
Globals: comms structure
returns: 0 if succesfull
*******************************************/
int handle_get_signal_strength(int *signal_strength) {
    unsigned char *p = comms.snd_buf;
    int len, bytes_read, cmd, size=sizeof(comms.snd_buf);
    unsigned char *payload;

    /* Start by reading from BlueTooth */
    if (g_debug > 1) printf("\nSend Get Bluetooth signal strength\n");
    len = create_header(CMD_BT_SIGNAL_REQ, p, size);
    p += len; size -= len;
    len = pack_smanet2_data(p, size, smanet2getBT, sizeof(smanet2getBT));
    p += len; size -= len;
    if(send_packet(comms.snd_buf, p - comms.snd_buf)) {
        /* Wait for the correct answer */
        while( (bytes_read = read_bt(comms.rcv_buf, sizeof(comms.rcv_buf))) > 0) {
            cmd = get_header_cmd(comms.rcv_buf, bytes_read);
            if(cmd == CMD_BT_SIGNAL_RSP) {
                payload = check_header(comms.rcv_buf, bytes_read);
                break;
            }
        }
        if (cmd != CMD_BT_SIGNAL_RSP) {
            printf("No valid response from handle_get_signal_strength\n");
            return 1;
        }
    }
    else {
        if (g_debug > 0) printf("Could not send data of handle_get_signal_strength\n");
        return 1;
    }
    *signal_strength = (int)(payload[IDX_BT_SIGNAL]/2.55);          
    if (g_debug > 0) cout << "Bluetooth signal strength: " << *signal_strength << endl; 
    return 0;
}

/*******************************************
Function handle_logon - Logon at the SunnyBoy
uses globals: comms
returns: pointer directly after the copied bytes
*******************************************/
int handle_logon(string password) {
    unsigned char *p = comms.snd_buf;
    int  len, bytes_read, cmd, logon_failed, size=sizeof(comms.snd_buf);
    unsigned char          code;
    unsigned char *payload;
    time_t        cur_time;

    if (g_debug > 1) printf("\nSend Logon\n");
    len = create_header(CMD_DATA, p, size);
    p += len; size -= len;
    len = build_data_packet(p, size, 0x0e, 0xa0, 0, 0x01, 0x01);
    p += len; size -= len;
    len = pack_smanet2_data(p, size, smanet2logon, sizeof(smanet2logon));
    p += len; size -= len;
    cur_time = time(NULL);
    /* Add it three times to the buffer */
    len = pack_smanet2_data(p, size, (unsigned char *)&cur_time, sizeof(cur_time));
    p += len; size -= len;
    len = pack_smanet2_data(p, size, fourzeros, sizeof(fourzeros));
    p += len; size -= len;
    /* Encode the password */
    for (unsigned i = 0; i < sizeof(password.c_str()); i++) {
      code = (password.c_str()[i] + 0x88) % 0xff;
      len = pack_smanet2_data(p, size, &code, 1);
      p += len; size -= len;
    }
    len = add_fcs_checksum(p, size);
    p += len; size -= len;
    if(send_packet(comms.snd_buf, p - comms.snd_buf)) {
        /* Wait for the correct answer */
        while( (bytes_read = read_bt(comms.rcv_buf, sizeof(comms.rcv_buf))) > 0) {
            payload = check_header(comms.rcv_buf, bytes_read);
            cmd = get_header_cmd(comms.rcv_buf, bytes_read);
            if(cmd == CMD_DATA)
                break;
        }
        if (cmd != CMD_DATA) {
            printf("No valid response from handle_logon\n");
            return 1;
        }
    }
    else {
        if (g_debug > 0) printf("Could not send data of handle_logon\n");
        return 1;
    }
    /* Check if logon succeeded */
    logon_failed = payload[IDX_LOGONFAIL];          
    if(logon_failed) {
        if (g_debug > 0) printf("Logon failed, check password\n");
        return 1;
    }
    if (g_debug >= 1) printf("Logon succeeded\n");
    return 0;
}




/*******************************************
Function handle_net_voltage - Get the Net Voltage
uses globals: comms
returns: pointer directly after the copied bytes
*******************************************/
int handle_sma_bt_request(string request_name, unsigned char c1, unsigned char c2, unsigned char request_src[], unsigned int request_src_size, unsigned long *result_long) 
{  
	unsigned char *p = comms.snd_buf;
	int           len, size=sizeof(comms.snd_buf);
	unsigned char *payload;

	len = create_header(CMD_DATA, p, size);
	p += len; size -= len;
	len = build_data_packet(p, size, c1, c2, 0, 0, 0 );
	p += len; size -= len;
	len = pack_smanet2_data(p, size, request_src, request_src_size);
	p += len; size -= len;
	len = add_fcs_checksum(p, size);
	p += len; size -= len;
	payload = retry_send_receive(p-comms.snd_buf, request_name.c_str());
	if(!payload)
		return -1;
	/* get data from the payload. Result is encoded thus: UINT32 - Little Endian (DCBA)
      It is 3 bytes long. */
	memcpy(result_long, &payload[IDX_VALUE], 3);
	if (g_debug > 0) cout << "Request: " << request_name << " . Result: " << *result_long << endl;
	return 0;
}



/*******************************************
Function retry_send_receive - send data-request and receive respons, retry as necessary
  bytes_send : amount of data to send (buffer in comms.snd_buf)
  log_str : string to use for logging errors
uses globals: comms
returns: pointer directly after the copied bytes
*******************************************/
unsigned char *retry_send_receive(int bytes_send, string log_str) {
	
	int           bytes_read, cmd, data_chk, fcs_retry=10;
    unsigned char *payload;

    while(fcs_retry--) {
        if(send_packet(comms.snd_buf, bytes_send)) {
            /* Wait for the correct answer */
            bytes_read = read_packet();
            if(bytes_read > 0) {  /* Data error, checksum or missing sync, force a retry */
                payload = check_header(comms.rcv_buf, bytes_read);
                cmd = get_header_cmd(comms.rcv_buf, bytes_read);
                data_chk = payload[IDX_DATA_CHK];
                if(cmd == CMD_DATA) {
                    if(data_chk==DATA_CHK)
                        break;  /* Retry */
                }
            }
        }
        else {
            cout << "Could not send data of " << log_str << endl;
        }
        sleep(5); /* Wait 5 seconds before trying again */
    }
    if (cmd != CMD_DATA || data_chk != DATA_CHK) {
        cout << "No valid response from " << log_str << endl;
        return NULL;
    }
    return payload;
}

/*******************************************
Function build_data_packet - Pack a smanet 2 message, translating bytes as necessary
  buf   : destination buffer
  size  : size of the buffer.
  c1    : 
  c2    : 
  pc    : 
  a     :
  b     :
  c     :
returns: Amount of bytes added to buffer
*******************************************/
int build_data_packet(unsigned char *buf, int size, unsigned char c1, unsigned char c2,
                      unsigned char a, unsigned char b, unsigned char c) {
    unsigned char *p = buf;
    int len;

    *p++ = HDLC_SYNC; /* exclude from checksum */
    size--;
    len = pack_smanet2_data(p, size, smanet2dataheader, sizeof(smanet2dataheader));
    p += len; size -= len;
    len = pack_smanet2_data(p, size, &c1, 1);
    p += len; size -= len;
    len = pack_smanet2_data(p, size, &c2, 1);
    p += len; size -= len;
    len = pack_smanet2_data(p, size, sixff, sizeof(sixff));
    p += len; size -= len;
    len = pack_smanet2_data(p, size, &a, 1);
    p += len; size -= len;
    len = pack_smanet2_data(p, size, &b, 1);
    p += len; size -= len;
    /* Some software uses a "unknown secret inverter code", but six ff bytes works too */
    len = pack_smanet2_data(p, size, sixff, sizeof(sixff));
    p += len; size -= len;
    len = pack_smanet2_data(p, size, fourzeros, 1);    /* only need one 0x0, but must be in checksum */
    p += len; size -= len;
    len = pack_smanet2_data(p, size, &c, 1);
    p += len; size -= len;
    len = pack_smanet2_data(p, size, fourzeros, sizeof(fourzeros));
    p += len; size -= len;
    len = pack_smanet2_data(p, size, &comms.packet_send_counter, 1);
    p += len; size -= len;
    /* Check the packet counter */
    if (comms.packet_send_counter++ > 75)
        comms.packet_send_counter = 1;
    return p - buf;
}


/*******************************************
Function add_fcs_checksum - Add an fcs checksum
  buf   : destination buffer
  size  : size of the buffer.
returns: Amount of bytes added to buffer
*******************************************/
int add_fcs_checksum(unsigned char *buf, int size) {
    unsigned char *p = buf;
    if(size > 3) {
      comms.fcs_checksum = comms.fcs_checksum ^ 0xffff;
      *p++ = comms.fcs_checksum & 0x00ff;
      *p++ = (comms.fcs_checksum >> 8) & 0x00ff;
      *p++ = HDLC_SYNC;
    }
    return p - buf;
}

/*******************************************
Function pack_smanet2_data - Pack a smanet 2 message, translating bytes as necessary
                             This function also calculates the checksum, so all data
                             that should be "checksummed" must pass this function
  buf   : destination buffer
  size  : size of the buffer.
  src   : source buffer
  len   : length of source buffer
returns: Amount of bytes added to destination buffer
*******************************************/
int pack_smanet2_data(unsigned char *buf, int size, unsigned char *src, unsigned int num) {
    unsigned char *p = buf;

    while (num-- && size--) {
        comms.fcs_checksum = (comms.fcs_checksum >> 8) ^ (fcstab[(comms.fcs_checksum ^ *src) & 0xff]);
        if (*src == HDLC_ESC || *src == HDLC_SYNC || *src == 0x11 || *src == 0x12 || *src == 0x13) {
            *p++ = HDLC_ESC;
            *p++ = *src++ ^ 0x20;
        } else
            *p++ = *src++;
    }

    return p-buf;
}

/*******************************************
Function read_packet - Read a packet from BlueTooth, translating escape characters and checksum
returns: Amount of bytes read, or -1 if an error occurs
*******************************************/
int read_packet() {
    int bytes_read, bytes_in_packet;
    unsigned int fcs_checksum, msg_checksum;
    unsigned char *from, *src, *dest; /* Buffer pointers for translating data */
    packet_hdr *hdr;

    bytes_read = read_bt(comms.rcv_buf, sizeof(comms.rcv_buf));
    bytes_in_packet = bytes_read;
    hdr = (packet_hdr *)comms.rcv_buf;
    /* If we received a data package (CMD 01), do some checking */
    if(hdr->cmd == CMD_DATA) {
        if(comms.rcv_buf[bytes_read-1] != HDLC_SYNC) {
            printf("Invalid data message, missing 0x7e at end\n");
            return -1;
        }
        /* Find out where translation and checksumming should start */
        from = comms.rcv_buf + PKT_OFF_DATASTART;
        while((*from != HDLC_SYNC) && (from < comms.rcv_buf+bytes_read ))
          from++;
        if(from < comms.rcv_buf+bytes_read) {
            /* Skip the SYNC character itself */
            from++;
            /* Translate all escaped characters */
            src = from;
            dest = from;
            while(src < comms.rcv_buf+bytes_read) {
                if( *src == HDLC_ESC ) {    /*Found escape character. Need to convert*/
                    src++;                  /* Skip the escape character */
                    *dest++ = *src++^0x20;  /* and Xor the following character with 0x020 */
                    bytes_in_packet--;
                }
                else
                  *dest++ = *src++;
            }
            /* Calculate the checksum */
            fcs_checksum = 0xffff;
            src = from;
            while(src < comms.rcv_buf+bytes_in_packet-3) {
                fcs_checksum = (fcs_checksum >> 8) ^ (fcstab[(fcs_checksum ^ *src++) & 0xff]);
            }
        }
        if (g_debug > 1) dump_hex("Translated package", comms.rcv_buf, bytes_in_packet, 0);
        msg_checksum = (comms.rcv_buf[bytes_in_packet-2]<<8) + comms.rcv_buf[bytes_in_packet-3];
        fcs_checksum = fcs_checksum ^ 0xffff;
        if(msg_checksum != fcs_checksum) {
            printf("Checksum failed: calculated %04x instead of %04x\n", fcs_checksum, msg_checksum);
            return -1;
        }
    }
    return bytes_in_packet;
}

/*******************************************
Function send_packet - Send a smanet_packet, adding length and checksum
  buf   : buffer
  len   : length of buffer
*******************************************/
int send_packet(unsigned char *buf, int len) {
    packet_hdr *hdr;

    /* Fill in the length */
    hdr = (packet_hdr *)buf;
    hdr->len = len;
    hdr->chksum = hdr->sync ^ buf[PKT_OFF_LEN1] ^ buf[PKT_OFF_LEN2];
    return write_bt(buf, len);
}

/*******************************************
Function check_header - Perform some basic checking on the header, to see if it is valid
  buf   : buffer to be dumped in headecimal format
  len   : length of data in this buffer
returns: pointer to payload if header ok, NULL if header false
*******************************************/
unsigned char *check_header(unsigned char *buf, int len) {
    packet_hdr *hdr;
    int chksum = 0;

    hdr = (packet_hdr *)buf;
    if(hdr->sync != HDLC_SYNC) {
      printf("WARNING: Start Of Message is %02x instead of %02x\n", hdr->sync, HDLC_SYNC);
      printf("pkt  checksum: 0x%x, calc checksum: 0x%x\n", hdr->chksum, chksum);
      return NULL; 
    }
    chksum = hdr->sync ^ buf[PKT_OFF_LEN1] ^ buf[PKT_OFF_LEN2];
    if (hdr->chksum != chksum) {
      printf("WARNING: checksum mismatch\n");
      printf("pkt  checksum: 0x%x, calc checksum: 0x%x\n", hdr->chksum, chksum);
      return NULL; 
    }
    return buf + PKT_OFF_DATASTART;
}

/*******************************************
Function get_header_cmd - Read the header, and give back te command
  buf   : buffer to be dumped in headecimal format
  len   : length of data in this buffer
returns: pointer to payload if header ok, NULL if header false
*******************************************/
unsigned int get_header_cmd(unsigned char *buf, int len) {
    packet_hdr *hdr;

    if(check_header(buf, len)) {
      hdr = (packet_hdr *)buf;
      return(hdr->cmd);
    }
    return 0; /* Failure: return an invalid command */
}


/*******************************************
Function create_header - fill the given buffer with the header, and return a pointer to the payload
  cmd       : Command for this package
  buf       : buffer to be processed
  size      : size of the buffer
returns: pointer to start of payload
*******************************************/
int create_header(int cmd, unsigned char *buf, int size) {
    packet_hdr * hdr;
 
    comms.fcs_checksum = 0xffff;    /* Initialise a fresh checksum */
    hdr = (packet_hdr *) buf;
    hdr->sync    = HDLC_SYNC;
    hdr->len    = 0;        /* Filled in later */
    hdr->chksum = 0;        /* Filled in later */
    hdr->cmd    = cmd;
    memcpy(&hdr->src_addr, &comms.src_addr.rc_bdaddr, sizeof(bdaddr_t));
    memcpy(&hdr->dst_addr, &comms.dst_addr.rc_bdaddr, sizeof(bdaddr_t));
    return PKT_OFF_DATASTART;
}


/*******************************************
Function read_bt - Read a buffer from BlueTooth
  buf       : buffer to be dumped in headecimal format
  buf_size  : size of this buffer
returns: Amount of bytes sent
*******************************************/
int read_bt(unsigned char *buf, int buf_size) {
    int bytes_read;
    bytes_read = read(comms.sock, buf, buf_size);
    dump_hex("reading", buf, bytes_read, 0);
    return bytes_read;
}

/*******************************************
Function write_bt - Write a message to bluetooth
  buf       : buffer to be dumped in headecimal format
  length    : length of the message
*******************************************/
int write_bt(unsigned char *buf, int length) {
  int written;
  written = write(comms.sock, buf, length);
  dump_hex("writing", buf, length, 0);
  return written;
}

/*******************************************
Function dump_hex - show a buffer in hexadecimal format, if the debug level is 2 or more
  logstr    : Description of the buffer being dumped
  buf       : buffer to be dumped in headecimal format
  length    : length of the message
  force     : force dumping, even if debug level is insufficient
*******************************************/
int dump_hex(char *logstr, unsigned char *buf, int length, int force) {
    int i;
    if( (force || g_debug > 1)  && length > 0 ) {
        printf( "\n%s: len=%d data=\n", logstr, length);
        for( i=0; i<length; i++ ) {
          if( i%16== 0 ) printf( "%s  %08x: ", (i>0 ? "\n" : ""), i);
          /* Make sure it is not reported as a negative value (starting with ffffff */
          printf( " %02x", buf[i]);
        }
        printf( "\n" );
    }
    return 0;
}



