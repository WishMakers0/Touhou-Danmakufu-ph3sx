#include "source/GcLib/pch.h"
#include "GcLibImpl.hpp"

/**********************************************************
WinMain
**********************************************************/
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
	gstd::DebugUtility::DumpMemoryLeaksOnExit();

	HWND handleWindow = nullptr;

	try {
		DnhConfiguration* config = DnhConfiguration::CreateInstance();
		ELogger* logger = ELogger::CreateInstance();
		logger->Initialize(config->IsLogFile(), config->IsLogWindow());
		EPathProperty::CreateInstance();

		EApplication* app = EApplication::CreateInstance();

		app->Initialize();

		if (app->IsRun()) {
			bool bInit = app->_Initialize();
			if (!bInit)
				throw gstd::wexception("Initialization failure.");
			handleWindow = app->GetPtrGraphics()->GetAttachedWindowHandle();

			app->Run();

			bool bFinalize = app->_Finalize();
			if (!bFinalize)
				throw gstd::wexception("Finalization failure.");
		}
	}
	catch (std::exception& e) {
		std::wstring log = StringUtility::ConvertMultiToWide(e.what());
		MessageBox(handleWindow, log.c_str(), L"Unexpected Error", MB_ICONERROR | MB_APPLMODAL | MB_OK);
	}
	catch (gstd::wexception& e) {
		MessageBox(handleWindow, e.what(), L"Engine Error", MB_ICONERROR | MB_APPLMODAL | MB_OK);
	}
	//	catch(...)
	//	{
	//		Logger::WriteTop("�s���ȃG���[");
	//	}

	EApplication::DeleteInstance();
	EPathProperty::DeleteInstance();
	ELogger::DeleteInstance();
	DnhConfiguration::DeleteInstance();

	return 0;
}
