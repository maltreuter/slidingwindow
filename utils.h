#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <chrono>
#include <stdlib.h>
#include <cstring>

#define BUF_SIZE 1024

using namespace std;

int get_current_time();
string get_md5(string file_path);
string uchar_to_binary(unsigned char c);

#endif
