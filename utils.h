#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <chrono>
#include <stdlib.h>
#include <cstring>
#include <vector>
#include <sstream>

#define BUF_SIZE 1024

using namespace std;

int get_current_time();
string get_md5(string file_path);
string uchar_to_binary(unsigned char c);
string vector_to_string(vector<int> packets);
vector<int> string_to_vector(string packets);

#endif
