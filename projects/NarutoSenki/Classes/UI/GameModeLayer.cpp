#include "StartMenu.h"
#include "UI/GameModeLayer.h"
#include "UI/ModeMenuButton.hpp"
#include "Constants/UiFlowKeys.hpp"
#include "Data/Fonts.h"
#include <cstdio>

extern const GameData kDefaultGameData;

// const GameModeData &GameModeData::from(const char *path)
// {
//     GameModeData data = {}
//     return data;
// }

bool GameModeLayer::init()
{
	RETURN_FALSE_IF(!Layer::init());

	initModeData();

	// Mode buttons: touch handling and selection live in ModeMenuButton /
	// selectMode. Lua positions them (see lua/ui/GameModeLayer.lua).
	for (size_t i = 0; i < GameMode::__Internal_Max_Length; i++)
	{
		auto mode_btn = ModeMenuButton::create(format("GameMode/{}.png", i + 1));
		mode_btn->mode = (GameMode)i;
		mode_btn->setDelegate(this);
		menuButtons[i] = mode_btn;
		addChild(mode_btn);
	}

	// Text of the selected mode; Lua places it.
	menuLabel = CCLabelTTF::create();
	addChild(menuLabel, 5);

	// Return button: the action stays here, Lua places the menu.
	auto return_img = MenuItemSprite::create(Sprite::create("UI/return_btn.png"), nullptr, nullptr, this, menu_selector(GameModeLayer::backToMenu));
	returnMenu = Menu::create(return_img, nullptr);
	addChild(returnMenu, 5);

	// Spectate team-size picker (1v1..5v5). Pure C++ UI, positioned here;
	// Lua only lays out the mode buttons, the mode label and the return menu.
	teamPicker = Menu::create();
	for (int n = 1; n <= 5; n++)
	{
		auto label = CCLabelBMFont::create(format("{}v{}", n, n).c_str(), Fonts::White);
		label->setScale(0.42f);
		teamPickerLabels.push_back(label);
		auto item = CCMenuItemLabel::create(label, this, menu_selector(GameModeLayer::onTeamSizePicked));
		item->setTag(n);
		teamPicker->addChild(item);
	}
	teamPicker->alignItemsHorizontallyWithPadding(26);
	{
		auto ws = Director::sharedDirector()->getWinSize();
		teamPicker->setPosition(Vec2(ws.width / 2, 54));
	}
	teamPicker->setVisible(false);
	addChild(teamPicker, 20);

	// Lua builds the decoration and lays out every control created above.
	lua_call_func_self(GameModeFlowKeys::kInit, this, "GameModeLayer");

	// Locking needs the final button positions (Lua draws the chain on top).
	for (size_t i = 0; i < menuButtons.size(); i++)
	{
		if (modes[i].isLocked)
		{
			menuButtons.at(i)->useMask2 = modes.at(i).useMask2;
			menuButtons.at(i)->lock();
		}
	}

	return true;
}

void GameModeLayer::backToMenu(Ref *sender)
{
	if (teamPicker)
		teamPicker->setVisible(false);

	SimpleAudioEngine::sharedEngine()->playEffect("Audio/Menu/cancel.ogg");

	auto menuScene = Scene::create();
	auto menuLayer = StartMenu::create();
	menuScene->addChild(menuLayer);
	Director::sharedDirector()->replaceScene(TransitionFade::create(1.0f, menuScene));
}

