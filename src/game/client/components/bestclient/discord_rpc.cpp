/* Copyright © 2026 BestProject Team */
#include "discord_rpc.h"

#include <base/dbg.h>
#include <base/hash.h>
#include <base/mem.h>
#include <base/net.h>
#include <base/secure.h>
#include <base/str.h>
#include <base/time.h>

#include <engine/client.h>
#include <engine/discord.h>
#include <engine/http.h>
#include <engine/shared/json.h>
#include <engine/shared/jsonwriter.h>

#if defined(CONF_FAMILY_WINDOWS)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include <string>
#include <vector>

#if defined(CONF_DISCORD)
#include <discord_game_sdk.h>

typedef enum EDiscordResult(DISCORD_API *FDiscordCreate)(DiscordVersion, struct DiscordCreateParams *, struct IDiscordCore **);

#if defined(CONF_DISCORD_DYNAMIC)
#include <dlfcn.h>
static FDiscordCreate GetDiscordCreate()
{
	void *pSdk = dlopen("discord_game_sdk.so", RTLD_NOW);
	if(!pSdk)
	{
		return nullptr;
	}
	return (FDiscordCreate)dlsym(pSdk, "DiscordCreate");
}
#else
static FDiscordCreate GetDiscordCreate()
{
	return DiscordCreate;
}
#endif

class CDiscord : public IDiscord
{
	DiscordActivity m_Activity;
	bool m_UpdateActivity = false;
	int64_t m_LastActivityUpdate = 0;
	bool m_ShowMap = true;

	IDiscordCore *m_pCore = nullptr;
	IDiscordActivityEvents m_ActivityEvents;
	IDiscordActivityManager *m_pActivityManager = nullptr;

	FDiscordCreate m_pfnDiscordCreate = nullptr;
	bool m_Enabled = false;

	// Discord IPC bridge: the archived Game SDK treats the activity "name" as
	// read-only, so Discord always shows the name registered for the app ID.
	// The current Discord IPC accepts an explicit activity name, so the same
	// activity is also sent over the local pipe to show "Trial Client".
	int64_t m_AppId = 0;
	int64_t m_LastIpcConnectAttempt = 0;
	int64_t m_LastIpcActivitySent = 0;
	unsigned m_IpcNonce = 0;
#if defined(CONF_FAMILY_WINDOWS)
	HANDLE m_IpcPipe = INVALID_HANDLE_VALUE;
#endif

	void IpcDisconnect()
	{
#if defined(CONF_FAMILY_WINDOWS)
		if(m_IpcPipe != INVALID_HANDLE_VALUE)
		{
			CloseHandle(m_IpcPipe);
			m_IpcPipe = INVALID_HANDLE_VALUE;
		}
#endif
		m_LastIpcActivitySent = 0;
	}

	bool IpcWriteFrame(int Opcode, const char *pData, int Size)
	{
#if defined(CONF_FAMILY_WINDOWS)
		if(m_IpcPipe == INVALID_HANDLE_VALUE || pData == nullptr || Size <= 0)
			return false;

		unsigned char aHeader[8];
		aHeader[0] = (unsigned char)(Opcode & 0xFF);
		aHeader[1] = (unsigned char)((Opcode >> 8) & 0xFF);
		aHeader[2] = (unsigned char)((Opcode >> 16) & 0xFF);
		aHeader[3] = (unsigned char)((Opcode >> 24) & 0xFF);
		aHeader[4] = (unsigned char)(Size & 0xFF);
		aHeader[5] = (unsigned char)((Size >> 8) & 0xFF);
		aHeader[6] = (unsigned char)((Size >> 16) & 0xFF);
		aHeader[7] = (unsigned char)((Size >> 24) & 0xFF);

		DWORD Written = 0;
		if(!WriteFile(m_IpcPipe, aHeader, sizeof(aHeader), &Written, nullptr) || Written != sizeof(aHeader))
		{
			IpcDisconnect();
			return false;
		}
		if(!WriteFile(m_IpcPipe, pData, (DWORD)Size, &Written, nullptr) || Written != (DWORD)Size)
		{
			IpcDisconnect();
			return false;
		}
		return true;
#else
		(void)Opcode;
		(void)pData;
		(void)Size;
		return false;
#endif
	}

