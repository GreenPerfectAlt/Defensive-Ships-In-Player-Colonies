#pragma once

#include <Spore\BasicIncludes.h>

using namespace Simulator;

#define cPlayerDefensiveShipsManagerPtr intrusive_ptr<cPlayerDefensiveShipsManager>

class cPlayerDefensiveShipsManager
	: public Simulator::cStrategy,
	App::IMessageListener
{
public:
	struct planetReinforcementStruct {
		cPlanetRecordPtr planet;
		uint32_t timeToReinforce;
	};
	static const uint32_t TYPE = id("Defensive_Ships_In_Player_Colonies::cPlayerDefensiveShipsManager");
	static const uint32_t NOUN_ID = TYPE;

	int AddRef() override;
	int Release() override;
	void Initialize() override;
	void Dispose() override;
	const char* GetName() const override;
	bool Write(Simulator::ISerializerStream* stream) override;
	bool Read(Simulator::ISerializerStream* stream) override;
	void Update(int deltaTime, int deltaGameTime) override;
	bool WriteToXML(XmlSerializer* xml) override;

	void OnModeEntered(uint32_t previousModeID, uint32_t newModeID) override;

	bool HandleMessage(uint32_t messageID, void* msg) override;

	static Simulator::Attribute ATTRIBUTES[];

	static cPlayerDefensiveShipsManager* Get();

	cGameDataUFO* SpawnPlayerDefensiveShip();

	/// @brief Считает макс. число кораблей с учетом Т-индекса, всех городов и мягкого замедления после 50.
	int GetMaxDefenders(cPlanetRecord* planet);

	/// @brief Считает, сколько кораблей генерируется за один цикл (зависит от Т-индекса и числа городов).
	int GetSpawnBatch(cPlanetRecord* planet);

	/// @brief Считает время до следующего спавна (замедляется, если планету активно штурмуют и флот разбит).
	int GetDynamicReinforceTime(cPlanetRecord* planet);

	int GetDefendersOnPlanet(cPlanetRecord* planet);

	void DecreaseDefendersOnPlanet(cPlanetRecord* planet);

	void AddDefenderToPlanet(cPlanetRecord* planetToReinforce, int countToAdd = 1);

	void ManagePlanetDefenders(cPlanetRecord* planet);

private:
	static cPlayerDefensiveShipsManager* instance;

	int playerOutpostDefenders;
	int playerColonyDefenders;
	int playerHomeworldDefenders;
	int reinforceTime;
	
	// Порог, после которого рост максимального числа кораблей начинает плавно замедляться
	int softCapThreshold;

	queue<planetReinforcementStruct> planetsToReinforce;
	eastl::map<cPlanetRecordPtr, int> planetDefenderShips;

	uint32_t elapsedTime;
};
