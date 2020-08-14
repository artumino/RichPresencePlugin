#include "RichPresenceComponent.h"
#include <iostream>
#include "KeyValuesBuilder.h"
#include <vector>
#include <locale>
#include <codecvt>
#include <string>

#define REVIVE_CLOSED_HANDLE_NAME TEXT("Revive_ClosedEvent")

RichPresenceComponent::RichPresenceComponent(UINT parentId, const wchar_t* gameName)
	: m_parentAppID(218),
	m_wsGameName(gameName),
	m_parentProcID(parentId),
	m_hUser(NULL),
	m_hPipe(NULL),
	m_pClientEngine(NULL),
	m_pClientUser(NULL),
	m_pClientShortcuts(NULL),
	m_pSteamClient(NULL),
	m_hParentClosedEvent(NULL)
{
}

RichPresenceComponent::~RichPresenceComponent()
{
}

void RichPresenceComponent::InitializeMain()
{
	m_hParentClosedEvent = CreateEvent(
		NULL,               // default security attributes
		TRUE,
		FALSE,
		REVIVE_CLOSED_HANDLE_NAME  // object name
	);

	SetEnvironmentVariable(L"SteamAppId", L"218");

	/*_unlink("steam_appid.txt");*/

	if (LoadSteamworks())
	{
		InitializePublicAPI();
		SpawnChildProcess();
		InitializeClientAPI();
	}

	// write back steam_appid.txt for later purposes
	/*FILE* f = new FILE;
	if (fopen_s(&f, "steam_appid.txt", "w"))
	{
		fprintf(f, "%d", m_parentAppID);
		fclose(f);
	}*/

	//Wait for parent to end
	if (m_parentProcID)
	{
		HANDLE parentHandle = OpenProcess(SYNCHRONIZE, FALSE, m_parentProcID);
		WaitForSingleObject(parentHandle, INFINITE);
	}

	SetEvent(m_hParentClosedEvent);
	CloseHandle(m_hParentClosedEvent);
}

void RichPresenceComponent::InitializeGameParent()
{
	SetEnvironmentVariable(L"SteamAppId", nullptr);

	if (LoadSteamworks())
	{
		//_unlink("steam_appid.txt");

		InitializePublicAPI();
		InitializeClientAPI();
		InitializePresence();
	}
}

void RichPresenceComponent::InitializeGameChild()
{
	m_hParentClosedEvent = OpenEvent(SYNCHRONIZE,
		TRUE,
		REVIVE_CLOSED_HANDLE_NAME);

	HANDLE processHandle = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, m_parentProcID);
	if (processHandle != INVALID_HANDLE_VALUE)
	{
		while (true)
		{
			if (WaitForSingleObject(processHandle, 1000) != WAIT_TIMEOUT)
			{
				break;
			}
		}

		DWORD exitCode;
		GetExitCodeProcess(processHandle, &exitCode);

		CloseHandle(processHandle);
	}


	if (m_hParentClosedEvent != INVALID_HANDLE_VALUE)
	{
		WaitForSingleObject(m_hParentClosedEvent, INFINITE);
		CloseHandle(m_hParentClosedEvent);
	}
}


std::wstring RichPresenceComponent::GetCurrentProcessName()
{
	HMODULE hModule = GetModuleHandleW(NULL);
	WCHAR path[MAX_PATH];
	GetModuleFileName(hModule, path, MAX_PATH);
	return std::wstring(path);
}

std::wstring RichPresenceComponent::GetChildProcessCmd(const wchar_t* marker, const wchar_t* gameName)
{
	return L"\"" + GetCurrentProcessName() + L"\" -" + marker + L":" + std::to_wstring(GetCurrentProcessId()) + L" -name:" + gameName;
}

std::wstring RichPresenceComponent::GetCurrentDirectoryName()
{
	WCHAR dir[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, dir); 
	return std::wstring(dir);
}

bool RichPresenceComponent::LoadSteamworks()
{
	if (!OpenAPI_LoadLibrary())
	{
		std::wcout << "OpenAPI failed to load\n";
		return false;
	}
	std::wcout << "OpenAPI loaded\n";
	return true;
}

void RichPresenceComponent::InitializePublicAPI()
{
	m_pSteamClient = (ISteamClient017*)SteamInternal_CreateInterface(STEAMCLIENT_INTERFACE_VERSION_017);
	m_hPipe = m_pSteamClient->CreateSteamPipe();
	m_hUser = m_pSteamClient->ConnectToGlobalUser(m_hPipe);
	if (!m_hUser || !m_hPipe)
	{
		std::wcout << "Unable to create the global user.\n";
		return;
	}
}

