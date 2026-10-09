#pragma once
#include "GameMode/IGameModeHandler.hpp"
#include "Data/Fonts.h"
#include <cstdio>
#include <string>

// Live battle-stats panel for spectate mode: team kill score plus per-fighter
// K/D, refreshed every second. Purely visual; no gameplay logic.
class SpectateStatsPanel : public CCNode
{
public:
	static SpectateStatsPanel *create(GameLayer *layer)
	{
		auto p = new SpectateStatsPanel();
		if (p && p->init(layer))
		{
			p->autorelease();
			return p;
		}
		CC_SAFE_DELETE(p);
		return nullptr;
	}

	bool init(GameLayer *layer)
	{
		_layer = layer;

		auto ws = CCDirector::sharedDirector()->getWinSize();
		_bg = CCLayerColor::create(ccc4(0, 0, 0, 110), 236, 220);
		_bg->ignoreAnchorPointForPosition(false);
		_bg->setAnchorPoint(ccp(0.5f, 1.0f));
		_bg->setPosition(ccp(ws.width / 2, ws.height - 52));
		addChild(_bg);

		_label = CCLabelBMFont::create("", Fonts::White);
		_label->setAnchorPoint(ccp(0.5f, 1.0f));
		_label->setScale(0.34f);
		_label->setPosition(ccp(ws.width / 2, ws.height - 58));
		addChild(_label);

		schedule(schedule_selector(SpectateStatsPanel::refresh), 1.0f);
		refresh(0);
		return true;
	}

	void refresh(float)
	{
		if (!_layer)
			return;

		int kKills = 0, aKills = 0;
		int n = 0;
		for (auto hero : _layer->_CharacterArray)
		{
			if (!hero)
				continue;
			n++;
			if (hero->isKonohaGroup())
				kKills += (int)hero->getKillNum();
			else
				aKills += (int)hero->getKillNum();
		}
		// Grow/shrink the backing with the fighter count.
		_bg->setContentSize(CCSize(236, 34 + n * 15));

		char line[96];
		std::string s;
		snprintf(line, sizeof(line), "AI VS AI  --  %d : %d\n", kKills, aKills);
		s += line;
		s += "------------------------\n";
		for (auto hero : _layer->_CharacterArray)
		{
			if (!hero)
				continue;
			snprintf(line, sizeof(line), "%s  K%d D%d\n",
					 hero->getName().c_str(), (int)hero->getKillNum(), (int)hero->getDeadNum());
			s += line;
		}
		_label->setString(s.c_str());
	}

private:
	GameLayer *_layer = nullptr;
	CCLayerColor *_bg = nullptr;
	CCLabelBMFont *_label = nullptr;
};

// Spectate (AI vs AI): team duel from 1v1 up to 5v5 where EVERY fighter is
// AI-driven and the human only watches.
//
// Fighter #1 keeps Role::Player (so the HUD, camera, scoring and game-over
// flow keep working untouched) but is driven by doAI() exactly like the
// Ino-possession path does, with every input locked. All other fighters are
// Role::Com and get doAI() from the battle runtime like any CPU opponent.
class ModeSpectate : public IGameModeHandler
{
public:
	void init()
	{
		CCLOG("Enter SPECTATE (AI vs AI) mode.");

		gd.isHardCore = true;
		gd.isSpectate = true;
	}

	void onInitHeros()
	{
		int n = g_SpectateTeamSize;
		if (n < 1)
			n = 1;
		if (n > 5)
			n = 5;
		// One Role::Player (fighter #1, AI-driven later) + the rest Com.
		// Honors the heroes picked on the select screen, random for the rest.
		initHeros((uint32_t)n, (uint32_t)n);
	}

	void onGameStart()
	{
		getGameLayer()->_enableGuardian = false;

		// AI-drive the player's fighter and lock every input, so the human
		// is a pure spectator. Works whether the HUD is already up or still
		// initializing (same deferred pattern Mode1v1 uses).
		auto layer = getGameLayer();
		auto applySpectate = [layer]()
		{
			if (!layer)
				return;
			if (layer->currentPlayer)
				layer->currentPlayer->doAI();
			if (layer->getHudLayer())
				layer->getHudLayer()->_isAllButtonLocked = true;
			auto stats = SpectateStatsPanel::create(layer);
			if (stats)
				layer->addChild(stats, 9999);
		};
		if (layer->isHUDInit())
			applySpectate();
		else
			layer->onHUDInitialized(applySpectate);
	}

	void onGameOver()
	{
	}

	void onCharacterInit(CharacterBase *c)
	{
	}

	void onCharacterDead(CharacterBase *c)
	{
	}

	void onCharacterReborn(CharacterBase *c)
	{
	}
};
