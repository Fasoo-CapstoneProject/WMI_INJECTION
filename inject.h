#include "windows.h"
#include "tchar.h"
#include <iostream>
#include <tlhelp32.h>
#include <psapi.h>
#include "util.h"



bool SetPrivilege(
	_In_z_ const wchar_t* privilege,
	_In_ bool enable
)
{
	_ASSERTE(nullptr != privilege);
	if (nullptr == privilege)
	{
		return false;
	}

	HANDLE token = INVALID_HANDLE_VALUE;
	if (TRUE != OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &token))
	{
		if (ERROR_NO_TOKEN == GetLastError())
		{
			if (TRUE != ImpersonateSelf(SecurityImpersonation))
			{
				return false;
			}

			if (TRUE != OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &token))
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}

	TOKEN_PRIVILEGES tp = { 0 };
	LUID luid = { 0 };
	DWORD cb = sizeof(TOKEN_PRIVILEGES);

	bool ret = false;
	do
	{
		if (!LookupPrivilegeValueW(NULL, privilege, &luid)) { break; }

		tp.PrivilegeCount = 1;
		tp.Privileges[0].Luid = luid;
		if (enable)
		{
			tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		}
		else
		{
			tp.Privileges[0].Attributes = 0;
		}

		AdjustTokenPrivileges(token, FALSE, &tp, cb, NULL, NULL);
		if (GetLastError() != ERROR_SUCCESS) { break; }

		ret = true;
	} while (false);

	CloseHandle(token);
	return ret;
}

HANDLE AdvancedOpenProcess(_In_ DWORD pid)
{
	_ASSERTE(NULL != pid);
	if (NULL == pid)
	{
		printf("pid�� NULL\n");
		return INVALID_HANDLE_VALUE;
	}

	HANDLE ret = NULL;
	if (true != SetPrivilege(L"SeDebugPrivilege", true))
	{
		printf("�켱���� ����1\n");
		return ret;
	}

	do
	{
		ret = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
		if (NULL == ret)
		{
			printf("ret�� NULL\n");
			break;
		}

		if (true != SetPrivilege(L"SeDebugPrivilege", false))
		{
			printf("�켱���� ����2\n");
			CloseHandle(ret);
			ret = NULL;
			break;
		}
	} while (false);

	return ret;
}


BOOL RtlCreateUserThread(_In_ HANDLE process_handle, _In_ wchar_t* buffer, _In_ SIZE_T buffer_size)
{
	HMODULE ntdll = NULL;
	HMODULE kernel32 = NULL;
	HANDLE thread_handle = NULL;
	CLIENT_ID cid;
	PROC_RtlCreateUserThread RtlCreateUserThread = NULL;
	PTHREAD_START_ROUTINE start_address = NULL;

	__try
	{
		ntdll = LoadLibraryW(L"ntdll.dll");
		if (NULL == ntdll)
		{
			printf("LoadLibrary(ntdll.dll) Func err gle : 0x%08X", GetLastError());
			return false;
		}

		RtlCreateUserThread = (PROC_RtlCreateUserThread)GetProcAddress(ntdll, "RtlCreateUserThread");
		if (NULL == RtlCreateUserThread)
		{
			printf("GetProcAddress(RtlCreateUserThread) Func err gle : 0x%08X", GetLastError());
			return false;
		}

		kernel32 = LoadLibrary("kernel32.dll");
		if (NULL == kernel32)
		{
			printf("LoadLibrary(kernel32.dll) Func err gle : 0x%08X", GetLastError());
			return false;
		}

		start_address = (PTHREAD_START_ROUTINE)GetProcAddress(kernel32, "LoadLibraryW");
		if (NULL == start_address)
		{
			printf("GetProcAddress(LoadLibraryW) Func err gle : 0x%08X", GetLastError());
			return false;
		}

		NTSTATUS status = RtlCreateUserThread(process_handle, NULL, false, 0, 0, 0, start_address, buffer, &thread_handle, &cid);
		if (status > 0)
		{
			printf("RtlCreateUserThread failed (0x%08x) status : %x\n", GetLastError(), status);
			return false;
		}

		status = WaitForSingleObject(thread_handle, INFINITE);
		if (status == WAIT_FAILED)
		{
			printf("WaitForSingleObject failed (0x%08x) status : %x\n", GetLastError(), status);
			return false;
		}
	}
	__finally
	{
		if (kernel32 != NULL)
			FreeLibrary(kernel32);
		if (ntdll != NULL)
			FreeLibrary(ntdll);
		if (thread_handle != NULL)
			CloseHandle(thread_handle);
	}
	return true;
}

