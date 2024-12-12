#ifndef __PSW_INTERFACE_EVT__
#define __PSW_INTERFACE_EVT__


class  ProcessNotify
{
public:

    // callback 
    virtual void Create(const std::wstring& processName) = 0;
    virtual void Delete(const std::wstring& processName) = 0;

};


#endif