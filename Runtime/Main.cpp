#include <iostream>
#include <exception>
#include <Windows.h>
#include "Applcation/Application.h"
#include "Foundation/GameTimer.h"
#include "Foundation/StringUtil.h"

int main() {
    try {
        GameTimer timer;
        Application::OnInstanceCreate();

        Application *pApp = Application::GetInstance();
        pApp->OnCreate();
        while (pApp->IsRunning()) {
            timer.StartNewFrame();
            pApp->OnPreUpdate(timer);
            pApp->OnUpdate(timer);
            pApp->OnPostUpdate(timer);

            pApp->OnPreRender(timer);
            pApp->OnRender(timer);
            pApp->OnPostRender(timer);
        }
        pApp->OnDestroy();
        Application::OnInstanceDestroy();
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        std::wstring message = nstd::to_wstring(std::string(e.what()));
        MessageBox(nullptr, message.c_str(), L"Error", MB_OK | MB_ICONHAND);
    }
}
