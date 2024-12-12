#define _CRT_SECURE_NO_WARNINGS


#include <stdio.h>
#include <tchar.h>
#include <iostream>
#include <string>
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // �Ϻ� CString �����ڴ� ��������� ����˴ϴ�.
#include "eventsink.h"
#include "MonitorProc.h"
#include "inject.h"
#include <vector>
#include <thread>
#include <codecvt> // for wstring_convert
#include <locale>  // for codecvt_utf8
#include "stringapiset.h"
#define DLL_PATH ("C:\\ClipLock_DLL.dll")

using namespace std;



class MyMonitor : public MonitorProc

{

    void Log(char* sMsg)
    {
        SYSTEMTIME LocalTime;
        GetLocalTime(&LocalTime);

        char szDate[1024];
        ZeroMemory(szDate, 1024);

        sprintf(szDate, "[%04d-%02d-%02d %02d:%02d:%02d]",
            LocalTime.wYear, LocalTime.wMonth,
            LocalTime.wDay, LocalTime.wHour,
            LocalTime.wMinute, LocalTime.wSecond);

        printf("%s %s\n", szDate, sMsg);

    }

    void Create(const wstring& processName)

    {

        char szMSG[1024];

        ZeroMemory(szMSG, 1024);

        wstring_convert<codecvt_utf8<wchar_t>> converter;
        string processNameStr = converter.to_bytes(processName);

        sprintf(szMSG, "[%s] ����Ǿ����ϴ�!!", processNameStr.c_str());
        
        Log(szMSG);

        DWORD pid = get_process_id(processNameStr.c_str());
        printf("pid : %d\n", pid);

        InjectDll(pid, DLL_PATH);

    };



    void Delete(const wstring& processName)

    {

        char szMSG[1024];

        ZeroMemory(szMSG, 1024);

        wstring_convert<codecvt_utf8<wchar_t>> converter;
        string processNameStr = converter.to_bytes(processName);

        sprintf(szMSG, "[%s] ����Ǿ����ϴ�!!", processNameStr.c_str());
   
        Log(szMSG);


    };

};



int main(int iArgCnt, char** argv)

{

    vector<string> processNames = { "notepad.exe", "EXCEL.EXE", "WINWORD.EXE", "POWERPNT.EXE", "gimp-2.10.exe" };

    // MonitorProc ��ü ����
    MyMonitor a;

    // ���� ����
    int result = a.StartMonitoring(processNames);
    if (result != 0)
    {
        cerr << "Failed to start monitoring processes." << endl;
        return 1;
    }

    cout << "Monitoring processes. Press any key to exit..." << endl;
    cin.get();  // ����� �Է��� ��ٸ����ν� ���α׷��� ������� �ʵ��� ��

    //std::cout << "Monitoring processes. Press any key to exit..." << std::endl;
    //std::cin.get();  // ����� �Է��� ��ٸ����ν� ���α׷��� ������� �ʵ��� ��
    //
    //if (a.StartMonitoring("notepad.exe"))
    //
    //    printf("WMI �����������߽��ϴ�.");
    //
    //getchar();

    return 0;

}