void GameModeLayer::initModeData()
{
	// init mode text data
	auto lang = Application::sharedApplication()->getCurrentLanguage();
	if (lang == LanguageType::kLanguageChinese)
	{
		modes[GameMode::OneVsOne] = {"1 VS 1", ""};
		modes[GameMode::Classic] = {"3 VS 3", "经典模式"};
		modes[GameMode::FourVsFour] = {"4 VS 4", ""};
		modes[GameMode::HardCore_4Vs4] = {"硬核模式 (4 VS 4)", "禁用装备"};
		modes[GameMode::Boss] = {"Boss模式 (3 VS 1)", ""};
		modes[GameMode::Clone] = {"克隆模式 (3 VS 3)", ""};
		modes[GameMode::Deathmatch] = {"死亡竞赛 (3 VS 3)", ""};
		modes[GameMode::RandomDeathmatch] = {"随机死亡竞赛 (3 VS 3)", ""};
		modes[GameMode::Spectate] = {"AI VS AI", "观战模式"};
	}
	else // English
	{
		modes[GameMode::OneVsOne] = {"1 VS 1", ""};
		modes[GameMode::Classic] = {"3 VS 3", "Classic Mode"};
		modes[GameMode::FourVsFour] = {"4 VS 4", ""};
		modes[GameMode::HardCore_4Vs4] = {"HardCore (4 VS 4)", "Disabled gear"};
		modes[GameMode::Boss] = {"Boss (3 VS 3)", ""};
		modes[GameMode::Clone] = {"Clone (3 VS 3)", ""};
		modes[GameMode::Deathmatch] = {"Deathmatch (3 VS 3)", ""};
		modes[GameMode::RandomDeathmatch] = {"Random Deathmatch (3 VS 3)", ""};
		modes[GameMode::Spectate] = {"AI VS AI", "Spectate mode"};
	}

	// init in developtment game modes
	modes[GameMode::Boss].isLocked = true;
	modes[GameMode::Deathmatch].isLocked = true;
	modes[GameMode::Deathmatch].useMask2 = true;

	// init mode handlers
	for (size_t i = 0; i < GameMode::__Internal_Max_Length; i++)
	{
		auto &data = modes.at(i);
		if (data.isLocked)
			data.description += " (In developtment)";
		// data.handler = s_ModeHandlers[i];
	}
}

bool GameModeLayer::pushMode(const GameModeData &data)
{
	return true;
}

void GameModeLayer::removeMode(const GameModeData &data)
{
}

void GameModeLayer::selectMode(GameMode mode)
{
	auto data = modes[(size_t)mode];
	string label = data.title;
	if (data.description.size() > 0)
	{
		label += " | ";
		label += data.description;
	}
	menuLabel->setString(label.c_str());

	bool confirmed = setSelect(mode);
	// Refresh after setSelect(): it relies on the updated hasSelected flags.
	refreshSpectatePicker();

	if (confirmed)
	{
		for (size_t i = 0; i < modes.size(); i++)
			modes.at(i).isLocked = true;
		CCLOG("Selected %s mode", data.title.c_str());

		s_GameMode = mode;
		bool enableCustomSelect = false;
		if (Cheats < kMaxCheats && (mode == GameMode::FourVsFour || mode == GameMode::HardCore_4Vs4))
		{
			enableCustomSelect = false;
		}
		else if (mode != GameMode::RandomDeathmatch && mode != GameMode::Clone)
		{
			enableCustomSelect = Cheats >= kMaxCheats;
		}

		// call lua global function StartMenu.enterSelectLayer
		auto pStack = get_luastack;
		auto state = pStack->getLuaState();
		lua_getglobal(state, UiFlowKeys::kEnterSelectLayer);
		pStack->pushInt(mode);
		pStack->pushBoolean(enableCustomSelect);
		pStack->executeFunction(2);

		auto handler = getGameModeHandler();
		handler->setOldCheats(Cheats);
		handler->init();
	}
}

bool GameModeLayer::setSelect(GameMode mode)
{
	auto &data = modes.at(mode);
	if (!data.isLocked && data.hasSelected)
		return true;

	for (size_t i = 0; i < modes.size(); i++)
		modes.at(i).hasSelected = false;
	data.hasSelected = true;
	return false;
}

void GameModeLayer::onTeamSizePicked(Ref *sender)
{
	auto item = (MenuItem *)sender;
	int n = item ? item->getTag() : 3;
	if (n < 1)
		n = 1;
	if (n > 5)
		n = 5;
	g_SpectateTeamSize = n;
	refreshSpectatePicker();
}

void GameModeLayer::refreshSpectatePicker()
{
	if (!teamPicker)
		return;

	bool show = modes[(size_t)GameMode::Spectate].hasSelected;
	teamPicker->setVisible(show);
	if (!show)
		return;

	for (size_t idx = 0; idx < teamPickerLabels.size(); idx++)
	{
		auto label = teamPickerLabels[idx];
		int n = (int)idx + 1;
		bool selected = (n == g_SpectateTeamSize);
		label->setColor(selected ? ccc3(255, 200, 60) : ccc3(255, 255, 255));
		label->setScale(selected ? 0.52f : 0.42f);
	}

	char buf[96];
	snprintf(buf, sizeof(buf), "AI VS AI | Spectate mode | %dv%d",
			 g_SpectateTeamSize, g_SpectateTeamSize);
	menuLabel->setString(buf);
}
