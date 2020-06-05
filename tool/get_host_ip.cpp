#include <iostream>
#include <sstream>

#include <arpa/inet.h>
//#include <sys/socket.h>
//#include <netdb.h>
#include <ifaddrs.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

using namespace std;

#define PORT    111

template<typename T1, typename T2>
inline T1 lexical_cast(const T2& t)
{
    stringstream ss;

    ss << t;
    T1 tReturn;

    ss >> tReturn;

    return tReturn;
}


inline const string getHostIp(const string& sKey = "")
{
    ifaddrs* ifAddrStruct = NULL;
    int err = getifaddrs(&ifAddrStruct);
    if (err != 0)
    {
        cout << "getifaddrs error." << endl;
        return "";
    }

    // freeÊ±ºòÊ¹ÓÃ
    ifaddrs* ifAddrStruct1 = ifAddrStruct;

    while (ifAddrStruct != NULL)
    {
        sockaddr* addr = ifAddrStruct->ifa_addr;
        if (addr == NULL)
        {
            ifAddrStruct = ifAddrStruct->ifa_next;
            continue;
        }

        if (addr->sa_family != AF_INET)
        {
            ifAddrStruct = ifAddrStruct->ifa_next;
            continue;
        }

        // check it is IP4
        // is a valid IP4 Address
        void* tmpAddrPtr = &((struct sockaddr_in*)addr)->sin_addr;
        if (tmpAddrPtr == NULL)
        {
            ifAddrStruct = ifAddrStruct->ifa_next;
            continue;
        }

        char addressBuffer[INET_ADDRSTRLEN] = "";
        inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
        if (strcmp(addressBuffer, "127.0.0.1") != 0)
        {
            if (sKey.empty())
            {
                freeifaddrs(ifAddrStruct1);
                return addressBuffer;
            }
            else
            {
                string sIfaName = ifAddrStruct->ifa_name;
                if (sIfaName.find(sKey) != string::npos)
                {
                    freeifaddrs(ifAddrStruct1);
                    return addressBuffer;
                }
            }
        }

        ifAddrStruct = ifAddrStruct->ifa_next;
    }

    freeifaddrs(ifAddrStruct1);
    return "";
}

int main() 
{
    string sIp = getHostIp();
    printf("this ip is: %s\n", sIp.c_str());

    //string host = "http://" + sIp + ":" + lexical_cast<string>(PORT) + "/";
    //printf("host is: %s\n", host.c_str());
}