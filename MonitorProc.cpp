#define _CRT_SECURE_NO_WARNINGS


#include <stdio.h>
#include <tchar.h>
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // �Ϻ� CString �����ڴ� ��������� ����˴ϴ�.
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

// COM ���̺귯���� �ʱ�ȭ�մϴ�. ��Ƽ ������ ȯ�濡�� �ʱ�ȭ�մϴ�.
HRESULT MonitorProc::InitializeCOM()
{
    return CoInitializeEx(0, COINIT_MULTITHREADED);
}


// COM ���� ������ �ʱ�ȭ�մϴ�.
// �⺻ ���� ���� �� IMPERSONATE ���� ������ �����մϴ�.
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


// WbemLocator ��ü�� �����մϴ�.
HRESULT MonitorProc::CreateLocator()
{
    return CoCreateInstance(
        CLSID_WbemLocator,
        0,
        CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, (LPVOID*)&pLoc);
}


// WMI ������ �����մϴ�. ROOT\CIMV2 ���ӽ����̽��� �����մϴ�.
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


// WMI ���� ȣ�⿡ ���� ���� ������ �����մϴ�.
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


// �̺�Ʈ�� ó���� EventSink ��ü�� �����ϰ� ����մϴ�.
HRESULT MonitorProc::MonitorProcess(const string& processName, EventSink::state eventType)
{
    EventSink* pSink = new EventSink(this, eventType);
    pSink->AddRef();

    IUnknown* pStubUnk = NULL;
    pUnsecApp->CreateObjectStub(pSink, &pStubUnk);

    IWbemObjectSink* pStubSink = NULL;
    pStubUnk->QueryInterface(IID_IWbemObjectSink, (void**)&pStubSink);

    // ���� ���ڿ��� �����Ͽ� �̺�Ʈ ����͸��� ���� �񵿱� ������ �����մϴ�.
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
        // ���� ���� ���� �� ���ҽ��� �����մϴ�.
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

    // Unsecured Apartment ����
    hres = CoCreateInstance(CLSID_UnsecuredApartment, NULL, CLSCTX_LOCAL_SERVER, IID_IUnsecuredApartment, (void**)&pUnsecApp);
    if (FAILED(hres)) {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return 1;
    }

    // �� ���μ����� ���� �̺�Ʈ ����͸� ����
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