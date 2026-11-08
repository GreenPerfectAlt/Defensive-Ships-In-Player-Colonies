#pragma once

#include <Spore\BasicIncludes.h>
#include <Spore\Simulator\cCity.h>

using namespace Simulator;

#define cPlayerDefensiveShipsManagerPtr intrusive_ptr<cPlayerDefensiveShipsManager>

class cPlayerDefensiveShipsManager
	: public Simulator::cStrategy,
	public App::IMessageListener
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
	int GetColonyCount(cPlanetRecord* planet);
	int GetMaxDefenders(cPlanetRecord* planet);
	int GetSpawnBatch(cPlanetRecord* planet);
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
	int softCapThreshold;
	queue<planetReinforcementStruct> planetsToReinforce;
	eastl::map<cPlanetRecordPtr, int> planetDefenderShips;
	eastl::map<cPlanetRecordPtr, int> planetColonyCounts;

	uint32_t elapsedTime;
};
