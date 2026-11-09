#include "stdafx.h"
#include "cPlayerDefensiveShipsManager.h"
#include <Spore-Mod-Utils/Include/SporeModUtils.h>
#include <cmath>

using namespace SporeModUtils;
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
	lastColonyCount = 0;
	timeSinceLastCheck = 0.0f;
}

void cPlayerDefensiveShipsManager::Dispose()
{
	defendersOnPlanets.clear();
	planetsToReinforce.clear();
}

void cPlayerDefensiveShipsManager::Update(int deltaTime, int deltaMilliseconds)
{
	timeSinceLastCheck += static_cast<float>(deltaMilliseconds) / 1000.0f;

	if (GetCurrentContext() == SpaceContext::Planet) {
		cPlanetRecord* planet = GetActivePlanetRecord();
		if (planet != nullptr) {
			int currentColonies = 0;
			const auto& cities = Simulator::GetData<cCity>();
			for (const auto& city : cities) {
				if (city) {
					currentColonies++;
				}
			}

			// Проверка динамической установки новых ратуш прямо в реальном времени
			if (lastColonyCount > 0 && currentColonies > lastColonyCount) {
				int newColonies = currentColonies - lastColonyCount;
				int stepBonus = (planet->mType == PlanetType::T3) ? 4 : ((planet->mType == PlanetType::T2) ? 3 : 2);
				int shipsToSpawn = newColonies * stepBonus;
				AddDefendersToCurrentPlanet(planet, shipsToSpawn);
			}
			lastColonyCount = currentColonies;
		}
	}

	// Обработка очереди подкреплений раз в 2 секунды
	if (timeSinceLastCheck >= 2.0f) {
		timeSinceLastCheck = 0.0f;
		ManagePlanetDefenders();
	}
}

void cPlayerDefensiveShipsManager::OnModeEntered(uint32_t modeType)
{
	if (GetCurrentContext() == SpaceContext::Planet) {
		cPlanetRecord* planet = GetActivePlanetRecord();
		if (planet != nullptr) {
			lastColonyCount = 0;
			const auto& cities = Simulator::GetData<cCity>();
			for (const auto& city : cities) {
				if (city) {
					lastColonyCount++;
				}
			}

			int currentDefenders = GetCurrentDefenders(planet);
			int maxDefenders = GetMaxDefenders(planet);
			if (currentDefenders < maxDefenders) {
				AddDefendersToCurrentPlanet(planet, maxDefenders - currentDefenders);
			}
		}
	}
}

void cPlayerDefensiveShipsManager::OnModeExited(uint32_t modeType)
{
	lastColonyCount = 0;
}

int cPlayerDefensiveShipsManager::GetMaxDefenders(Simulator::cPlanetRecord* planet)
{
	if (planet == nullptr) {
		return 3;
	}

	int baseDefenders = 10;
	if (PlanetUtils::Homeworld(planet)) {
		baseDefenders = 20;
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

	// Алгоритмический Soft-Cap демпфирования на пороге в 50 кораблей
	const int softCapThreshold = 50;
	if (rawTotal > softCapThreshold) {
		float excess = static_cast<float>(rawTotal - softCapThreshold);
		return softCapThreshold + static_cast<int>(round(sqrt(excess) * 4.0f));
	}

	return rawTotal;
}

int cPlayerDefensiveShipsManager::GetCurrentDefenders(Simulator::cPlanetRecord* planet)
{
	if (planet == nullptr) {
		return 0;
	}
	auto it = defendersOnPlanets.find(planet);
	if (it != defendersOnPlanets.end()) {
		return it->second;
	}
	return 0;
}

float cPlayerDefensiveShipsManager::CalculateShipHealth(Simulator::cPlanetRecord* planet)
{
	float baseHealth = 700.0f;
	if (planet == nullptr) {
		return baseHealth;
	}

	// Считываем владельца системы (игрок или конкретная NPC-империя)
	cEmpire* targetEmpire = nullptr;
	if (planet->GetStarRecord() != nullptr) {
		targetEmpire = StarManager.GetEmpire(planet->GetStarRecord()->mEmpireID);
	}
	if (targetEmpire == nullptr) {
		targetEmpire = Simulator::GetPlayerEmpire();
	}

	if (targetEmpire != nullptr) {
		Simulator::Archetypes archetype = ArchetypeUtils::GetBaseArchetype(targetEmpire->mArchetype);
		switch (archetype) {
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

	// Бонусы от типа пряности
	if (planet->mSpiceGen == 0x028682F8 || planet->mSpiceGen == 0x028682FB) { // Purple & Pink
		baseHealth += 200.0f;
	} else if (planet->mSpiceGen == 0x028682F9 || planet->mSpiceGen == 0x028682FA) { // Blue & Green
		baseHealth += 100.0f;
	}

	// Бонус от защитных турелей всех колоний на планете
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

void cPlayerDefensiveShipsManager::AddDefendersToCurrentPlanet(Simulator::cPlanetRecord* planet, int count)
{
	if (planet == nullptr || count <= 0) {
		return;
	}

	cStarRecord* star = planet->GetStarRecord();
	if (star == nullptr) {
		return;
	}

	// Разрешаем спавн защитников как игроку, так и любым развитым империям NPC
	cEmpire* empire = StarManager.GetEmpire(star->mEmpireID);
	if (empire == nullptr) {
		return;
	}

	float shipHealth = CalculateShipHealth(planet);

	for (int i = 0; i < count; ++i) {
		cGameDataUFOPtr ship = PlanetUtils::CreateShip(planet, empire);
		if (ship != nullptr) {
			ship->mMaxHealth = shipHealth;
			ship->mHealth = shipHealth;
			defendersOnPlanets[planet]++;
		}
	}
}

void cPlayerDefensiveShipsManager::DecreaseDefendersOnPlanet(Simulator::cPlanetRecord* planet)
{
	if (planet == nullptr) {
		return;
	}

	auto it = defendersOnPlanets.find(planet);
	if (it != defendersOnPlanets.end() && it->second > 0) {
		it->second--;
		planetsToReinforce.push_back(planet);
	}
}

void cPlayerDefensiveShipsManager::ManagePlanetDefenders()
{
	if (planetsToReinforce.empty()) {
		return;
	}

	cPlanetRecordPtr planet = planetsToReinforce.front();
	planetsToReinforce.pop_front();

	if (planet != nullptr) {
		int current = GetCurrentDefenders(planet.get());
		int maxAllowed = GetMaxDefenders(planet.get());

		if (current < maxAllowed) {
			int batch = 1;
			if (planet->mType == PlanetType::T3) batch = 3;
			else if (planet->mType == PlanetType::T2) batch = 2;

			int countToAdd = min(batch, maxAllowed - current);

			// Списание спорлингов только с казны игрока
			cEmpire* playerEmpire = Simulator::GetPlayerEmpire();
			if (planet->GetStarRecord() != nullptr && 
				planet->GetStarRecord()->mEmpireID == Simulator::GetPlayerEmpireID() && 
				playerEmpire != nullptr && 
				playerEmpire->mEmpireMoney >= 1000) 
			{
				playerEmpire->mEmpireMoney -= 1000;
			}

			AddDefendersToCurrentPlanet(planet.get(), countToAdd);
		}
	}
}
