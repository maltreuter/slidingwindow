#include <iostream>
#include <algorithm>
#include <bitset>

using namespace std;

string createChecksum(string data, int blockSize) {
    int dataLength = data.length();
    if (dataLength % blockSize != 0) {
        int paddingSize = blockSize - (dataLength % blockSize);
        for (int i = 0; i < paddingSize; i++) {
            data = '0' + data;
        }
    }

    string binaryString = "";
    for (int i = 0; i < blockSize; i++) {
        binaryString = binaryString + data[i];
    }
    string newBinary = "";
    for (char &_char : binaryString) {
        newBinary += bitset<8>(_char).to_string();
    }
    cout << "\nNewBinary: " << newBinary;

    for (int i = blockSize; i < dataLength; i = i + blockSize) {
        string next = "";
        for (int j = i; j < i + blockSize; j++) {
            next = next + data[j];
        }
        string nextBlock = "";
        for (char &_char : next) {
            nextBlock += bitset<8>(_char).to_string();
        }

        cout << "\nnextBlock: " << nextBlock;

        string binaryAddition = "";
        int currentSum = 0;
        int currentCarry = 0;

        for (int n = blockSize - 1; n >= 0; n--) {
            currentSum = currentSum + (nextBlock[n] - '0') + (newBinary[n] - '0');
            currentCarry = currentSum / 2;
            if (currentSum == 0 || currentSum == 2) {
                binaryAddition = '0' + binaryAddition;
                currentSum = currentCarry;
            } else {
                binaryAddition = '1' + binaryAddition;
                currentSum = currentCarry;
            }
        }

        string finalAddition = "";
        if (currentCarry == 1) {
            for (int k = binaryAddition.length() - 1; k >= 0; k--) {
                if (currentCarry == 0) {
                    finalAddition = binaryAddition[k] + finalAddition;
                } else if (((binaryAddition[k] - '0') + currentCarry) % 2 == 0) {
                    finalAddition = "0" + finalAddition;
                    currentCarry = 1;
                } else {
                    finalAddition = "1" + finalAddition;
                    currentCarry = 0;
                }
            }
            binaryString = finalAddition;
        } else {
            binaryString = binaryAddition;
        }
    }
    string checksum = binaryString;
    for (int i = 0; i < binaryString.length(); i++) {
        if (binaryString[i] == '0') {
            checksum[i] = '1';
        } else {
            checksum[i] = '0';
        }
    }
    return checksum;
}

bool checkChecksum(string checksum, string data, int blockSize) {
    int dataLength = data.length();
    int currentSum = 0;
    int currentCarry = 0;
    string recvSum = "";
    if (dataLength % blockSize != 0) {
        int paddingSize = blockSize - (dataLength % blockSize);
        for (int i = 0; i < paddingSize; i++) {
            data = '0' + data;
        }
    }

    string binaryString = "";
    for (int i = 0; i < blockSize; i++) {
        binaryString = binaryString + data[i];
    }
    string newBinary = "";
    for (char &_char : binaryString) {
        newBinary += bitset<8>(_char).to_string();
    }

    for (int i = blockSize; i < dataLength; i = i + blockSize) {
        string next = "";
        for (int j = i; j < i + blockSize; j++) {
            next = next + data[j];
        }
        string nextBlock = "";
        for (char &_char : next) {
            nextBlock += bitset<8>(_char).to_string();
        }

        string binaryAddition = "";
        int currentSum = 0;
        int currentCarry = 0;

        for (int n = blockSize - 1; n >= 0; n--) {
            currentSum = currentSum + (nextBlock[n] - '0') + (newBinary[n] - '0');
            currentCarry = currentSum / 2;
            if (currentSum == 0 || currentSum == 2) {
                binaryAddition = '0' + binaryAddition;
                currentSum = currentCarry;
            } else {
                binaryAddition = '1' + binaryAddition;
                currentSum = currentCarry;
            }
        }

        string finalAddition = "";
        if (currentCarry == 1) {
            for (int k = binaryAddition.length() - 1; k >= 0; k--) {
                if (currentCarry == 0) {
                    finalAddition = binaryAddition[k] + finalAddition;
                } else if (((binaryAddition[k] - '0') + currentCarry) % 2 == 0) {
                    finalAddition = "0" + finalAddition;
                    currentCarry = 1;
                } else {
                    finalAddition = "1" + finalAddition;
                    currentCarry = 0;
                }
            }
            binaryString = finalAddition;
        } else {
            binaryString = binaryAddition;
        }
    }

    cout << "\nChecksum: " << checksum;
    cout << "\nRecvString: " << binaryString;

    for (int n = blockSize - 1; n >= 0; n--) {
        currentSum = currentSum + (checksum[n] - '0') + (binaryString[n] - '0');
        currentCarry = currentSum / 2;
        if (currentSum == 0 || currentSum == 2) {
            recvSum = '0' + recvSum;
            currentSum = currentCarry;
        } else {
            recvSum = '1' + recvSum;
            currentSum = currentCarry;
        }
    }
    cout << "\n" << recvSum;
    if (count(recvSum.begin(), recvSum.end(), '1') == blockSize) {
        return true;
    } else {
        return false;
    }
};

int main() {
    string recieved = "abcdefghijklmnop";
    int blockSize = 8;
    FILE *file = fopen(R"(C:\Users\laura\CLionProjects\checksumCode\test.txt)", "r");
    int c;
    string sent = "";
    char buffer;
    if(file){
        while(1){
            c = fgetc(file);
            if(feof(file)) {
                break;
            }
            buffer = (char) c;
            sent.push_back(buffer);
        }
        fclose(file);
    }
    string checksum = createChecksum(sent, blockSize);

    if (checkChecksum(checksum, recieved, blockSize)) {
        cout << "\nNo Error";
    } else {
        cout << "\nError";
    }
    return 0;
}
