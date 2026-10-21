#ifndef GAME_CLIENT_COMPONENTS_BESTCLIENT_PLAYER_ACTIONS_H
#define GAME_CLIENT_COMPONENTS_BESTCLIENT_PLAYER_ACTIONS_H

#include <base/str.h>
#include <base/vmath.h>

#include <engine/console.h>

#include <game/client/component.h>
#include <game/client/ui.h>

#include <vector>

class IConfigManager;

enum
{
	PLAYERACTIONS_MAX_NAME = 32,
	PLAYERACTIONS_MAX_CMD = 1024,
	PLAYERACTIONS_MAX_BINDS = 64
};

class CPlayerActions : public CComponent
{
	float m_AnimationTime = 0.0f;
	float m_aAnimationTimeItems[PLAYERACTIONS_MAX_BINDS] = {0};

	bool m_Active = false;
	bool m_WasActive = false;

	vec2 m_SelectorMouse = vec2(0.0f, 0.0f);
	CUi::CTouchState m_TouchState;
	bool m_TouchPressedOutside = false;

	int m_PlayerActionId = -1;
	int m_SelectedBind = -1;

	int GetClosestClientId(vec2 Pos);

	static void ConOpenPlayerActionMenu(IConsole::IResult *pResult, void *pUserData);
	static void ConAddPlayerAction(IConsole::IResult *pResult, void *pUserData);
	static void ConRemovePlayerAction(IConsole::IResult *pResult, void *pUserData);
	static void ConResetAllPlayerActions(IConsole::IResult *pResult, void *pUserData);
	static void ConRemoveAllPlayerActions(IConsole::IResult *pResult, void *pUserData);

	static void ConfigSaveCallback(IConfigManager *pConfigManager, void *pUserData);

public:
	class CBind
	{
	public:
		char m_aName[PLAYERACTIONS_MAX_NAME] = "*";
		char m_aCommand[PLAYERACTIONS_MAX_CMD] = "";

		bool operator==(const CBind &Other) const
		{
			return !str_comp(m_aName, Other.m_aName) && !str_comp(m_aCommand, Other.m_aCommand);
		}
	};

	std::vector<CBind> m_vBinds;

	CPlayerActions();
	int Sizeof() const override { return sizeof(*this); }

	void OnReset() override;
	void OnRender() override;
	void OnConsoleInit() override;
	void OnInit() override;
	void OnStateChange(int NewState, int OldState) override;
	void OnRelease() override;
	bool OnInput(const IInput::CEvent &Event) override;
	bool OnCursorMove(float x, float y, IInput::ECursorType CursorType) override;

	bool IsActive() const { return m_Active; }

	void AddBind(const char *pName, const char *pCommand);
	void RemoveBind(const char *pName, const char *pCommand);
	void RemoveBind(int Index);
	void RemoveAllBinds();

	void AddDefaultBinds();

	void ExecuteBind(int Bind);
};

#endif // GAME_CLIENT_COMPONENTS_BESTCLIENT_PLAYER_ACTIONS_H
