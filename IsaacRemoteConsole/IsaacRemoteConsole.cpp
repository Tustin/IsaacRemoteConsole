#include "stdafx.h"
#include "IsaacRemoteConsole.h"
#include "Resource.h"

#include "enums.h"
using namespace std;
extern "C" { int _afxForceUSRDLL; }

DWORD freeMem = 0x0E001932; //spot in memory that is rw where we can throw the command buffer. shouldnt cause issues.
DWORD processCommandAddress = 0x013CE6A0; //function address that parses the command. function prototype from IDA: void __thiscall ProcessCommand(char *this)
DWORD unkPointer = 0x017C9C98; //some pointer of a pointer that is read for the command struct
DWORD unkPointerOffset = 0x0A0B0; //the offset of the pointer at unkPointer that points to the command structure
DWORD commandBufferOffset = 0x0C; //the offset from the commandStructAddress to the buffer for the command

//runtime addresses are stored here
DWORD unkPointerAddress;
DWORD commandStructAddress;
DWORD commandBuffer;
HANDLE isaacProcess = GetCurrentProcess();

//simulates a __thiscall method by pushing the commandstructaddress into ecx
void ExecuteCommand() {
	__asm {
		push eax
		push ecx
		push esi
		push edi
		push edx
		mov eax, 0x0D
		mov edx, 0x0D
		mov ecx, [commandStructAddress]
		mov esi, [commandStructAddress]
		mov edi, [unkPointerAddress]
		call processCommandAddress
		pop     edi
		pop     esi
		pop     ebx
		mov     esp, ebp
		pop     ebp
		retn
	}

}

struct ConsoleCommand {
	//state of the menu (see ConsoleState in enums.h)
	int state;
	//how long the menu has been opened (not necessary)
	int timer;
	//some random padding, not sure if this is for the timer or what but it seems to always be empty
	char padding[2];
	//Positioning of the menu
	byte y;
	byte x;
	//this command buffer is weird, it can contain either a pointer to another region of memory with the text, OR the text can be stored here directly
	//the issue with storing it directly is that there's only so much space in this buffer
	//the game will automatically write the buffer to a new region of memory and set the pointer here if the user enters a command longer than 16 chars
	//i just cut out the middleman and use the pointer instead of dealing with the other garbage
	DWORD pointerToCommandBuffer;
	//padding for where the other 11 bytes would be if you put the command here directly
	char padding2[11];
	//padding between the buffer and the length of the buffer
	byte padding3;
	//length of the buffer - NOTE: not totally sure if this is int but it works
	int bufferLength;
	//how the engine parses the command (see BufferType in enums.h)
	byte bufferTypeFlag;

};
DWORD GetPointerToCommandStruct() {
	unkPointerAddress = (*(DWORD*)unkPointer);
	return unkPointerAddress + unkPointerOffset;
}

BOOL CALLBACK EventHandler(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg)
	{
	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED)
		{
			switch (wParam)
			{
			case IDOK:
				char cmd[64];
				GetDlgItemTextA(hDlg, IDC_EDIT1, cmd, sizeof(cmd));
				int commandLen = strlen(cmd);
				WriteProcessMemory(isaacProcess, (LPVOID)freeMem, cmd, commandLen + 1, 0);
				//dont judge me. this is just freeMem as little endian lol
				char mem[] = { 0x32, 0x19, 0x00, 0x0E };
				//write the address to our command buffer into the command struct
				WriteProcessMemory(isaacProcess, (LPVOID)commandBuffer, mem, 4, 0);
				//set length for the command entered
				DWORD commandBufferLen = commandBuffer + 0x10;
				DWORD commandBufferFlag = commandStructAddress + 0x20;
				*(BYTE*)commandBufferLen = commandLen;
				*(BYTE*)commandBufferFlag = BufferType::Pointer;
				ExecuteCommand();
			break;

			}
		}
		break;
	case WM_CLOSE:
		DestroyWindow(hDlg);
		ExitProcess(0);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		ExitProcess(0);
		break;
	default:
		return DefWindowProc(hDlg, uMsg, wParam, lParam);
	}
	return 0;
}

DWORD WINAPI mainWindow(HMODULE hMod) {
	commandStructAddress = GetPointerToCommandStruct();
	commandBuffer = commandStructAddress + 0x0C;
	//char out[64];
	//sprintf_s(out, "%02X", commandBuffer);
	//MessageBoxA(0, out, "", 0);
	DialogBox(hMod, MAKEINTRESOURCE(IDD_DIALOG1), NULL, (DLGPROC)EventHandler);
	ExitThread(0);
	return 0;
}
BOOL APIENTRY WINAPI DllMain(HINSTANCE hModule, DWORD dwAttached, LPVOID lpvReserved)
{
	if (dwAttached == DLL_PROCESS_ATTACH) {
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)mainWindow, hModule, 0, NULL);
	}
	return TRUE;
}