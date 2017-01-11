#include "stdafx.h"
#include "IsaacRemoteConsole.h"
#include "Resource.h"

#include "enums.h"
#include "Scanner.h"

using namespace std;
extern "C" { int _afxForceUSRDLL; }

BYTE processCommandAddressSignature[] = { 0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8, 0x6A, 0xFF, 0x68, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x00, 0x00, 0x00, 0x00, 0x50, 0x81, 0xEC, 0xE8, 0x01, 0x00, 0x00, 0xA1, 0x40, 0x99, 0x99, 0x99, 0x33, 0xC4, 0x99, 0x99, 0x99, 0x99, 0x01, 0x00, 0x00 };

BYTE commandStructurePointerSignature[] = { 0x00, 0x00, 0x00, 0x00, 0x20, 0x99, 0x99, 0x99, 0x00, 0x00, 0x80, 0x3F }; //this one isnt as accurate as the function sig above but its more difficult considering its just a pointer address

DWORD processCommandAddress; //function address that parses the command. function prototype from IDA: void __thiscall ProcessCommand(char *this)
DWORD commandStructurePointerOfPointerAddress; //pointer of a pointer that leads to the command structure address in memory. (trust me, i know how dumb this variable name is)
DWORD commandStructurePointerOffset = 0x0A0B0; //the offset of the pointer at commandStructureAddress that points to the command structure

//runtime addresses are stored here
DWORD commandStructurePointerAddress;
DWORD commandStructureAddress;

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
		mov ecx, [commandStructureAddress]
		mov esi, [commandStructureAddress]
		mov edi, [commandStructurePointerAddress]
		call processCommandAddress
		pop     edi
		pop     esi
		pop     ebx
		mov     esp, ebp
		pop     ebp
		retn
	}

}

void executetest(DWORD commandPointer, int commandLen) {
	__asm {
		push esi
		push edi
		mov esi, [commandPointer]
		mov edi, commandLen
		call processCommandAddress

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

} commandStructure;

DWORD GetPointerToCommandStruct() {
	commandStructurePointerAddress = (*(DWORD*)commandStructurePointerOfPointerAddress);
	return commandStructurePointerAddress + commandStructurePointerOffset;
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
				char cmd[255];
				int commandLen = strlen(cmd);

				GetDlgItemTextA(hDlg, IDC_EDIT1, cmd, commandLen);

				//create some memory for our command buffer
				LPVOID commandBufferAddress = VirtualAllocEx(isaacProcess, NULL, commandLen + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

				WriteProcessMemory(isaacProcess, commandBufferAddress, cmd, commandLen + 1, 0);

				commandStructure.pointerToCommandBuffer = (DWORD)commandBufferAddress;
				commandStructure.bufferLength = commandLen;
				commandStructure.bufferTypeFlag = BufferType::Pointer;

				//write our command structure into the command structure in game memory
				WriteProcessMemory(isaacProcess, (LPVOID)commandStructureAddress, &commandStructure, sizeof(commandStructure), 0);
				ExecuteCommand();
				//free our command buffer memory
				VirtualFreeEx(isaacProcess, commandBufferAddress, commandLen + 1, MEM_FREE);
				break;

			}
		}
		break;
	case WM_CLOSE:
		DestroyWindow(hDlg);
		ExitThread(0);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		ExitThread(0);
		break;
	default:
		return DefWindowProc(hDlg, uMsg, wParam, lParam);
	}
	return 0;
}

DWORD WINAPI MainWindow(HMODULE hMod) {
	HMODULE isaacBase = GetModuleHandle(NULL);

	processCommandAddress = FindAddress(processCommandAddressSignature, sizeof(processCommandAddressSignature), (DWORD)isaacBase, 0xFFFFFF);
	if (processCommandAddress == 00) {
		MessageBoxA(NULL, "Unable to find processCommandAddress using signature scan", "Error", MB_OK);
		ExitThread(0);
	}
	commandStructurePointerOfPointerAddress = FindAddress(commandStructurePointerSignature, sizeof(commandStructurePointerSignature), (DWORD)isaacBase, 0xFFFFFF);
	if (commandStructurePointerOfPointerAddress == 00) {
		MessageBoxA(NULL, "Unable to find commandStructureAddress using signature scan", "Error", MB_OK);
		ExitThread(0);
	}
	//add 4 because the signature scanner starts scanning at the bytes before the pointer we need
	commandStructurePointerOfPointerAddress += 0x4;

	commandStructureAddress = GetPointerToCommandStruct();
	//Load the structure from memory into our own defined struct
	if (!ReadProcessMemory(isaacProcess, (LPVOID)commandStructureAddress, &commandStructure, sizeof(commandStructure), 0)) {
		char out[64];
		sprintf_s(out, "Error loading structure from commandStructAddress %02X", commandStructureAddress);
		MessageBoxA(NULL, out, "Error", MB_OK);
		ExitThread(0);
	}

	DialogBox(hMod, MAKEINTRESOURCE(IDD_DIALOG1), NULL, (DLGPROC)EventHandler);
	ExitThread(0);
	return 0;
}
BOOL APIENTRY WINAPI DllMain(HINSTANCE hModule, DWORD dwAttached, LPVOID lpvReserved)
{
	if (dwAttached == DLL_PROCESS_ATTACH) {
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)MainWindow, hModule, 0, NULL);
	}
	return TRUE;
}