bool InjectThread(
	_In_ DWORD pid,
	_In_ const wchar_t* dll_path
)
{
	_ASSERTE(NULL != pid);
	_ASSERTE(nullptr != dll_path);
	if (NULL == pid ||
		nullptr == dll_path)
	{
		return false;
	}

	HANDLE process_handle = NULL;
	SIZE_T buffer_size = 0;
	wchar_t* buffer = NULL;
	SIZE_T byte_written = 0;

	__try
	{
		process_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
		if (NULL == process_handle)
		{
			printf("OpenProcess Func err gle : 0x%08X", GetLastError());
			return false;
		}

		buffer_size = wcslen(dll_path) * sizeof(wchar_t) + 1;
		buffer = (wchar_t*)VirtualAllocEx(process_handle, NULL, buffer_size, MEM_COMMIT, PAGE_READWRITE);
		if (NULL == buffer)
		{
			printf("VirtualAllocEx Func err gle : 0x%08X", GetLastError());
			return false;
		}

		if (TRUE != WriteProcessMemory(process_handle, buffer, dll_path, buffer_size, &byte_written))
		{
			printf("WriteProcessMemory Func err gle : 0x%08X", GetLastError());
			return false;
		}

		if (TRUE != RtlCreateUserThread(process_handle, buffer, buffer_size))
		{
			printf("RtlCreateUserThread Func err gle : 0x%08X", GetLastError());
			return false;
		}
	}
	__finally
	{
		if (buffer != NULL)
			VirtualFreeEx(process_handle, buffer, buffer_size, MEM_COMMIT);
		if (process_handle != NULL)
			CloseHandle(process_handle);
	}

	return true;
}



uint32_t get_process_id(const char* process)
{
	DWORD pid = 0; // ���̵� �޾ƿ� ����

	// TH32CS_SNAPPROCESS�� 0�� �Ѱ� ������μ��� ������
	HANDLE hPorcessShap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (hPorcessShap == INVALID_HANDLE_VALUE) {
		printf("CreateToolhelp32Snapshot ����\n");
	}
	PROCESSENTRY32 pe; // ���μ��� ������ ���� ����ü

	// MODULEENTRY32�� ����ϱ� ���� ����ü ũ�� ��ŭ �ʱ�ȭ
	pe.dwSize = sizeof(MODULEENTRY32);

	// Process32Next�� �������� ù��° ���μ������� ��������
	while (Process32Next(hPorcessShap, &pe) == TRUE)
	{
		// ������ ���μ����� ���� ã�� ���μ��� �̸���
		if (strcmp(process, pe.szExeFile) == 0)
		{
			// ������ ���μ��� ���̵� ����
			pid = pe.th32ProcessID;

		}
	}
	// �ڵ�ݰ� ���μ��� ���̵� ����
	CloseHandle(hPorcessShap);
	return pid;
}



BOOL InjectDll(DWORD dwPID, LPCTSTR szDllPath)
{
    HANDLE hProcess = NULL, hThread = NULL;
    HMODULE hMod = NULL;
    LPVOID pRemoteBuf = NULL;
    DWORD dwBufSize = (DWORD)(_tcslen(szDllPath) + 1) * sizeof(TCHAR);
    LPTHREAD_START_ROUTINE pThreadProc;

    // #1. dwPID �� �̿��Ͽ� ��� ���μ���(notepad.exe)�� HANDLE�� ���Ѵ�.
    if (!(hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID)))
    {

        return FALSE;
    }

    // #2. ��� ���μ���(notepad.exe) �޸𸮿� szDllName ũ�⸸ŭ �޸𸮸� �Ҵ��Ѵ�.
    pRemoteBuf = VirtualAllocEx(hProcess, NULL, dwBufSize, MEM_COMMIT, PAGE_READWRITE);

    // #3. �Ҵ� ���� �޸𸮿� myhack.dll ���("c:\\myhack.dll")�� ����.
    WriteProcessMemory(hProcess, pRemoteBuf, (LPVOID)szDllPath, dwBufSize, NULL);

    // #4. LoadLibraryA() API �ּҸ� ���Ѵ�.
    hMod = GetModuleHandle("kernel32.dll");
    pThreadProc = (LPTHREAD_START_ROUTINE)GetProcAddress(hMod, "LoadLibraryA");

    // #5. notepad.exe ���μ����� �����带 ����
    hThread = CreateRemoteThread(hProcess, NULL, 0, pThreadProc, pRemoteBuf, 0, NULL);
    WaitForSingleObject(hThread, INFINITE);

    CloseHandle(hThread);
    CloseHandle(hProcess);

    printf("���� ����!\n");

    return TRUE;
}