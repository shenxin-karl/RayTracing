#pragma once
#ifdef PLATFORM_WIN
    #include <string>
    #include <optional>
    #include <Windows.h>

namespace Registry {

/*
*	HKEY:
*		HKEY_CURRENT_CONFIG:	该根键包含当前用户的配置信息，例如桌面设置、应用程序首选项等
*		HKEY_CURRENT_USER:		该根键包含计算机的全局配置信息，例如安装的软件、硬件驱动程序等
*		HKEY_LOCAL_MACHINE:		该根键包含计算机上所有用户的配置信息
*		HKEY_USERS:				该根键包含计算机的当前配置信息，例如显示器分辨率、打印机设置等。
*		HKEY_CLASSES_ROOT:		该根键包含文件扩展名和注册表中的关联信息
*/
std::optional<std::string> GetString(HKEY hkey, std::wstring_view path, std::wstring_view key);

}    // namespace Registry

#endif