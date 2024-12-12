#include <stdio.h>
#include <tchar.h>
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // 일부 CString 생성자는 명시적으로 선언됩니다.
#include "eventsink.h"

EventSink::EventSink(ProcessNotify* i, int state)
{
    m_state = state;
    m_pInterface = i;
    EventSink();
}

ULONG EventSink::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}

ULONG EventSink::Release()
{
    LONG lRef = InterlockedDecrement(&m_lRef);
    if (lRef == 0)
        delete this;
    return lRef;
}

HRESULT EventSink::QueryInterface(REFIID riid, void** ppv)
{
    if (riid == IID_IUnknown || riid == IID_IWbemObjectSink)
    {
        *ppv = (IWbemObjectSink*)this;
        AddRef();
        return WBEM_S_NO_ERROR;
    }
    else return E_NOINTERFACE;
}


HRESULT EventSink::Indicate(long lObjectCount,
    IWbemClassObject** apObjArray)
{
    for (int i = 0; i < lObjectCount; i++)
    {
        _variant_t vtProp;

        HRESULT hr = apObjArray[i]->Get(L"TargetInstance", 0, &vtProp, 0, 0);
        if (FAILED(hr))
        {
            continue;
        }

        IWbemClassObject* pObj = NULL;
        hr = (vtProp.vt == VT_UNKNOWN ? vtProp.punkVal->QueryInterface(IID_IWbemClassObject, (void**)&pObj) : E_NOINTERFACE);
        if (FAILED(hr))
        {
            VariantClear(&vtProp);
            continue;
        }

        VARIANT vtName;
        hr = pObj->Get(L"Name", 0, &vtName, 0, 0);
        if (SUCCEEDED(hr))
        {

            if (m_state == EventSink::CRE) m_pInterface->Create(vtName.bstrVal);

            VariantClear(&vtName);
        }
        pObj->Release();
        VariantClear(&vtProp);
    }



    //HRESULT hres = S_OK;
    //
    //VARIANT vtProp;
    //
    //for (int i = 0; i < lObjectCount; i++)
    //{
    //
    //    if (m_state == EventSink::CRE) m_pInterface->Create();
    //    if (m_state == EventSink::DEL) m_pInterface->Delete();
    //
    //}

    return WBEM_S_NO_ERROR;
}

HRESULT EventSink::SetStatus(
    /* [in] */ LONG lFlags,
    /* [in] */ HRESULT hResult,
    /* [in] */ BSTR strParam,
    /* [in] */ IWbemClassObject __RPC_FAR* pObjParam
)
{
    if (lFlags == WBEM_STATUS_COMPLETE)
    {
        //printf("Call complete. hResult = 0x%X\n", hResult);
    }
    else if (lFlags == WBEM_STATUS_PROGRESS)
    {
        //printf("Call in progress.\n");
    }

    return WBEM_S_NO_ERROR;
}