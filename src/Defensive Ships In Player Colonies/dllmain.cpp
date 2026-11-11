// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include "cPlayerDefensiveShipsManager.h"

using namespace Simulator;

void Initialize()
{
	cPlayerDefensiveShipsManager* playerDefensiveShipsManager = new cPlayerDefensiveShipsManager();
	cSimulatorSystem::Get()->AddStrategy(playerDefensiveShipsManager, cPlayerDefensiveShipsManager::NOUN_ID);
	ModAPI::AddSimulatorStrategy(playerDefensiveShipsManager, cPlayerDefensiveShipsManager::NOUN_ID);
}

void Dispose()
{
	// This method is called when the game is closing
}

member_detour(GeneratePlanetHighLOD__detour, cSimPlanetHighLOD, void(cPlanet* planet)) {
	void detoured(cPlanet* planet) {
		original_function(this, planet);
		if (IsSpaceGame() && planet != nullptr && planet->mpPlanetRecord != nullptr &&
		    cPlayerDefensiveShipsManager::Get() != nullptr) {

			bool isPlayerPlanet = false;
			if (planet->GetEmpire() != nullptr && planet->GetEmpire() == GetPlayerEmpire()) {
				isPlayerPlanet = true;
			}
			else if (planet->mpPlanetRecord->GetStarRecord() != nullptr &&
			         GetPlayerEmpireID() != 0 &&
			         planet->mpPlanetRecord->GetStarRecord()->mEmpireID == GetPlayerEmpireID()) {
				isPlayerPlanet = true;
			}

			if (isPlayerPlanet) {
				cPlayerDefensiveShipsManager::Get()->ManagePlanetDefenders(planet->mpPlanetRecord.get());
			}
		}
	}
};

void AttachDetours()
{
	GeneratePlanetHighLOD__detour::attach(Address(ModAPI::ChooseAddress(0x00ffa340, 0x00ff96b0)));
}


// Generally, you don't need to touch any code here
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		ModAPI::AddPostInitFunction(Initialize);
		ModAPI::AddDisposeFunction(Dispose);

		PrepareDetours(hModule);
		AttachDetours();
		CommitDetours();
		break;

	case DLL_PROCESS_DETACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}
