#include "CStrafeModule.hpp"
#include "CCheat.hpp"

CStrafeModule::CStrafeModule() : CModule("Strafe", 'B') {

}

void CStrafeModule::onEnable() {
	CCheat::eventBus->registerListener(this);
}


void CStrafeModule::onDisable() {
	CCheat::eventBus->unregisterListener(this);
}

void CStrafeModule::onEvent(const CSimpleEvent* event) {
	if (auto e = dynamic_cast<const UpdateEvent*>(event)) {
		CMinecraft* mc = CCheat::theMinecraft;

		if (!mc->thePlayer->onGround && mc->gameSettings->isAnyKeyDown()) {
			mc->thePlayer->strafe(mc->thePlayer->getSpeed());
		}
	}
}