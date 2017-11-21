/*
 * Copyright (c) 2013-2017 Ardexa Pty Ltd
 *
 * This code is licensed under GPL v3
 *
 */

#include "utils.hpp"

/* Open the file where the log entry will be written, and write the line to it
   When using this function, make sure 'line' and 'header' have a newline at
   end If the 'rotate' is true, it will move thew old file and file instead of
   appending If the 'log_to_latest' it will also log a line to 'latest.csv' in
   'directory'
   */
int log_line(string directory, string filename, string line, string header, bool log_to_latest)
{
    struct stat st_directory;
    string fullpath;
    bool write_header = false;
    /* if the log directory does not exist, or the log file does not exist, then declare a 'rotation'.
       This is where the 'latest.csv' file is renamed and a new one created */
    bool rotate = false;

    /* Add an ending '/' to the directory path, if it doesn't exist */
    if (*directory.rbegin() != '/') {
        directory += "/";
    }

    /* Check and create the directory if necessary */
    if (stat(directory.c_str(), &st_directory) == -1) {
        if (g_debug > 0) cout << "Directory doesn't exist. Creating it: " << directory.c_str() << endl;
        rotate = true;
        bool result = create_directory(directory);
        if (!result) {
            return 2;
        }
    }

    fullpath = directory + filename;
    if (g_debug > 1) cout << "Full filename: " << fullpath << endl;

    /* Check the full path. If it doesn't exist, the header line will need to
     * be written If the file DOES exist AND if a rotation is called, then
     * rename it and annotate a header is required And the rotate will need to
     * be set to true
     */
    if (stat(fullpath.c_str(), &st_directory) == -1) {
        if (g_debug > 1) cout << "Fullpath doesn't exist. Path: " << fullpath.c_str() << endl;
        write_header = true;
        rotate = true;
    }

    /* Open it for appending data only */
    ofstream writer(fullpath.c_str(), ios::app);
    if(!writer) {
        if (g_debug > 0) cout << "Cannot open logging file: " << fullpath << endl;
        return 2;
    }
    if (write_header) {
        writer << header << endl;
    }

    writer << line << endl;
    writer.close();

    write_header = false;
    /* If 'log_to_latest' is set, then write the line to this file as well */
    if (log_to_latest) {
        /* if file exists and rotate is declared, rename it and create a new one */
        fullpath = directory + "latest.csv";
        if (rotate) {
            string newpath = directory + "latest.csv.OLD";
            rename(fullpath.c_str(), newpath.c_str());
            write_header = true;
        }

        /* Open it for appending data only */
        ofstream latest(fullpath.c_str(), ios::app);
        if(!latest) {
            if (g_debug > 0) cout << "Cannot open logging file: " << fullpath << endl;
            return 3;
        }
        if (write_header) {
            latest << header << endl;
        }
        latest << line << endl;

        /* close it */
        latest.close();
    }

    return 0;
}

/* Returns the current date as a string in the format "2017-01-30" */
string get_current_date()
{
    time_t rawtime;
    struct tm * timeinfo;
    char buffer[DATESIZE];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer, sizeof(buffer), "%Y-%m-%d", timeinfo);

    string date(buffer);
    return date;
}

/* Returns the current time as a string, in the format "2017-01-30T15:30:45" */
string get_current_datetime()
{
    time_t rawtime;
    struct tm * timeinfo;
    char buffer[DATESIZE];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    /* This includes the time zone at the end of the time */
    strftime(buffer, DATESIZE, "%H:%M:%S", timeinfo);

    string time(buffer);

    string datetime = get_current_date() + "T" + time;

    return datetime;
}

/* Check if a direetory exists */
bool check_directory(string directory)
{
    struct stat sb;

    if (stat(directory.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode)) {
        return true;
    }
    else {
        return false;
    }

}

/* check if a file exists */
bool check_file(string file)
{
    struct stat sb;

    if (stat(file.c_str(), &sb) == 0 && S_ISREG(sb.st_mode)) {
        return true;
    }
    else {
        return false;
    }

}

/* Create a directory, including all parent paths if they don't exist */
bool create_directory(string directory)
{
    string delimiter = "/";
    size_t pos = 0, start = 0;
    int count = 0;
    string temp = "";

    /* Add a trailing '/' */
    directory = directory + '/';
    while ((pos = directory.find(delimiter, start)) != string::npos) {
        if (count == 0) {
            if (pos != 0) {
                cout << "Directory needs to start with " << delimiter << endl;
                return false;
            }
        }
        else {
            temp = directory.substr(0, pos);
            if (check_directory(temp)) {
                if (g_debug > 1) cout << "The dir: " << temp << " exists." << endl;

            }
            else {
                if (g_debug > 0) cout << "Creating the dir: " << temp << endl;
                if (mkdir(temp.c_str(), 0744) != 0) {
                    cout << "Could not create the directory: " << temp << endl;
                    return false;
                }
            }
        }
        count++;
        /* start at one past the las pos */
        start = pos + 1;
    }

    return true;
}

/* This function will convert a double number to a string */
string convert_double(double number)
{
    char buffer[DOUBLE_SIZE] = "";
    sprintf(buffer, "%.2f", number);
    string converted(buffer);
    /* remove last 3 characters if they exactly equal '.00' */
    if (converted.compare(converted.size() - 3, 3,".00") == 0) {
        converted.erase(converted.size() - 3, 3);
    }
    return converted;

}

