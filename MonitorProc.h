#pragma once

#include "eventsink.h"
#include "interface.h"
#include <vector>

using namespace std;

class MonitorProc : public ProcessNotify
{

public:
    MonitorProc();
    ~MonitorProc();
    int StartMonitoring(const vector<string>& processNames);

private:
    HRESULT InitializeCOM();
    HRESULT InitializeSecurity();
    HRESULT CreateLocator();
    HRESULT ConnectToWMI();
    HRESULT SetProxyBlanket();
    HRESULT MonitorProcess(const string& processName, EventSink::state eventType);
    void Cleanup();

    IWbemLocator* pLoc;
    IWbemServices* pSvc;
    IUnsecuredApartment* pUnsecApp;
};