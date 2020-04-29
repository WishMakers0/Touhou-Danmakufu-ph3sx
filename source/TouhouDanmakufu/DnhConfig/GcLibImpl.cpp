#include "source/GcLib/pch.h"

#include "GcLibImpl.hpp"
#include "MainWindow.hpp"

/**********************************************************
//EApplication
**********************************************************/
EApplication::EApplication() {

}
EApplication::~EApplication() {

}
bool EApplication::_Initialize() {
	EFileManager* fileManager = EFileManager::CreateInstance();
	fileManager->Initialize();

	HWND hWndMain = MainWindow::GetInstance()->GetWindowHandle();
	WindowLogger::InsertOpenCommandInSystemMenu(hWndMain);
//	::SetWindowText(hWndMain, "DnhViewer");
//	::SetClassLong(hWndMain, GCL_HICON, (LONG)LoadIcon(GetApplicationHandle(), MAKEINTRESOURCE(IDI_ICON)));

	EDirectInput* input = EDirectInput::CreateInstance();
	input->Initialize(hWndMain);

	MainWindow* wndMain = MainWindow::GetInstance();
	wndMain->StartUp();
	::SetForegroundWindow(wndMain->GetWindowHandle());

	return true;
}
bool EApplication::_Loop() {
	MainWindow* mainWindow = MainWindow::GetInstance();
	HWND hWndFocused = ::GetForegroundWindow();
	HWND hWndMain = mainWindow->GetWindowHandle();
	if (hWndFocused != hWndMain) {
		//��A�N�e�B�u���͓��삵�Ȃ�
		::Sleep(10);
		return true;
	}

	EDirectInput* input = EDirectInput::GetInstance();
	input->Update();
	mainWindow->UpdateKeyAssign();

	::Sleep(10);

	return true;
}
bool EApplication::_Finalize() {
	EFileManager::GetInstance()->EndLoadThread();
	EDirectInput::DeleteInstance();
	EFileManager::DeleteInstance();
	return true;
}


