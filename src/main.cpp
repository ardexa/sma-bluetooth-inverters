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
#include <unistd.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include "utils.hpp"
#include "arguments.hpp"
#include "in_bluetooth.h"
#include "in_smadata2plus.h"


using namespace std;

/* Global variables. */ 
int g_debug = DEFAULT_DEBUG_VALUE;

/* Prototypes */
void process_data(vector <vec_data> data_vector, int debug, string& line, string& header);


/* Main function */
int main(int argc, char **argv) {
	int result;
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


	/* this for loop will iterate through each address in the vector of BT addresses */
	for(unsigned int i = 0; i <= arguments_list.addresses.size() - 1; i++) {
		string line, header;
		vector <vec_data> data_vector;

		if (g_debug >= 1) cout << "\nProcessing BT address: " << arguments_list.addresses[i] << endl;
		/* Get the address of the devices were are interested in */
		get_bt_name(arguments_list.addresses[i], &device_name);

		/* Get the serial number out of the device name */
		serial_number = get_serial(device_name);
		if (g_debug >= 1) cout << "Serial number: " << serial_number << endl;


		/* Inizialize Bluetooth Inverter */
		struct bluetooth_inverter inv = { { 0 } };
		strcpy(inv.macaddr, arguments_list.addresses[i].c_str());   /// Change to strncpy
		memcpy(inv.password, "0000", 5);
		in_bluetooth_connect(&inv);
		in_smadata2plus_connect(&inv);
		in_smadata2plus_login(&inv);
		in_smadata2plus_get_values(&inv, data_vector);
		close(inv.socket_fd);

		process_data(data_vector, g_debug, line, header);
		if (g_debug >= 1) {
			cout << header << endl;
			cout << line << endl;
		}		
		

		string filename = get_current_date() + ".csv";
		/* Log the line based on the inverter serial number, in the logging directory */
		string full_dir = arguments_list.get_log_directory() + "/" + serial_number;
		/* log to a date and to a 'latest' file */
		log_line(full_dir, filename, line, header, true);

	}


	/* stop the timer */
	time_t end = time(nullptr);
	if (g_debug > 0) cout << "Query took: " << (end-start) << " Seconds\n" << endl;
	/* remove the PID file */
	remove_pid_file();

   return 0;
}


/* This function processes the data that is in a vector of structs */
void process_data(vector <vec_data> data_vector, int debug, string& line, string& header)
{
	string pac_header, yield_header, pdc1_header, pdc2_header, vdc1_header, vdc2_header, vac1_header, vac2_header, vac3_header, iac1_header, iac2_header, iac3_header;
	string pac_str, yield_str, pdc1_str, pdc2_str, vdc1_str, vdc2_str, vac1_str, vac2_str, vac3_str, iac1_str, iac2_str, iac3_str;
	stringstream stream_vac1, stream_vac2, stream_vac3, stream_iac1, stream_iac2, stream_iac3, stream_vdc1, stream_vdc2, stream_pdc1, stream_pdc2, stream_pac, stream_yield;


	for (auto iter = data_vector.begin(); iter != data_vector.end(); ++iter) {
		float value = iter->value;
		string name = iter->name;

		/* If any value (except yield) is greater than 167772, it means it is max as FFFF. Set to 0
			But only for voltage or current */
		if (name == "power_ac") {
			pac_header = "AC Power(" + iter->units + ")";
			stream_pac << fixed << setprecision(1) << value;
			pac_str = stream_pac.str();
		}

		if (name == "yield_total") { 
			yield_header = "Yield(" + iter->units + ")";
			stream_yield << fixed << setprecision(1) << value;
			yield_str = stream_yield.str();
		}

		if (name == "power_dc_1") {
			pdc1_header = "DC Power 1(" + iter->units + ")";
			stream_pdc1 << fixed << setprecision(1) << value;
			pdc1_str = stream_pdc1.str();
		}

		if (name == "power_dc_2") {
			pdc2_header = "DC Power 2(" + iter->units + ")";
			stream_pdc2 << fixed << setprecision(1) << value;
			pdc2_str = stream_pdc2.str();
		}

		if (name == "voltage_dc_1") {
			if (value > 16700.0) value = 0.0;
			vdc1_header = "DC Voltage 1(" + iter->units + ")";
			stream_vdc1 << fixed << setprecision(1) << value;
			vdc1_str = stream_vdc1.str();
		}

		if (name == "voltage_dc_2") {
			if (value > 16700.0) value = 0.0;
			vdc2_header = "DC Voltage 2(" + iter->units + ")";
			stream_vdc2 << fixed << setprecision(1) << value;
			vdc2_str = stream_vdc2.str();
		}

		if (name == "voltage_ac_l1") {
			if (value > 16700.0) value = 0.0;
			vac1_header = "AC Voltage 1(" + iter->units + ")";
			stream_vac1 << fixed << setprecision(1) << value;
			vac1_str = stream_vac1.str();
		}

		if (name == "voltage_ac_l2") {
			if (value > 16700.0) value = 0.0;
			vac2_header = "AC Voltage 2(" + iter->units + ")";
			stream_vac2 << fixed << setprecision(1) << value;
			vac2_str = stream_vac2.str();
		}

		if (name == "voltage_ac_l3") {
			if (value > 16700.0) value = 0.0;
			vac3_header = "AC Voltage 3(" + iter->units + ")";
			stream_vac3 << fixed << setprecision(1) << value;
			vac3_str = stream_vac3.str();
		}

		if (name == "current_ac_l1") {
			if (value > 16700.0) value = 0.0;
			iac1_header = "AC Current 1(" + iter->units + ")";
			stream_iac1 << fixed << setprecision(2) << value;
			iac1_str = stream_iac1.str();
		}

		if (name == "current_ac_l2") {
			if (value > 16700.0) value = 0.0;
			iac2_header = "AC Current 2(" + iter->units + ")";
			stream_iac2 << fixed << setprecision(2) << value;
			iac2_str = stream_iac2.str();
		}

		if (name == "current_ac_l3") {
			if (value > 16700.0) value = 0.0;
			iac3_header = "AC Current 3(" + iter->units + ")";
			stream_iac3 << fixed << setprecision(2) << value;
			iac3_str = stream_iac3.str();
		}


		string datetime = get_current_datetime();
		header = "#Datetime," + pac_header + "," + yield_header + "," + pdc1_header  + "," + pdc2_header + "," + vdc1_header + "," + 
					vdc2_header + "," + vac1_header + "," + vac2_header + "," + vac3_header + "," + iac1_header + "," + iac2_header + "," + iac3_header;

		line = datetime + "," + pac_str + "," + yield_str + "," + pdc1_str + "," + pdc2_str + "," + vdc1_str + "," + vdc2_str + "," + 
				 vac1_str + "," + vac2_str + "," + vac3_str + "," + iac1_str + "," + iac2_str + "," + iac3_str;


		if (debug >= 1) {
			cout << iter->name << " = " << value << " (" << iter->units << ") " << endl;
		}

   }

	return;
}


