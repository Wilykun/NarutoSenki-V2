#pragma once
// Developer contact popup for the start menu (Wilykun fork).
//
// A dim fullscreen layer with a centered card showing the developer's
// contact info. The WhatsApp row opens the chat link in the system browser
// through a tiny JNI bridge (NarutoSenki.openUrl, Android only); the
// Telegram ID is shown as plain text. Taps outside the card are swallowed
// so the menu buttons behind cannot be triggered while open.

#include "cocos2d.h"

#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
#include "platform/android/jni/JniHelper.h"
#include <jni.h>
#endif

class DeveloperLayer : public cocos2d::CCLayerColor
{
public:
	static DeveloperLayer* create()
	{
		auto p = new DeveloperLayer();
		if (p && p->init())
		{
			p->autorelease();
			return p;
		}
		CC_SAFE_DELETE(p);
		return nullptr;
	}

	virtual bool init()
	{
		if (!CCLayerColor::initWithColor(cocos2d::ccc4(0, 0, 0, 170)))
			return false;

		using namespace cocos2d;
		auto vs = CCDirector::sharedDirector()->getVisibleSize();
		auto origin = CCDirector::sharedDirector()->getVisibleOrigin();
		CCPoint center = ccp(origin.x + vs.width / 2, origin.y + vs.height / 2);

		// Gold frame behind the dark card (4px border effect).
		const float cardW = 460, cardH = 340;
		auto frame = CCLayerColor::create(ccc4(212, 175, 55, 255), cardW + 8, cardH + 8);
		frame->ignoreAnchorPointForPosition(false);
		frame->setAnchorPoint(ccp(0.5f, 0.5f));
		frame->setPosition(center);
		addChild(frame);

		auto card = CCLayerColor::create(ccc4(18, 14, 26, 255), cardW, cardH);
		card->ignoreAnchorPointForPosition(false);
		card->setAnchorPoint(ccp(0.5f, 0.5f));
		card->setPosition(center);
		addChild(card);

		float cy = center.y;

		auto title = CCLabelTTF::create("DEVELOPER", "Arial", 34);
		title->setColor(ccc3(212, 175, 55));
		title->setPosition(ccp(center.x, cy + 126));
		addChild(title);

		auto madeBy = CCLabelTTF::create("Dibuat oleh Wilykun", "Arial", 22);
		madeBy->setColor(ccc3(230, 230, 230));
		madeBy->setPosition(ccp(center.x, cy + 82));
		addChild(madeBy);

		// Tappable rows (WhatsApp opens the browser, Telegram is display-only).
		auto waLabel = CCLabelTTF::create("WhatsApp: 0896-8820-6739", "Arial", 24);
		waLabel->setColor(ccc3(120, 220, 140));
		auto waItem = CCMenuItemLabel::create(waLabel, this,
			menu_selector(DeveloperLayer::onWhatsApp));
		waItem->setPosition(ccp(center.x, cy + 28));

		auto closeLabel = CCLabelTTF::create("Tutup", "Arial", 24);
		closeLabel->setColor(ccc3(212, 175, 55));
		auto closeItem = CCMenuItemLabel::create(closeLabel, this,
			menu_selector(DeveloperLayer::onClose));
		closeItem->setPosition(ccp(center.x, cy - 82));

		auto menu = CCMenu::create(waItem, closeItem, nullptr);
		menu->setPosition(CCPointZero);
		addChild(menu);

		auto tgLabel = CCLabelTTF::create("Telegram ID: 5810736154", "Arial", 24);
		tgLabel->setColor(ccc3(140, 200, 240));
		tgLabel->setPosition(ccp(center.x, cy - 26));
		addChild(tgLabel);

		auto hint = CCLabelTTF::create("Ketuk WhatsApp untuk membuka chat", "Arial", 16);
		hint->setColor(ccc3(150, 150, 150));
		hint->setPosition(ccp(center.x, cy - 144));
		addChild(hint);

		// Swallow all other touches (priority below the card's own menu).
		setTouchMode(kCCTouchesOneByOne);
		setTouchEnabled(true);
		return true;
	}

	virtual void registerWithTouchDispatcher()
	{
		cocos2d::CCDirector::sharedDirector()->getTouchDispatcher()
			->addTargetedDelegate(this, -100, true);
	}
	virtual bool ccTouchBegan(cocos2d::CCTouch*, cocos2d::CCEvent*) { return true; }

	// Opens an URL in the system browser (Android via JNI, no-op elsewhere).
	static void openUrl(const char* url)
	{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
		cocos2d::JniMethodInfo minfo;
		if (cocos2d::JniHelper::getStaticMethodInfo(minfo,
			"com/proj/narsen/NarutoSenki", "openUrl", "(Ljava/lang/String;)V"))
		{
			jstring jurl = minfo.env->NewStringUTF(url);
			minfo.env->CallStaticVoidMethod(minfo.classID, minfo.methodID, jurl);
			minfo.env->DeleteLocalRef(jurl);
		}
#else
		(void)url;
#endif
	}

private:
	void onWhatsApp(Ref*)
	{
		openUrl("https://wa.me/6289688206739");
	}
	void onClose(Ref*)
	{
		removeFromParentAndCleanup(true);
	}
};
