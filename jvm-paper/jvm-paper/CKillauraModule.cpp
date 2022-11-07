#include "CKillauraModule.hpp"
#include "CCheat.hpp"

CKillauraModule::CKillauraModule() : CModule("Killaura", 'R') {

}

void CKillauraModule::onEnable() {
	CCheat::eventBus->registerListener(this);
}

void CKillauraModule::onDisable() {
	CCheat::eventBus->unregisterListener(this);
}

void CKillauraModule::onEvent(const CSimpleEvent* event) {
	if (auto e = dynamic_cast<const UpdateEvent*>(event)) {
		CMinecraft* mc = CCheat::theMinecraft;
		
		for (CPlayer* entityPlayer : mc->thePlayer->theWorld->getPlayers()) {
			if (!entityPlayer || !entityPlayer->GetKlass()) continue;

			printf("0x%p\n", entityPlayer);
		}
	}
}