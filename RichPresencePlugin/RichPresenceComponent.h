#pragma once
#include <string>
#include <windows.h>
#define STEAMWORKS_CLIENT_INTERFACES
#include "Steamworks.h"


class RichPresenceComponent
{
public:
	RichPresenceComponent(UINT parentId, const wchar_t* gameName);
	virtual ~RichPresenceComponent();
	void InitializeMain();
	void InitializeGameParent();
	void InitializeGameChild();
protected:
	std::wstring GetCurrentProcessName();
	std::wstring GetChildProcessCmd(const wchar_t* marker, const wchar_t* gameName);
	std::wstring GetCurrentDirectoryName();
private:
	UINT m_parentAppID;
	UINT m_parentProcID;
	std::wstring m_wsGameName;
	HANDLE m_hParentClosedEvent;
	HSteamPipe m_hPipe;
	HSteamUser m_hUser;
	ISteamClient017* m_pSteamClient;
	IClientEngine* m_pClientEngine;
	IClientUser* m_pClientUser;
	IClientShortcuts* m_pClientShortcuts;
	bool LoadSteamworks();
	void InitializePublicAPI();
	void InitializeClientAPI();
	void InitializePresence();
	void UpdateRichPresence(const char* status);
	void SpawnChildProcess();
};