/* replace all spaces in a string with an underscore */
string replace_spaces(string incoming)
{
    string outgoing = incoming;
    /* find first space */
    unsigned int position = outgoing.find(" ");

    while( position != string::npos )  {
        outgoing.replace( position, 1, "_" );
        position = outgoing.find(" ", position + 1 );
    }
    return outgoing;
}

bool check_root()
{
    uid_t uid=getuid();
    if (uid == 0) {
        return true;
    }
    else {
        return false;
    }
}

/*  This function checks for the existence of a PID file. If one is
    found, it checks if the process is alive and exits if so. Otherwise,
    it will (re)write the PID file. */
bool check_pid_file()
{
    /* Check to see if there is a pid file */
    bool pid_file_exists = true;
    struct stat pid_status;
    string pid_file = PID_FILE;
    string pid_str, pid_raw;
    long pid_long;

    /* if the file exists, then read in the pid */
    if (stat(pid_file.c_str(), &pid_status) == 0) {
        fstream read_pid(pid_file, ios::in);
        read_pid >> pid_raw;
        read_pid.close();
        /* trim leading and trailing whitespace */
        pid_str = trim_whitespace(pid_raw);

        if (!(convert_long(pid_str, &pid_long))) {
            cout << "The ardexa-sma application appears to be already running..." << endl;
            return false;
        }
    }
    else {
        /* Flag that the pid file doesn't exist */
        pid_file_exists = false;
    }

    /* Check that the file exists and pid is alive. Use a sig of 0 */
    if ((pid_file_exists) && (kill((pid_t) pid_long, 0) == 0)) {
        cout << "The ardexa-sma application appears to be already running..." << endl;
        return false;
    }

    /*  If routine gets to here, then the file exists and the pid is dead, or the pid file doesn't exist.
        Either condition is treated the same */

    ofstream write_pid(pid_file, ios::trunc);
    write_pid << getpid() << endl;
    write_pid.close();

    return true;
}

/* remove a PID file */
void remove_pid_file()
{
    struct stat pid_status;
    string pid_file = PID_FILE;

    /* if the file exists, then remove it */
    if (stat(pid_file.c_str(), &pid_status) == 0) {
        remove(pid_file.c_str());
    }
}

/* trim whitespace from the start and end of a string */
string trim_whitespace(string raw_string) {

    raw_string.erase(raw_string.begin(), find_if(raw_string.begin(), raw_string.end(), not1(ptr_fun<int, int>(isspace))));
    raw_string.erase(find_if(raw_string.rbegin(), raw_string.rend(), not1(ptr_fun<int, int>(isspace))).base(), raw_string.end());

    return raw_string;
}

/* Convert a string to a long */
bool convert_long(string incoming, long *outgoing)
{
    size_t idx;
    bool stol_success = true;
    *outgoing = 0;

    try {
        *outgoing = stol(incoming,&idx,10);
    }
    catch ( const std::exception& e ) {
        stol_success = false;
    }


    if (stol_success){
        if (idx == incoming.length()) {
            return true;
        }
        else {
            return false;
        }
    }
    else {
        return false;
    }

}


/* Get the name of a bluetooth device. If bt_address is empty, print all available devices  */
bool get_bt_name(string bt_address, string *name_str)
{
    inquiry_info *ii = NULL;
    int max_rsp, num_rsp;
    int dev_id, sock, len, flags;
    char addr[NAME] = {0};
    char name[NAME] = {0};
    bdaddr_t	dst_addr;

    /* retrieve ID of BT adapter, and open the device */
    dev_id = hci_get_route(NULL);

    sock = hci_open_dev( dev_id );
    if (dev_id < 0 || sock < 0) {
        if (g_debug > 0) cout << "Could not find a bluetooth adapter " << endl;
        return false;
    }

    /* if bt_address is empty, search and print all devices */
    if (bt_address.empty()) {
        len  = 8;
        max_rsp = NAME;
        flags = IREQ_CACHE_FLUSH;
        ii = (inquiry_info*)malloc(max_rsp * sizeof(inquiry_info));

        num_rsp = hci_inquiry(dev_id, len, max_rsp, NULL, &ii, flags);
        if( num_rsp < 0 ) {
            if (g_debug > 0) cout << "Could not find a bluetooth hosts on the network " << endl;
            close( sock );
            return false;
        }

        for (int i = 0; i < num_rsp; i++) {
            memset(addr, 0, sizeof(addr));
            ba2str(&(ii+i)->bdaddr, addr);
            memset(name, 0, sizeof(name));
            if (hci_read_remote_name(sock, &(ii+i)->bdaddr, sizeof(name), name, 0) < 0)  strncpy(name, "[unknown]", sizeof(name) -1);
            cout << "Found a device at address: " << addr << " and name: " << name << endl;
        }
    }
    /* if bt_address is NOT empty, query address for name, and copy it to name_str */
    else {
        str2ba(bt_address.c_str(), &dst_addr );
        if(hci_read_remote_name(sock, &dst_addr, sizeof(name), name, 0) < 0) {
            *name_str = "[unknown]";
        }
        else {
            *name_str = name;
        }
    }

    free( ii );
    close( sock );
    return true;
}

/* Get the SMA serial number out of the device name */
string get_serial(string device_name)
{
    string serial = "";

    /* Look for the character 'SN' */
    size_t found = device_name.rfind("SN");
    if (found != string::npos) {
        /* advance two positions */
        found++; found++;
        serial = device_name.substr(found, string::npos);
    }

    return serial;
}