void RichPresenceComponent::InitializeClientAPI()
{
	if (!m_hUser || !m_hPipe)
		return;

	m_pClientEngine = (IClientEngine*)SteamInternal_CreateInterface(CLIENTENGINE_INTERFACE_VERSION);
	if (!m_pClientEngine)
	{
		std::wcout << "Unable to get the client engine.\n";
		return;
	}
}

void RichPresenceComponent::InitializePresence()
{
	m_pClientUser = (IClientUser*)m_pClientEngine->GetIClientUser(m_hUser, m_hPipe);
	if (!m_pClientUser)
	{
		std::wcout << "Unable to get the client user interface.\n";
		return;
	}


	m_pClientShortcuts = m_pClientEngine->GetIClientShortcuts(m_hUser, m_hPipe);
	if (!m_pClientShortcuts)
	{
		std::wcout << "Unable to get the client shortcuts interface.\n";
		return;
	}

	uint32_t appId = m_pClientShortcuts->GetUniqueLocalAppId();
	std::wcout << "Successfully got ShortcutsAPI with appId " << appId << "\n";
	m_parentAppID = 243750;

	// create a fake app to hold our gameid           
	uint64_t gameID = 0x91F5912D01000000 | m_parentAppID;

	// create the keyvalues string for the app
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> converter;
	KeyValuesBuilder builder;
	builder.PackString("name", converter.to_bytes(m_wsGameName).c_str());
	builder.PackUint64("gameid", gameID);
	builder.PackString("installed", "1");
	builder.PackString("gamedir", "test");
	builder.PackString("serverbrowsername", "test");
	builder.PackEnd();

	std::string str = builder.GetString();
	IClientApps* m_pSteamClientApps = m_pClientEngine->GetIClientApps(m_hUser, m_hPipe);
	if (!m_pSteamClientApps)
		return;

	std::vector<uint8_t> myVector(str.begin(), str.end());
	uint8_t* p = &myVector[0];
	if (!m_pSteamClientApps->SetLocalAppConfig(appId, p, (int32_t)myVector.size()))
	{
		std::wcout << "Error setting config!\n";
		return;
	}

	IClientUtils* m_pSteamClientUtils = m_pClientEngine->GetIClientUtils(m_hPipe);
	if (!m_pSteamClientUtils)
		return;

	uint32_t newAppId = m_pSteamClientUtils->SetAppIDForCurrentPipe(m_parentAppID, false);
	if (newAppId != m_parentAppID)
	{
		std::wcout << "I failed to set the appId!\n";
		return;
	}

	std::wstring processName = GetCurrentProcessName();
	std::wstring commandLineChild = GetChildProcessCmd(L"child", m_wsGameName.c_str());
	std::wstring currentDirectory = GetCurrentDirectoryName();
	std::wcout << "Child process commandLine: " << commandLineChild << "\n";
	std::wcout << "Current directory " << currentDirectory << "\n";

	m_pClientUser->SpawnProcess(converter.to_bytes(processName).c_str(), converter.to_bytes(commandLineChild).c_str(), converter.to_bytes(currentDirectory).c_str(), (CGameID*)&gameID, converter.to_bytes(m_wsGameName).c_str(), 0, 0, 0);
	std::cout << "Spawned game process \n";
}

void RichPresenceComponent::UpdateRichPresence(const char* status)
{
	if (!m_pClientEngine || !m_hUser || !m_hPipe)
		return;

	IClientFriends* m_pSteamClientFriends = m_pClientEngine->GetIClientFriends(m_hUser, m_hPipe);
	if (!m_pSteamClientFriends)
		return;

	if (!m_pSteamClientFriends->SetRichPresence(218, "status", status))
		std::wcout << "Can't update rich presence!\n";
}

void RichPresenceComponent::SpawnChildProcess()
{
	std::wstring processName = GetCurrentProcessName();
	std::wstring commandLineParent = GetChildProcessCmd(L"parent", m_wsGameName.c_str());
	std::wstring currentDirectory = GetCurrentDirectoryName();
	std::wcout << "Parent process commandLine: " << commandLineParent << "\n";
	std::wcout << "Current directory " << currentDirectory << "\n";

	// run the steam parent
	STARTUPINFO si = { sizeof(STARTUPINFO) };
	PROCESS_INFORMATION pi;

	CreateProcessW(processName.c_str(), (wchar_t*)commandLineParent.c_str(), nullptr, nullptr, FALSE, 0, nullptr, currentDirectory.c_str(), &si, &pi);

	// wait for it to finish
	WaitForSingleObject(pi.hProcess, 15000);

	// and close up afterwards
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}