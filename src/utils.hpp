/*
* Copyright (c) 2013-2017 Ardexa Pty Ltd
*
* This code is licensed under GPL v3
*
*/


#ifndef UTILS_HPP_INCLUDED
#define UTILS_HPP_INCLUDED

#include <string>
#include <stdlib.h>
#include <iostream>
#include <sys/stat.h>
#include <ctime>
#include <stdio.h>
#include <fstream>
#include <unistd.h>
#include <signal.h>
#include <algorithm>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#define DATESIZE 64
#define DOUBLE_SIZE 64
#define NAME 248
#define PID_FILE "/run/ardexa-sma-bt.pid"

extern int g_debug;

using namespace std;

int log_line(string directory, string filename, string line, string header, bool log_to_latest);
string get_current_date();
string get_current_datetime();
bool check_directory(string directory);
bool check_file(string file);
bool create_directory(string directory);
string convert_double(double number);
string replace_spaces(string incoming);
bool check_root();
bool check_pid_file();
void remove_pid_file();
string trim_whitespace(string raw_string);
bool convert_long(string incoming, long *outgoing);
bool get_bt_name(string bt_address, string *name_str);
string get_serial(string device_name);

#endif /* UTILS_HPP_INCLUDED */
