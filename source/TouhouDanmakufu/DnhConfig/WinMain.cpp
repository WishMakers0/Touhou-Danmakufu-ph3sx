#include "source/GcLib/pch.h"

#include "GcLibImpl.hpp"
#include "MainWindow.hpp"


/**********************************************************
WinMain
**********************************************************/
int WINAPI wWinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPWSTR lpCmdLine,
	int nCmdShow) {
	gstd::DebugUtility::DumpMemoryLeaksOnExit();

	{
		DnhConfiguration::CreateInstance();
		EPathProperty::CreateInstance();
		MainWindow* wndMain = MainWindow::CreateInstance();
		wndMain->Initialize();

		EApplication* app = EApplication::CreateInstance();
		app->Initialize();
		app->Run();
	}

	EApplication::DeleteInstance();
	MainWindow::DeleteInstance();
	EPathProperty::DeleteInstance();
	DnhConfiguration::DeleteInstance();

	return 0;
}

