/*
* Copyright (c) 2013-2017 Ardexa Pty Ltd
*
* This code is licensed under GPL v3
*
*/

#ifndef ARGUMENTS_HPP_INCLUDED
#define ARGUMENTS_HPP_INCLUDED

#include <string>
#include "arguments.hpp"
#include <iostream>
#include <map>
#include <unistd.h>
#include <vector>

using namespace std;

#define DEFAULT_DEBUG_VALUE 0
#define DEFAULT_LOG_DIRECTORY "/opt/ardexa/sma-bt/logs"
#define VERSION 1.19
#define PASSWORD "0000"

extern int g_debug;

/* This class is to call in the command line arguments */
class arguments
{
    public:
        /* methods are public */
        arguments();
        int initialize(int argc, char * argv[]);
        void usage();
        int get_debug();
        bool get_discovery();
        string get_log_directory();
        int get_number();
        string get_password();

        /* variables */
        map <string, string> convert;
        string address;


    private:
        /* members are private */
        int debug;
        string log_directory;
        bool discovery; /* this is to simple list the available devices and exit */
        string usage_string;
        int number;
        string password;
};

#endif /* ARGUMENTS_HPP_INCLUDED */
