#pragma once
#ifdef PLATFORM_WIN
    #include <string>
    #include <optional>
    #include <Windows.h>

namespace Registry {

/*
*	HKEY:
*		HKEY_CURRENT_CONFIG:	�ø���������ǰ�û���������Ϣ�������������á�Ӧ�ó�����ѡ���
*		HKEY_CURRENT_USER:		�ø��������������ȫ��������Ϣ�����簲װ�������Ӳ�����������
*		HKEY_LOCAL_MACHINE:		�ø�������������������û���������Ϣ
*		HKEY_USERS:				�ø�������������ĵ�ǰ������Ϣ��������ʾ���ֱ��ʡ���ӡ�����õȡ�
*		HKEY_CLASSES_ROOT:		�ø��������ļ���չ����ע����еĹ�����Ϣ
*/
std::optional<std::string> GetString(HKEY hkey, std::wstring_view path, std::wstring_view key);

}    // namespace Registry

#endif