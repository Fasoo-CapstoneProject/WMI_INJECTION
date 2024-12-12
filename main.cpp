#define _CRT_SECURE_NO_WARNINGS


#include <stdio.h>
#include <tchar.h>
#include <iostream>
#include <string>
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // 일부 CString 생성자는 명시적으로 선언됩니다.
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

        sprintf(szMSG, "[%s] 실행되었습니다!!", processNameStr.c_str());
        
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

        sprintf(szMSG, "[%s] 종료되었습니다!!", processNameStr.c_str());
   
        Log(szMSG);


    };

};



int main(int iArgCnt, char** argv)

{

    vector<string> processNames = { "notepad.exe", "EXCEL.EXE", "WINWORD.EXE", "POWERPNT.EXE", "gimp-2.10.exe" };

    // MonitorProc 객체 생성
    MyMonitor a;

    // 감시 시작
    int result = a.StartMonitoring(processNames);
    if (result != 0)
    {
        cerr << "Failed to start monitoring processes." << endl;
        return 1;
    }

    cout << "Monitoring processes. Press any key to exit..." << endl;
    cin.get();  // 사용자 입력을 기다림으로써 프로그램이 종료되지 않도록 함

    //std::cout << "Monitoring processes. Press any key to exit..." << std::endl;
    //std::cin.get();  // 사용자 입력을 기다림으로써 프로그램이 종료되지 않도록 함
    //
    //if (a.StartMonitoring("notepad.exe"))
    //
    //    printf("WMI 생성에실패했습니다.");
    //
    //getchar();

    return 0;

}
