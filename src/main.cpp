#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/CCScheduler.hpp>
#include <Geode/modify/GameManager.hpp>

using namespace geode::prelude;

static bool on(char const* key) {
	return Mod::get()->getSettingValue<bool>(key);
}

static float speedVal() {
	return (float)Mod::get()->getSettingValue<double>("speed");
}

static bool anyCheat() {
	return on("noclip") || on("noclip-spikes") || on("noclip-blocks") ||
		on("jump-hack") || on("autoclick") || speedVal() != 1.f;
}

class $modify(MMPlayLayer, PlayLayer) {
	struct Fields {
		CCLabelBMFont* info = nullptr;
		CCLabelBMFont* mark = nullptr;
		int attempts = 0;
		int avoided = 0;
		float time = 0.f;
		float clickTimer = 0.f;
		bool clickState = false;
		bool held = false;
		bool practiceForced = false;
		bool restarting = false;
	};

	void destroyPlayer(PlayerObject* player, GameObject* obj) {
		bool ignore = on("noclip");
		if (!ignore && obj) {
			int t = static_cast<int>(obj->m_objectType);
			if (t == 2 && on("noclip-spikes")) ignore = true;
			if (t == 0 && on("noclip-blocks")) ignore = true;
		}
		if (ignore) {
			m_fields->avoided++;
			return;
		}
		PlayLayer::destroyPlayer(player, obj);
		if (on("instant-respawn")) this->delayedResetLevel();
	}

	void resetLevel() {
		PlayLayer::resetLevel();
		auto f = m_fields.self();
		f->attempts++;
		f->time = 0.f;
		f->held = false;
		f->restarting = false;
		f->clickState = false;
		f->clickTimer = 0.f;
	}

	void postUpdate(float dt) {
		PlayLayer::postUpdate(dt);
		auto f = m_fields.self();
		f->time += dt;

		if (on("hitboxes")) {
			m_isDebugDrawEnabled = true;
			this->updateDebugDraw();
		}
		if (on("jump-hack")) {
			m_player1->m_isOnGround = true;
			m_player2->m_isOnGround = true;
		}
		if (on("force-practice") && !m_isPracticeMode && !f->practiceForced) {
			f->practiceForced = true;
			this->togglePracticeMode(true);
		}
		if (on("hide-p2") && m_player2) {
			m_player2->setVisible(false);
		}
		int op = (int)Mod::get()->getSettingValue<int64_t>("player-opacity");
		if (op < 255) m_player1->setOpacity((GLubyte)op);

		if (on("mute-music")) {
			FMODAudioEngine::sharedEngine()->setBackgroundMusicVolume(0.f);
		}

		if (on("auto-hold") && !f->held) {
			f->held = true;
			this->handleButton(true, 1, true);
		}

		if (on("autoclick")) {
			int cps = (int)Mod::get()->getSettingValue<int64_t>("cps");
			if (cps < 1) cps = 1;
			f->clickTimer += dt;
			float half = 1.f / (cps * 2.f);
			if (f->clickTimer >= half) {
				f->clickTimer -= half;
				f->clickState = !f->clickState;
				this->handleButton(f->clickState, 1, true);
			}
		}

		int at = (int)Mod::get()->getSettingValue<int64_t>("restart-at");
		if (at > 0 && !f->restarting && this->getCurrentPercent() >= (float)at) {
			f->restarting = true;
			this->delayedResetLevel();
		}

		auto winSize = CCDirector::get()->getWinSize();

		if (!f->info) {
			f->info = CCLabelBMFont::create("", "bigFont.fnt");
			f->info->setScale(0.35f);
			f->info->setAnchorPoint({0.f, 1.f});
			f->info->setPosition({6.f, winSize.height - 6.f});
			this->addChild(f->info, 100);
		}
		f->info->setVisible(on("counter"));
		if (on("counter")) {
			f->info->setString(fmt::format(
				"Intento {}\nTiempo {:.1f}s\nMuertes evitadas {}",
				f->attempts, f->time, f->avoided).c_str());
		}

		if (!f->mark) {
			f->mark = CCLabelBMFont::create("TRUCOS ACTIVOS", "bigFont.fnt");
			f->mark->setScale(0.3f);
			f->mark->setOpacity(140);
			f->mark->setPosition({winSize.width / 2.f, 8.f});
			this->addChild(f->mark, 100);
		}
		f->mark->setVisible(on("watermark") && anyCheat());
	}

	void levelComplete() {
		if (on("safe-mode") && anyCheat()) {
			m_isTestMode = true;
		}
		PlayLayer::levelComplete();
	}
};

class $modify(MMPlayer, PlayerObject) {
	void playDeathEffect() {
		if (on("no-death-effect")) return;
		PlayerObject::playDeathEffect();
	}
};

class $modify(MMScheduler, CCScheduler) {
	void update(float dt) {
		float s = speedVal();
		auto pl = PlayLayer::get();
		if (s <= 0.f || !pl) {
			s = 1.f;
		} else if (on("speed-practice-only") && !pl->m_isPracticeMode) {
			s = 1.f;
		}
		CCScheduler::update(dt * s);
	}
};

class $modify(MMGameManager, GameManager) {
	bool isIconUnlocked(int id, IconType type) {
		if (on("unlock-icons")) return true;
		return GameManager::isIconUnlocked(id, type);
	}

	bool isColorUnlocked(int key, UnlockType type) {
		if (on("unlock-icons")) return true;
		return GameManager::isColorUnlocked(key, type);
	}
};
