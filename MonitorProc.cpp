#define _CRT_SECURE_NO_WARNINGS


#include <stdio.h>
#include <tchar.h>
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // 일부 CString 생성자는 명시적으로 선언됩니다.
#include "MonitorProc.h"
#include "eventsink.h"

using namespace std;


MonitorProc::MonitorProc()
    : pLoc(nullptr), pSvc(nullptr), pUnsecApp(nullptr)
{
}


MonitorProc::~MonitorProc()
{
    Cleanup();
}

// COM 라이브러리를 초기화합니다. 멀티 스레딩 환경에서 초기화합니다.
HRESULT MonitorProc::InitializeCOM()
{
    return CoInitializeEx(0, COINIT_MULTITHREADED);
}


// COM 보안 설정을 초기화합니다.
// 기본 인증 수준 및 IMPERSONATE 권한 수준을 설정합니다.
HRESULT MonitorProc::InitializeSecurity()
{
    return CoInitializeSecurity(
        NULL,
        -1,
        NULL,
        NULL,
        RPC_C_AUTHN_LEVEL_DEFAULT,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        EOAC_NONE,
        NULL
    );
}


// WbemLocator 객체를 생성합니다.
HRESULT MonitorProc::CreateLocator()
{
    return CoCreateInstance(
        CLSID_WbemLocator,
        0,
        CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, (LPVOID*)&pLoc);
}


// WMI 서버에 연결합니다. ROOT\CIMV2 네임스페이스에 연결합니다.
HRESULT MonitorProc::ConnectToWMI()
{
    return pLoc->ConnectServer(
        _bstr_t(L"ROOT\\CIMV2"),
        NULL,
        NULL,
        0,
        NULL,
        0,
        0,
        &pSvc);
}


// WMI 서비스 호출에 대한 보안 설정을 구성합니다.
HRESULT MonitorProc::SetProxyBlanket()
{
    return CoSetProxyBlanket(
        pSvc,
        RPC_C_AUTHN_WINNT,
        RPC_C_AUTHZ_NONE,
        NULL,
        RPC_C_AUTHN_LEVEL_CALL,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        EOAC_NONE);
}


// 이벤트를 처리할 EventSink 객체를 생성하고 등록합니다.
HRESULT MonitorProc::MonitorProcess(const string& processName, EventSink::state eventType)
{
    EventSink* pSink = new EventSink(this, eventType);
    pSink->AddRef();

    IUnknown* pStubUnk = NULL;
    pUnsecApp->CreateObjectStub(pSink, &pStubUnk);

    IWbemObjectSink* pStubSink = NULL;
    pStubUnk->QueryInterface(IID_IWbemObjectSink, (void**)&pStubSink);

    // 쿼리 문자열을 생성하여 이벤트 모니터링을 위한 비동기 쿼리를 실행합니다.
    string query = "SELECT * FROM " +
        string(eventType == EventSink::CRE ? "__InstanceCreationEvent" : "__InstanceDeletionEvent") +
        " WITHIN 1 WHERE TargetInstance ISA 'Win32_Process' and TargetInstance.Name = '" + processName + "'";

    HRESULT hres = pSvc->ExecNotificationQueryAsync(
        _bstr_t("WQL"),
        _bstr_t(query.c_str()),
        WBEM_FLAG_SEND_STATUS,
        NULL,
        pStubSink);

    if (FAILED(hres))
    {
        // 쿼리 실행 실패 시 리소스를 해제합니다.
        pStubSink->Release();
        pStubUnk->Release();
        pSink->Release();
    }

    return hres;
}

void MonitorProc::Cleanup()
{
    if (pSvc) pSvc->Release();
    if (pLoc) pLoc->Release();
    if (pUnsecApp) pUnsecApp->Release();
    CoUninitialize();
}

int MonitorProc::StartMonitoring(const vector<string>& processNames)
{
    HRESULT hres;

    hres = InitializeCOM();
    if (FAILED(hres)) return 1;

    hres = InitializeSecurity();
    if (FAILED(hres)) {
        CoUninitialize();
        return 1;
    }

    hres = CreateLocator();
    if (FAILED(hres)) {
        CoUninitialize();
        return 1;
    }

    hres = ConnectToWMI();
    if (FAILED(hres)) {
        pLoc->Release();
        CoUninitialize();
        return 1;
    }

    hres = SetProxyBlanket();
    if (FAILED(hres)) {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return 1;
    }

    // Unsecured Apartment 생성
    hres = CoCreateInstance(CLSID_UnsecuredApartment, NULL, CLSCTX_LOCAL_SERVER, IID_IUnsecuredApartment, (void**)&pUnsecApp);
    if (FAILED(hres)) {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return 1;
    }

    // 각 프로세스에 대해 이벤트 모니터링 시작
    for (const auto& processName : processNames) {
        hres = MonitorProcess(processName, EventSink::CRE);
        if (FAILED(hres)) {
            Cleanup();
            return 1;
        }
        hres = MonitorProcess(processName, EventSink::DEL);
        if (FAILED(hres)) {
            Cleanup();
            return 1;
        }
    }

    return 0;
}