	bool IpcConnect()
	{
#if defined(CONF_FAMILY_WINDOWS)
		if(m_IpcPipe != INVALID_HANDLE_VALUE)
			return true;
		const int64_t Now = time_get();
		if(m_LastIpcConnectAttempt != 0 && Now - m_LastIpcConnectAttempt < time_freq() * 10)
			return false;
		m_LastIpcConnectAttempt = Now;

		for(int Index = 0; Index < 10; Index++)
		{
			char aPath[64];
			str_format(aPath, sizeof(aPath), "\\\\.\\pipe\\discord-ipc-%d", Index);
			HANDLE hPipe = CreateFileA(aPath, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
			if(hPipe == INVALID_HANDLE_VALUE)
				continue;

			DWORD Mode = PIPE_READMODE_BYTE;
			SetNamedPipeHandleState(hPipe, &Mode, nullptr, nullptr);
			m_IpcPipe = hPipe;

			char aHandshake[128];
			str_format(aHandshake, sizeof(aHandshake), "{\"v\":1,\"client_id\":\"%lld\"}", (long long)m_AppId);
			if(!IpcWriteFrame(0, aHandshake, str_length(aHandshake)))
			{
				IpcDisconnect();
				continue;
			}
			dbg_msg("discord", "ipc connected (%s)", aPath);
			return true;
		}
#else
		(void)m_AppId;
#endif
		return false;
	}

	void IpcHandleEvent(const char *pJson)
	{
		json_value *pRoot = JsonParse(pJson, str_length(pJson));
		if(pRoot == nullptr)
			return;

		const json_value *pEvt = json_object_get(pRoot, "evt");
		if(pEvt != nullptr && pEvt->type == json_string && str_comp(json_string_get(pEvt), "ACTIVITY_JOIN") == 0)
		{
			const json_value *pData = json_object_get(pRoot, "data");
			const json_value *pSecret = pData != nullptr ? json_object_get(pData, "secret") : nullptr;
			if(pSecret != nullptr && pSecret->type == json_string)
			{
				IClient *pClient = Kernel()->RequestInterface<IClient>();
				if(pClient)
					pClient->Connect(json_string_get(pSecret));
			}
		}
		json_value_free(pRoot);
	}

	void IpcReadAvailable()
	{
#if defined(CONF_FAMILY_WINDOWS)
		if(m_IpcPipe == INVALID_HANDLE_VALUE)
			return;

		for(;;)
		{
			DWORD Available = 0;
			if(!PeekNamedPipe(m_IpcPipe, nullptr, 0, nullptr, &Available, nullptr))
			{
				IpcDisconnect();
				break;
			}
			if(Available < 8)
				break;

			unsigned char aHeader[8];
			DWORD Read = 0;
			if(!ReadFile(m_IpcPipe, aHeader, sizeof(aHeader), &Read, nullptr) || Read != sizeof(aHeader))
			{
				IpcDisconnect();
				break;
			}

			const int Opcode = aHeader[0] | (aHeader[1] << 8) | (aHeader[2] << 16) | (aHeader[3] << 24);
			const int Size = aHeader[4] | (aHeader[5] << 8) | (aHeader[6] << 16) | (aHeader[7] << 24);
			if(Size <= 0 || Size > 1024 * 1024)
			{
				IpcDisconnect();
				break;
			}

			std::vector<char> vPayload(Size + 1);
			if(!ReadFile(m_IpcPipe, vPayload.data(), (DWORD)Size, &Read, nullptr) || Read != (DWORD)Size)
			{
				IpcDisconnect();
				break;
			}
			vPayload[Size] = '\0';
			if(Opcode == 1)
				IpcHandleEvent(vPayload.data());
		}
#endif
	}

	bool IpcSendActivity()
	{
#if defined(CONF_FAMILY_WINDOWS)
		if(!IpcConnect())
			return false;

		CJsonStringWriter Writer;
		Writer.BeginObject();
		Writer.WriteAttribute("cmd");
		Writer.WriteStrValue("SET_ACTIVITY");
		Writer.WriteAttribute("args");
		Writer.BeginObject();
		Writer.WriteAttribute("pid");
		Writer.WriteIntValue((int)GetCurrentProcessId());
		Writer.WriteAttribute("activity");
		Writer.BeginObject();
		Writer.WriteAttribute("name");
		Writer.WriteStrValue(m_Activity.name[0] != '\0' ? m_Activity.name : "Trial Client");
		Writer.WriteAttribute("type");
		Writer.WriteIntValue((int)m_Activity.type);
		if(m_Activity.details[0] != '\0')
		{
			Writer.WriteAttribute("details");
			Writer.WriteStrValue(m_Activity.details);
		}
		if(m_Activity.state[0] != '\0')
		{
			Writer.WriteAttribute("state");
			Writer.WriteStrValue(m_Activity.state);
		}
		if(m_Activity.timestamps.start != 0)
		{
			Writer.WriteAttribute("timestamps");
			Writer.BeginObject();
			Writer.WriteAttribute("start");
			Writer.WriteIntValue((int)m_Activity.timestamps.start);
			Writer.EndObject();
		}
		if(m_Activity.assets.large_image[0] != '\0' || m_Activity.assets.large_text[0] != '\0' ||
			m_Activity.assets.small_image[0] != '\0' || m_Activity.assets.small_text[0] != '\0')
		{
			Writer.WriteAttribute("assets");
			Writer.BeginObject();
			if(m_Activity.assets.large_image[0] != '\0')
			{
				Writer.WriteAttribute("large_image");
				Writer.WriteStrValue(m_Activity.assets.large_image);
			}
			if(m_Activity.assets.large_text[0] != '\0')
			{
				Writer.WriteAttribute("large_text");
				Writer.WriteStrValue(m_Activity.assets.large_text);
			}
			if(m_Activity.assets.small_image[0] != '\0')
			{
				Writer.WriteAttribute("small_image");
				Writer.WriteStrValue(m_Activity.assets.small_image);
			}
			if(m_Activity.assets.small_text[0] != '\0')
			{
				Writer.WriteAttribute("small_text");
				Writer.WriteStrValue(m_Activity.assets.small_text);
			}
			Writer.EndObject();
		}
		if(m_Activity.party.id[0] != '\0' || m_Activity.party.size.current_size > 0 || m_Activity.party.size.max_size > 0)
		{
			Writer.WriteAttribute("party");
			Writer.BeginObject();
			if(m_Activity.party.id[0] != '\0')
			{
				Writer.WriteAttribute("id");
				Writer.WriteStrValue(m_Activity.party.id);
			}
			if(m_Activity.party.size.current_size > 0 && m_Activity.party.size.max_size > 0)
			{
				Writer.WriteAttribute("size");
				Writer.BeginArray();
				Writer.WriteIntValue(m_Activity.party.size.current_size);
				Writer.WriteIntValue(m_Activity.party.size.max_size);
				Writer.EndArray();
			}
			Writer.EndObject();
		}
		if(m_Activity.secrets.join[0] != '\0' || m_Activity.secrets.spectate[0] != '\0' || m_Activity.secrets.match[0] != '\0')
		{
			Writer.WriteAttribute("secrets");
			Writer.BeginObject();
			if(m_Activity.secrets.join[0] != '\0')
			{
				Writer.WriteAttribute("join");
				Writer.WriteStrValue(m_Activity.secrets.join);
			}
			if(m_Activity.secrets.spectate[0] != '\0')
			{
				Writer.WriteAttribute("spectate");
				Writer.WriteStrValue(m_Activity.secrets.spectate);
			}
			if(m_Activity.secrets.match[0] != '\0')
			{
				Writer.WriteAttribute("match");
				Writer.WriteStrValue(m_Activity.secrets.match);
			}
			Writer.EndObject();
		}
		Writer.WriteAttribute("instance");
		Writer.WriteBoolValue(m_Activity.instance);
		Writer.EndObject(); // activity
		Writer.EndObject(); // args

		char aNonce[64];
		str_format(aNonce, sizeof(aNonce), "trial-%u-%lld", ++m_IpcNonce, (long long)time_timestamp());
		Writer.WriteAttribute("nonce");
		Writer.WriteStrValue(aNonce);
		Writer.EndObject();

		std::string Payload = std::move(Writer.GetOutputString());
		const bool Sent = IpcWriteFrame(1, Payload.data(), (int)Payload.size());
		if(Sent)
			m_LastIpcActivitySent = time_get();
		return Sent;
#else
		return false;
#endif
	}

	void SetSkinImage(const char *pSkinName)
	{
		if(!pSkinName || pSkinName[0] == '\0')
			return;

		constexpr char SKIN_URL_PREFIX[] = "https://best-client-api.vercel.app/api/assemble-foot/";
		constexpr char SKIN_URL_SUFFIX[] = ".png";
		const int MaxEscapedSkinNameLength = (int)sizeof(m_Activity.assets.small_image) -
			(int)sizeof(SKIN_URL_PREFIX) - (int)sizeof(SKIN_URL_SUFFIX) + 1;
		char aEscapedSkinName[128];
		EscapeUrl(aEscapedSkinName, sizeof(aEscapedSkinName), pSkinName);
		if(aEscapedSkinName[0] == '\0' || str_length(aEscapedSkinName) > MaxEscapedSkinNameLength)
			str_copy(aEscapedSkinName, "default");

		str_format(m_Activity.assets.small_image, sizeof(m_Activity.assets.small_image), "%s%s%s", SKIN_URL_PREFIX, aEscapedSkinName, SKIN_URL_SUFFIX);
	}

	void ApplyTrialBranding()
	{
		str_copy(m_Activity.name, "Trial Client");
		str_copy(m_Activity.details, "Trial Client");
		str_copy(m_Activity.assets.large_text, "Trial Client");
		const char *pLargeImage = TrialDiscordLargeImage();
		if(pLargeImage[0] != '\0')
			str_copy(m_Activity.assets.large_image, pLargeImage);
		else
			str_copy(m_Activity.assets.large_image, "bestclientlogo");
	}

public:
	bool Init(FDiscordCreate pfnDiscordCreate)
	{
		m_pfnDiscordCreate = pfnDiscordCreate;
		m_Enabled = false;
		return InitDiscord();
	}
	bool InitDiscord()
	{
		IpcDisconnect();
		if(m_pCore)
		{
			m_pCore->destroy(m_pCore);
			m_pCore = nullptr;
			m_pActivityManager = nullptr;
		}
		if(!m_Enabled)
			return false;

		mem_zero(&m_ActivityEvents, sizeof(m_ActivityEvents));

		m_ActivityEvents.on_activity_join = &CDiscord::OnActivityJoin;
		m_pActivityManager = nullptr;

		DiscordCreateParams Params;
		DiscordCreateParamsSetDefault(&Params);

		const int64_t AppId = str_toint64_base(TrialDiscordAppId(), 10);
		Params.client_id = AppId > 0 ? AppId : 1444576875774083133;
		m_AppId = Params.client_id;
		m_LastIpcConnectAttempt = 0;
		Params.flags = EDiscordCreateFlags::DiscordCreateFlags_NoRequireDiscord;
		Params.event_data = this;
		Params.activity_events = &m_ActivityEvents;

		int Error = m_pfnDiscordCreate(DISCORD_VERSION, &Params, &m_pCore);

		if(Error != DiscordResult_Ok)
		{
			m_pCore = nullptr;
			dbg_msg("discord", "error initializing discord instance, error=%d", Error);
			return true;
		}

		if(!m_pCore)
		{
			dbg_msg("discord", "discord initialization returned no core");
			return true;
		}

		m_pActivityManager = m_pCore->get_activity_manager(m_pCore);
		if(!m_pActivityManager)
		{
			dbg_msg("discord", "discord initialization returned no activity manager");
			m_pCore->destroy(m_pCore);
			m_pCore = nullptr;
			return true;
		}

		m_pActivityManager->register_command(m_pActivityManager, CONNECTLINK_DOUBLE_SLASH);
		m_pActivityManager->register_steam(m_pActivityManager, 412220);

		ClearGameInfo();
		dbg_msg("discord", "initialized custom RPC (app_id=%lld)", (long long)Params.client_id);

		return false;
	}

	void Update(bool Enabled) override
	{
		bool NeedsUpdate = m_Enabled != Enabled;
		m_Enabled = Enabled;

		if(NeedsUpdate)
		{
			if(!m_Enabled)
				IpcDisconnect();
			dbg_msg("discord", "rpc %s", m_Enabled ? "enabled" : "disabled");
			InitDiscord();
		}

		if(m_Enabled)
		{
			// Reconnect after a Discord restart and periodically re-assert the
			// activity so the custom name survives connection resets.
			if(!m_UpdateActivity && IpcConnect() &&
				(m_LastIpcActivitySent == 0 || time_get() - m_LastIpcActivitySent > time_freq() * 15))
			{
				m_UpdateActivity = true;
			}

			if(m_UpdateActivity && (m_LastActivityUpdate == 0 || time_get() > m_LastActivityUpdate + time_freq() * 5))
			{
				m_UpdateActivity = false;
				m_LastActivityUpdate = time_get();

				// Prefer the IPC bridge because it can override the displayed
				// application name. Fall back to the Game SDK when IPC is not
				// available (e.g. non-Windows builds).
				if(!IpcSendActivity() && m_pCore && m_pActivityManager)
					m_pActivityManager->update_activity(m_pActivityManager, &m_Activity, nullptr, nullptr);
			}

			if(m_pCore)
				m_pCore->run_callbacks(m_pCore);
			IpcReadAvailable();
		}
	}

	void ClearGameInfo() override
	{
		mem_zero(&m_Activity, sizeof(DiscordActivity));

		ApplyTrialBranding();
		m_Activity.timestamps.start = 0;
		m_Activity.instance = false;

		m_UpdateActivity = true;
	}

	void SetGameInfo(const CServerInfo &ServerInfo, const char *pMapName, const char *pPlayerName, const char *pSkinName, bool ShowMap, bool Registered) override
	{
		mem_zero(&m_Activity, sizeof(DiscordActivity));

		ApplyTrialBranding();
		m_Activity.timestamps.start = time_timestamp();
		m_Activity.instance = true;
		m_ShowMap = ShowMap;

		SetSkinImage(pSkinName);

		if(!TrialDiscordStreamerMode() && pPlayerName && pPlayerName[0] != '\0')
		{
			str_copy(m_Activity.assets.small_text, pPlayerName);
		}
		else
		{
			str_copy(m_Activity.assets.small_text, "Trial player");
		}

		if(m_ShowMap)
		{
			const char *pDisplayMap = (pMapName && pMapName[0] != '\0') ? pMapName : ServerInfo.m_aMap;
			if(pDisplayMap[0] != '\0')
			{
				str_copy(m_Activity.state, pDisplayMap);
			}
		}
		m_Activity.party.size.current_size = ServerInfo.m_NumClients;
		m_Activity.party.size.max_size = ServerInfo.m_MaxClients;
		m_Activity.party.privacy = Registered ? DiscordActivityPartyPrivacy_Public : DiscordActivityPartyPrivacy_Private;

		if(!Registered)
		{
			char aPartyId[sizeof(m_Activity.party.id)];
			secure_random_password(aPartyId, sizeof(aPartyId), 64);
			str_copy(m_Activity.party.id, aPartyId);
		}
		UpdateServerIp(ServerInfo);

		m_UpdateActivity = true;
	}

	void UpdateServerInfo(const CServerInfo &ServerInfo, const char *pMapName, const char *pPlayerName, const char *pSkinName) override
	{
		if(!m_Activity.instance)
			return;

		UpdateServerIp(ServerInfo);

		SetSkinImage(pSkinName);
		if(!TrialDiscordStreamerMode() && pPlayerName && pPlayerName[0] != '\0')
		{
			str_copy(m_Activity.assets.small_text, pPlayerName);
		}
		else
		{
			str_copy(m_Activity.assets.small_text, "Trial player");
		}

		if(m_ShowMap)
		{
			const char *pDisplayMap = (pMapName && pMapName[0] != '\0') ? pMapName : ServerInfo.m_aMap;
			if(pDisplayMap[0] != '\0')
			{
				str_copy(m_Activity.state, pDisplayMap);
			}
		}
		m_Activity.party.size.max_size = ServerInfo.m_MaxClients;
		m_UpdateActivity = true;
	}

	void UpdatePlayerCount(int Count) override
	{
		if(!m_Activity.instance)
			return;

		if(m_Activity.party.size.current_size == Count)
			return;

		m_Activity.party.size.current_size = Count;
		m_UpdateActivity = true;
	}

	void UpdateServerIp(const CServerInfo &ServerInfo)
	{
		if(!m_Activity.instance)
			return;

		// Trial: streamer mode must not expose the server address through the
		// Discord join secret or a party id derived from it.
		if(TrialDiscordStreamerMode())
		{
			m_Activity.secrets.join[0] = '\0';
			m_Activity.party.id[0] = '\0';
			m_Activity.party.privacy = DiscordActivityPartyPrivacy_Private;
			return;
		}

		if(str_length(ServerInfo.m_aAddress) < (int)sizeof(m_Activity.secrets.join))
		{
			str_copy(m_Activity.secrets.join, ServerInfo.m_aAddress);
		}
		else
		{
			char aAddr[NETADDR_MAXSTRSIZE];
			net_addr_str(&ServerInfo.m_aAddresses[0], aAddr, sizeof(aAddr), true);
			str_copy(m_Activity.secrets.join, aAddr);
		}

		if(m_Activity.party.privacy == DiscordActivityPartyPrivacy_Public)
		{
			char aPartyId[SHA256_MAXSTRSIZE];
			SHA256_DIGEST PartyIdSha256 = sha256(m_Activity.secrets.join, str_length(m_Activity.secrets.join));
			sha256_str(PartyIdSha256, aPartyId, sizeof(aPartyId));
			str_copy(m_Activity.party.id, aPartyId);
		}
	}

	static void DISCORD_CALLBACK OnActivityJoin(void *pEventData, const char *pSecret)
	{
		if(!pEventData || !pSecret || pSecret[0] == '\0')
			return;

		CDiscord *pSelf = static_cast<CDiscord *>(pEventData);
		IClient *pClient = pSelf->Kernel()->RequestInterface<IClient>();
		if(pClient)
			pClient->Connect(pSecret);
	}

	~CDiscord() override
	{
		IpcDisconnect();
		if(m_pCore)
			m_pCore->destroy(m_pCore);
	}
};

static IDiscord *CreateBestClientDiscordImpl()
{
	FDiscordCreate pfnDiscordCreate = GetDiscordCreate();
	if(!pfnDiscordCreate)
	{
		return nullptr;
	}
	CDiscord *pDiscord = new CDiscord();
	if(pDiscord->Init(pfnDiscordCreate))
	{
		delete pDiscord;
		return nullptr;
	}
	return pDiscord;
}
#else
static IDiscord *CreateBestClientDiscordImpl()
{
	return nullptr;
}
#endif

IDiscord *CreateBestClientDiscord()
{
	return CreateBestClientDiscordImpl();
}
