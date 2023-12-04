#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <sstream>
#include <string>
#include <algorithm>

#define ADDR_MAX 20

int largestram = 0;
int largestPID = 0;

HANDLE dHandle = NULL;


void ScanPatternInProcessMemory(HANDLE hProcess, const char* pattern, size_t patternSize, uintptr_t* addrarr)
{
    MEMORY_BASIC_INFORMATION memInfo;
    uintptr_t scanAddress = 0;
    int foundcounter = 0;


    while (VirtualQueryEx(hProcess, (LPVOID)scanAddress, &memInfo, sizeof(memInfo)) != 0)
    {
        if (memInfo.Protect != PAGE_NOACCESS && memInfo.State == MEM_COMMIT)
        {
            SIZE_T dummy;

            char* buffer = new char[memInfo.RegionSize + 1];

            if (ReadProcessMemory(dHandle, memInfo.BaseAddress, buffer, memInfo.RegionSize - 1, &dummy))
            {
                buffer[memInfo.RegionSize] = '\0';
                for (size_t i = 0; i < memInfo.RegionSize; i++)
                {
                    for (size_t u = 0; u < patternSize; u++)
                    {
                        if (buffer[u + i] != pattern[u])
                        {
                            break;
                        }

                        if (u == patternSize - 1)
                        {
                            //std::cout << "address: 0x" << std::hex << (uintptr_t)memInfo.BaseAddress + i << std::dec << '\n';

                            addrarr[foundcounter] = (uintptr_t)memInfo.BaseAddress + i;
                            foundcounter++;
                        }
                    }
                }

                //std::cout << "region size: 0x" << std::hex << (uintptr_t)memInfo.RegionSize << std::dec<< '\n';
            }

            buffer[memInfo.RegionSize] = '\0';

            delete[] buffer;
        }
        scanAddress += memInfo.RegionSize;
    }

    return;
}


int main()
{
    // grab the correct discord handle (the one that uses the most RAM)
    unsigned long long ramarr[6] = {}; // only 6 discord.exe s

    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (Process32First(snapshot, &entry) == TRUE)
    {
        int processcounter = 0;
        while (Process32Next(snapshot, &entry) == TRUE)
        {
            if (_stricmp(entry.szExeFile, "Discord.exe") == 0)
            {
                HANDLE currenthProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);

                PROCESS_MEMORY_COUNTERS buffer;
                GetProcessMemoryInfo(currenthProcess, &buffer, sizeof(PROCESS_MEMORY_COUNTERS));

                //std::cout << "PID: "<< entry.th32ProcessID << "         ram usage = ~" << (int)(buffer.WorkingSetSize / 1000) << "K\n";


                if (buffer.WorkingSetSize > largestram)
                {
                    largestram = buffer.WorkingSetSize;
                    largestPID = entry.th32ProcessID;
                }

                CloseHandle(currenthProcess);
                processcounter++;
                if (processcounter == 6)
                {
                    break;
                }
            }
        }
    }

    CloseHandle(snapshot);

    dHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, largestPID);

    // // // // // //

    for (;;)
    {
        std::string messagetochange;
        std::string changeto;
        SetConsoleTitleA("SubZero Discord Spoofer - Made By Payson");
        system("color 5");
        system("cls");
        std::cout << "\n SubZero Discord Spoofer - Made By Payson - discord.gg/subz\n\n";
        std::cout << " Input Message > ";
        std::getline(std::cin, messagetochange);
        std::replace_if(messagetochange.begin(), messagetochange.end(), [](char ch) {
            return ch == ' ';
            }, '|');

        std::cout << " Output message > ";
        std::getline(std::cin, changeto); // Read entire line
        std::replace_if(changeto.begin(), changeto.end(), [](char ch) {
            return ch == ' ';
            }, '|');



        // std::cin workaround
        for (size_t i = 0; i < messagetochange.length(); i++)
        {
            if (messagetochange[i] == ' |')
            {
                messagetochange[i] = ' ';
            }
        }
        for (size_t i = 0; i < changeto.length(); i++)
        {
            if (changeto[i] == ' |')
            {
                changeto[i] = ' ';
            }
        }


        uintptr_t* addrarr = new uintptr_t[ADDR_MAX];

        for (size_t i = 0; i < ADDR_MAX; i++)
        {
            addrarr[i] = 0;
        }

        const char* pattern = messagetochange.c_str();
        size_t patternsize = messagetochange.length();
        ScanPatternInProcessMemory(dHandle, pattern, patternsize, addrarr);

        std::string writestr = changeto;
        int spacesneeded = patternsize - writestr.length();

        for (size_t i = 0; i < spacesneeded; i++)
        {
            writestr += " ";
        }

        for (size_t i = 0; i < ADDR_MAX; i++)
        {
            std::cout << " Address [" << i << "]: 0x" << std::hex << addrarr[i] << std::dec << '\n';
            if (addrarr[i] != 0)
            {
                SIZE_T dummy;
                WriteProcessMemory(dHandle, (LPVOID)addrarr[i], writestr.c_str(), writestr.length(), &dummy);
            }
        }

        delete[] addrarr;
    }
}