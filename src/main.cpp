#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

static bool g_sim = false;
static bool g_simDied = false;

class $modify(CAPlayLayer, PlayLayer) {
	void destroyPlayer(PlayerObject* player, GameObject* obj) {
		if (g_sim) {
			g_simDied = true;
			return;
		}
		PlayLayer::destroyPlayer(player, obj);
	}
};

class $modify(CAGameLayer, GJBaseGameLayer) {
	struct Fields {
		bool pending = false;
		bool userHolding = false;
		bool injected = false;
		bool releaseNext = false;
		bool curState = false;
		float sinceClick = 0.f;
		int tick = 0;
		int lastNotify = -1000;
	};

	bool isCube() {
		auto p = m_player1;
		return !(p->m_isShip || p->m_isBird || p->m_isBall ||
			p->m_isDart || p->m_isRobot || p->m_isSpider || p->m_isSwing);
	}

	void press(bool down) {
		auto f = m_fields.self();
		f->injected = true;
		GJBaseGameLayer::handleButton(down, 1, true);
		f->injected = false;
		f->curState = down;
	}

	bool survives(bool hold, int steps, float dt) {
		auto f = m_fields.self();
		auto cp = PlayerCheckpoint::create();
		m_player1->saveToCheckpoint(cp);

		g_sim = true;
		g_simDied = false;
		f->injected = true;
		GJBaseGameLayer::handleButton(hold, 1, true);

		for (int i = 0; i < steps && !g_simDied; i++) {
			m_player1->update(dt);
			this->checkCollisions(m_player1, dt, false);
			if (m_player1->m_isDead) g_simDied = true;
		}

		m_player1->loadFromCheckpoint(cp);
		GJBaseGameLayer::handleButton(f->curState, 1, true);
		f->injected = false;
		g_sim = false;
		return !g_simDied;
	}

	void handleButton(bool down, int button, bool isPlayer1) {
		if (!m_fields->injected && !g_sim && button == 1 && isPlayer1) {
			m_fields->userHolding = down;
			m_fields->curState = down;
			if (down && Mod::get()->getSettingValue<bool>("enabled")
				&& isCube() && !m_player1->m_isOnGround) {
				m_fields->pending = true;
				m_fields->sinceClick = 0.f;
			}
		}
		GJBaseGameLayer::handleButton(down, button, isPlayer1);
	}

	void processCommands(float dt, bool isHalfTick, bool isLastTick) {
		GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);
		if (g_sim) return;

		auto mod = Mod::get();
		if (!mod->getSettingValue<bool>("enabled")) return;
		auto f = m_fields.self();

		// Etapa 1: buffer de click
		if (f->releaseNext) {
			f->releaseNext = false;
			if (!f->userHolding) press(false);
		}
		if (f->pending) {
			f->sinceClick += dt;
			float window = mod->getSettingValue<int64_t>("window") / 1000.f;
			if (m_player1->m_isOnGround) {
				if (f->sinceClick <= window) {
					press(true);
					f->releaseNext = true;
				}
				f->pending = false;
			} else if (f->sinceClick > window) {
				f->pending = false;
			}
		}

		// Etapa 2: predictor
		if (!mod->getSettingValue<bool>("predict")) return;
		if (!isCube() || m_player1->m_isDead) return;

		int interval = (int)mod->getSettingValue<int64_t>("interval");
		if (interval < 1) interval = 1;
		if (++f->tick % interval != 0) return;

		int steps = (int)mod->getSettingValue<int64_t>("lookahead");
		bool cur = f->curState;
		bool want = f->userHolding;

		if (cur != want && survives(want, steps, dt)) {
			press(want);
			return;
		}
		if (survives(cur, steps, dt)) return;
		if (survives(!cur, steps, dt)) {
			press(!cur);
			if (f->tick - f->lastNotify > 30) {
				f->lastNotify = f->tick;
				Notification::create("Click corregido", NotificationIcon::Success, 0.3f)->show();
			}
		}
	}
};
