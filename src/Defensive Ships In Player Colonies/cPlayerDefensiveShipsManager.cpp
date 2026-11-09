#include "stdafx.h"
#include "cPlayerDefensiveShipsManager.h"
#include <cmath>

using namespace Simulator;

cPlayerDefensiveShipsManager::cPlayerDefensiveShipsManager()
{
}

cPlayerDefensiveShipsManager::~cPlayerDefensiveShipsManager()
{
}

// For internal use, do not modify.
int cPlayerDefensiveShipsManager::AddRef()
{
	return DefaultRefCounted::AddRef();
}

// For internal use, do not modify.
int cPlayerDefensiveShipsManager::Release()
{
	return DefaultRefCounted::Release();
}

// You can extend this function to return any other types your class implements.
void* cPlayerDefensiveShipsManager::Cast(uint32_t type) const
{
	CLASS_CAST(Object);
	CLASS_CAST(cPlayerDefensiveShipsManager);
	return nullptr;
}

void cPlayerDefensiveShipsManager::Initialize()
{
}

void cPlayerDefensiveShipsManager::Dispose()
{
}

void cPlayerDefensiveShipsManager::Update(int deltaTime, int deltaMilliseconds)
{
	ManagePlanetDefenders(deltaMilliseconds);
}

int cPlayerDefensiveShipsManager::GetMaxDefenders(cPlanetRecord* planet)
{
	if (planet == nullptr) {
		return 3;
	}

	int baseDefenders = 10;
	cStarRecord* star = planet->GetStarRecord();
	if (star != nullptr && star->mEmpireID != 0) {
		cEmpire* empire = StarManager.GetEmpire(star->mEmpireID);
		if (empire != nullptr && empire->mHomePlanetKey.instanceID == planet->GetResourceKey().instanceID) {
			baseDefenders = 20;
		}
	} else if (planet->mType == PlanetType::T0) {
		baseDefenders = 3;
	} else if (planet->mType == PlanetType::T1) {
		baseDefenders = 5;
	} else if (planet->mType == PlanetType::T2) {
		baseDefenders = 8;
	}

	int cityCount = 1;
	if (GetCurrentContext() == SpaceContext::Planet && planet == GetActivePlanetRecord()) {
		int actualCities = 0;
		const auto& cities = Simulator::GetData<cCity>();
		for (const auto& city : cities) {
			if (city) actualCities++;
		}
		if (actualCities > 0) {
			cityCount = actualCities;
		}
	}

	int stepBonus = 4;
	if (planet->mType == PlanetType::T1) stepBonus = 2;
	else if (planet->mType == PlanetType::T2) stepBonus = 3;
	else if (planet->mType == PlanetType::T0) stepBonus = 1;

	int rawTotal = baseDefenders + max(cityCount - 1, 0) * stepBonus;

	const int softCapThreshold = 50;
	if (rawTotal > softCapThreshold) {
		float excess = static_cast<float>(rawTotal - softCapThreshold);
		return softCapThreshold + static_cast<int>(round(sqrt(excess) * 4.0f));
	}

	return rawTotal;
}

float cPlayerDefensiveShipsManager::CalculateShipHealth(cPlanetRecord* planet)
{
	float baseHealth = 700.0f;
	if (planet == nullptr) {
		return baseHealth;
	}

	cEmpire* targetEmpire = nullptr;
	if (planet->GetStarRecord() != nullptr) {
		targetEmpire = StarManager.GetEmpire(planet->GetStarRecord()->mEmpireID);
	}
	if (targetEmpire == nullptr) {
		targetEmpire = Simulator::GetPlayerEmpire();
	}

	if (targetEmpire != nullptr) {
		switch (targetEmpire->mArchetype) {
		case Archetypes::kArchetypeWarrior:
		case Archetypes::kArchetypeZealot:
		case Archetypes::kArchetypePlayerKnight:
		case Archetypes::kArchetypeGrob:
			baseHealth = 1000.0f;
			break;
		case Archetypes::kArchetypeScientist:
		case Archetypes::kArchetypeTrader:
		case Archetypes::kArchetypeEcologist:
			baseHealth = 700.0f;
			break;
		case Archetypes::kArchetypeBard:
		case Archetypes::kArchetypeShaman:
		case Archetypes::kArchetypeDiplomat:
		case Archetypes::kArchetypePlayerWanderer:
		default:
			baseHealth = 250.0f;
			break;
		}
	}

	if (planet->mSpiceGen == 0x028682F8 || planet->mSpiceGen == 0x028682FB) {
		baseHealth += 200.0f;
	} else if (planet->mSpiceGen == 0x028682F9 || planet->mSpiceGen == 0x028682FA) {
		baseHealth += 100.0f;
	}

	if (GetCurrentContext() == SpaceContext::Planet && planet == GetActivePlanetRecord()) {
		int totalTurrets = 0;
		const auto& cities = Simulator::GetData<cCity>();
		for (const auto& city : cities) {
			if (city) {
				totalTurrets += static_cast<int>(city->mTurrets.size());
			}
		}
		baseHealth += static_cast<float>(totalTurrets) * 15.0f;
	}

	return baseHealth;
}

void cPlayerDefensiveShipsManager::AddDefendersToCurrentPlanet(cPlanetRecord* planet, int count)
{
	if (planet == nullptr || count <= 0) {
		return;
	}

	cStarRecord* star = planet->GetStarRecord();
	if (star == nullptr) {
		return;
	}

	// Разрешаем спавн защитников для игрока и развитых империй NPC
	cEmpire* empire = StarManager.GetEmpire(star->mEmpireID);
	if (empire == nullptr) {
		return;
	}

	float shipHealth = CalculateShipHealth(planet);

	for (int i = 0; i < count; ++i) {
		cGameDataUFOPtr ship = Simulator::CreateUFO(empire, planet);
		if (ship != nullptr) {
			auto combatant = object_cast<cCombatant>(ship);
			if (combatant != nullptr) {
				combatant->mMaxHealth = shipHealth;
				combatant->mHealth = shipHealth;
			}
			planetDefendersCount[planet]++;
		}
	}
}

void cPlayerDefensiveShipsManager::DecreaseDefendersOnPlanet(cPlanetRecord* planet)
{
	if (planet == nullptr) {
		return;
	}

	auto it = planetDefendersCount.find(planet);
	if (it != planetDefendersCount.end() && it->second > 0) {
		it->second--;
		planetReinforcementStruct reinforcement;
		reinforcement.planet = planet;
		planetsToReinforce.push(reinforcement);
	}
}

void cPlayerDefensiveShipsManager::ManagePlanetDefenders(int deltaMilliseconds)
{
	if (planetsToReinforce.empty()) {
		return;
	}

	planetReinforcementStruct reinforcement = planetsToReinforce.front();
	planetsToReinforce.pop();

	cPlanetRecord* planet = reinforcement.planet.get();
	if (planet != nullptr) {
		int current = planetDefendersCount[planet];
		int maxAllowed = GetMaxDefenders(planet);

		if (current < maxAllowed) {
			int batch = (planet->mType == PlanetType::T3) ? 3 : 2;
			int countToAdd = min(batch, maxAllowed - current);

			cEmpire* playerEmpire = Simulator::GetPlayerEmpire();
			if (planet->GetStarRecord() != nullptr && 
				planet->GetStarRecord()->mEmpireID == Simulator::GetPlayerEmpireID() && 
				playerEmpire != nullptr && 
				playerEmpire->mEmpireMoney >= 1000) 
			{
				playerEmpire->mEmpireMoney -= 1000;
			}

			AddDefendersToCurrentPlanet(planet, countToAdd);
		}
	}
}
