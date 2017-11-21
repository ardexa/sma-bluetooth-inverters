/*
 * Copyright (c) 2013-2017 Ardexa Pty Ltd
 *
 * This code is licensed under GPL v3
 *
 */

#include "arguments.hpp"
#include "utils.hpp"

using namespace std;

/* Constructor for the arguments class */
arguments::arguments()
{
    /* Initialize the private members */
    this->debug = DEFAULT_DEBUG_VALUE;
    this->log_directory = DEFAULT_LOG_DIRECTORY;
    this->discovery = false;
    this->number = 0;
    this->password = "0000";

    /* Usage string */
    this->usage_string = "Usage: ardexa-sma-bt -b mac_addr file path [-p password -l log directory] [-d] [-v] [-i]\n";
}

/* This method is to initialize the member variables based on the command line arguments */
int arguments::initialize(int argc, char * argv[])
{
    int opt;
    bool ret_error = false;
    string debug_raw = "0";

    /*
     * -l (optional) <directory> name for the location of the directory in which the logs will be written
     * -b (mandatory) <mac_addr> the MAC address of the bluetooth inverter
     * -d (optional) if specified, debug will be turned on
     * -i (optional) discovery. Print a listing of all available SMA Bluetooth inverters
     * -v (optional) prints the version and exits
     * -p (optional) sets the inverter password
     */
    while ((opt = getopt(argc, argv, "l:b:d:iv")) != -1) {
        switch (opt) {
            case 'b':
                this->address = string(optarg);
                break;
            case 'l':
                /* There is no need to verify the logging directory. If it doesn't exist, it will be created later */
                this->log_directory = optarg;
                break;
            case 'p':
                this->password = optarg;
                break;
            case 'd':
                debug_raw = optarg;
                break;
            case 'i':
                this->discovery = true;
                break;
            case 'v':
                cout << "Ardexa SMA Bluetooth Version: " << VERSION << endl;
                exit(0);
            default:
                this->usage();
                return 1;
        }
    }

    /* Convert "debug_raw" to INT */
    size_t idx;
    try {
        this->debug = stoi(debug_raw,&idx,10);
    }
    catch ( const std::exception& e ) {
        ret_error = true;
    }

    if ((this->debug < 0) or (this->debug >= 5)) {
        cout << "Debug level must be a number, and be greater than -1 and less than 5 " << endl;
        ret_error = true;
    }

    g_debug = this->debug;

    if (not create_directory(this->log_directory)) {
        cout << "Could not create the logging directory: " << this->log_directory << endl;
        ret_error = true;
    }

    /* check existance of address */
    if (this->address == "") {
        cout << "Must supply a valid MAC address to connect to" << endl;
        ret_error = true;
    }

    /* If any errors, then return as an error */
    if (ret_error) {
        this->usage();
        return 1;
    }
    else {
        return 0;
    }
}


/* Print usage string */
void arguments::usage()
{
    cout << usage_string;
}

/* Get the logging directory */
string arguments::get_log_directory()
{
    return this->log_directory;
}

/* Get the discovery bool value */
bool arguments::get_discovery()
{
    return this->discovery;
}

/* Get the number of devices */
int arguments::get_number()
{
    return this->number;
}

/* Get the debug bool value */
int arguments::get_debug()
{
    return this->debug;
}

/* return the password */
string arguments::get_password()
{
    return this->password;
}
