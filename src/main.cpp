#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>

using namespace geode::prelude;

class $modify(CAGameLayer, GJBaseGameLayer) {
	struct Fields {
		bool pending = false;
		bool userHolding = false;
		bool injected = false;
		bool releaseNext = false;
		float sinceClick = 0.f;
	};

	bool isCube() {
		auto p = m_player1;
		return !(p->m_isShip || p->m_isBird || p->m_isBall ||
			p->m_isDart || p->m_isRobot || p->m_isSpider || p->m_isSwing);
	}

	void handleButton(bool down, int button, bool isPlayer1) {
		if (!m_fields->injected && button == 1 && isPlayer1) {
			m_fields->userHolding = down;
			if (down && Mod::get()->getSettingValue<bool>("enabled")
				&& isCube() && !m_player1->m_isOnGround) {
				m_fields->pending = true;
				m_fields->sinceClick = 0.f;
			}
		}
		GJBaseGameLayer::handleButton(down, button, isPlayer1);
	}

	void processCommands(float dt) {
		GJBaseGameLayer::processCommands(dt);

		if (!Mod::get()->getSettingValue<bool>("enabled")) return;
		auto f = m_fields.self();

		if (f->releaseNext) {
			f->releaseNext = false;
			if (!f->userHolding) {
				f->injected = true;
				GJBaseGameLayer::handleButton(false, 1, true);
				f->injected = false;
			}
		}

		if (!f->pending) return;
		f->sinceClick += dt;
		float window = Mod::get()->getSettingValue<int64_t>("window") / 1000.f;

		if (m_player1->m_isOnGround) {
			if (f->sinceClick <= window) {
				f->injected = true;
				GJBaseGameLayer::handleButton(true, 1, true);
				f->injected = false;
				f->releaseNext = true;
			}
			f->pending = false;
		} else if (f->sinceClick > window) {
			f->pending = false;
		}
	}
};
