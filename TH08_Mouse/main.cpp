#include <windows.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <cstdio>
using namespace std;

typedef UINT64 QWORD;

static DWORD GetProcessID(const wchar_t* processName) {
    DWORD processID = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);
    if (Process32FirstW(snapshot, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, processName) == 0) {
                processID = pe.th32ProcessID;
                break;
            }
        } while (Process32NextW(snapshot, &pe));
    }
    CloseHandle(snapshot);
    return processID;
}

const wchar_t* processName;
DWORD pid;
HANDLE hd;  //���̾��

//��ȡ����λ�õ�����
RECT gameWindow; //left, top, right, bottom
POINT mousePos; //x, y
HWND gameHandle;
struct handle_data {
    unsigned long process_id;
    HWND best_handle;
};

static BOOL IsMainWindow(HWND handle)
{
    return GetWindow(handle, GW_OWNER) == (HWND)0 && IsWindowVisible(handle);
}

static BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM lParam)
{
    handle_data& data = *(handle_data*)lParam;
    unsigned long process_id = 0;
    GetWindowThreadProcessId(handle, &process_id);
    if (data.process_id != process_id || !IsMainWindow(handle)) {
        return TRUE;
    }
    data.best_handle = handle;
    return FALSE;
}

static HWND FindMainWindow(unsigned long process_id)
{
    handle_data data;
    data.process_id = process_id;
    data.best_handle = 0;
    EnumWindows(EnumWindowsCallback, (LPARAM)&data);
    return data.best_handle;
}

//ӳ������
//��Ϸ���������꣺x�������8 ~ 376
//              y����ϵ���16 ~ 432  (th08)
//��Ӧ���ڵ�left+lb*width ~ right-rb*width
//          top+tb*height ~ bottom-bb*height  (lb = left blank)
//ֱ��ӳ�䣺[a1,a2]->[b1,b2] === ����(a1,b1),(a2,b2)��ֱ���ϵ�һ��(x,y)

float lb=40.0f/640.0f,
      rb=232.0f/640.0f,
      tb=32.0f/480.0f,
      bb=32.0f/480.0f;
//640 * 480��
//���հ�Ϊ32����16����16����224
//width = 384
//heigh = 448

static float mapping(float a1, float a2, float b1, float b2, long a) {
    if (a <= a1) {
        return b1;
    }
    if (a >= a2) {
        return b2;
    }
    float k = (b2 - b1) / (a2 - a1);
    return k * (float(a) - a1) + b1;
}

void XYMapping(RECT R, POINT P, float* x, float* y) {
    float width = float(R.right - R.left);
    float height = float(R.bottom - R.top);
    *x = mapping(float(R.left) + lb * width, float(R.right) - rb * width, 8.0f, 376.0f, P.x);
    *y = mapping(float(R.top) + tb * height, float(R.bottom) - bb * height, 16.0f, 432.0f, P.y);
}

//�޸���Ϸ������ֵ
//x:th08.exe+13D61AC y:th08.exe+13D61B0 ���ͣ�������

int gameN = 0;
float x, y;
DWORD xAddr = 0x400000,
      yAddr = 0x400000;

int main()
{
    while (gameN < 1 or gameN > 3) {
        printf("��ѡ����Ϸ�� 1)��ħ�� 2)������ 3)��ҹ��\n");
        scanf_s("%d", &gameN);
    }
    switch (gameN) {
    case 1:
        processName = L"�����g����.exe";
        xAddr += 0x2CAA68, yAddr += 0x2CAA6C;
        break;
    case 2:
        processName = L"th07.exe";
        xAddr += 0xBE408, yAddr += 0xBE40C;
        break;
    case 3:
        processName = L"th08.exe";
        xAddr += 0x13D61AC, yAddr += 0x13D61B0;
    }
    for (pid = GetProcessID(processName); pid == 0;) {
        printf("�޷���ȡĿ�����PID��������Ϸ�Ƿ�������\n");
        system("pause");
        pid = GetProcessID(processName);
    }
    for (hd = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid); hd == 0;) {//���̾��
        printf("�޷���ȡĿ����̾����������Ϸ�Ƿ�������\n");
        system("pause");
        hd = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    }
    for (gameHandle = FindMainWindow(pid); gameHandle == 0;) {
        printf("�޷���׽��Ϸ���ڣ�������Ϸ�Ƿ��������С�\n");
        system("pause");
    }
    printf("�����������á�\n");
    while (1) { //��ѭ��
        while (GetCursorPos(&mousePos) == 0) {
            printf("�޷���ȡָ��λ�ã�\n");
            system("pause");
        }
        while (GetWindowRect(gameHandle, &gameWindow) == 0) {
            printf("�޷���ȡ���ڴ�С��\n");
            system("pause");
        }
        XYMapping(gameWindow, mousePos, &x, &y);
        while (WriteProcessMemory(hd, LPVOID(xAddr), &x, sizeof(float), 0) == 0) {
            printf("�޷��޸��ڴ棡\n");
            system("pause");
        }
        while (WriteProcessMemory(hd, LPVOID(yAddr), &y, sizeof(float), 0) == 0) {
            printf("�޷��޸��ڴ棡\n");
            system("pause");
        }
    }
    return 0;
}
