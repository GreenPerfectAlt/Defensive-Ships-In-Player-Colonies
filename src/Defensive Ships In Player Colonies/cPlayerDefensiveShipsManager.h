#pragma once

#include <Spore\BasicIncludes.h>
#include <EASTL\vector.h>
#include <EASTL\queue.h>
#include <EASTL\map.h>

#define cPlayerDefensiveShipsManagerPtr intrusive_ptr<cPlayerDefensiveShipsManager>

class cPlayerDefensiveShipsManager 
	: public Simulator::cStrategy
	, public App::IMessageListener
{
public:
	static const uint32_t TYPE = 0xF5A81B20;
	static const uint32_t NOUN_ID = 0xF5A81B20;
	
	cPlayerDefensiveShipsManager();
	~cPlayerDefensiveShipsManager();

	int AddRef() override;
	int Release() override;
	void Initialize() override;
	void Dispose() override;
	const char* GetName() const override;
	bool Write(Simulator::ISerializerStream* stream) override;
	bool Read(Simulator::ISerializerStream* stream) override;
	bool WriteToXML(Simulator::XmlSerializer* writexml) override;
	void Update(int deltaTime, int deltaGameTime) override;
	void OnModeEntered(uint32_t previousModeID, uint32_t newModeID) override;
	bool HandleMessage(uint32_t messageID, void* msg) override;

	static cPlayerDefensiveShipsManager* Get();

	int GetMaxDefenders(Simulator::cPlanetRecord* planet);
	int GetPeacePatrolCount(Simulator::cPlanetRecord* planet);
	int GetColonyCount(Simulator::cPlanetRecord* planet);
	int GetSpawnBatch(Simulator::cPlanetRecord* planet);
	int GetDynamicReinforceTime(Simulator::cPlanetRecord* planet);
	float CalculateShipHealth(Simulator::cPlanetRecord* planet);
	bool IsPlanetUnderAttack(Simulator::cPlanetRecord* planet);

	Simulator::cGameDataUFO* SpawnPlayerDefensiveShip();
	void ManagePlanetDefenders(Simulator::cPlanetRecord* planet);
	void DecreaseDefendersOnPlanet(Simulator::cPlanetRecord* planet);
	void AddDefenderToPlanet(Simulator::cPlanetRecord* planetToReinforce, int countToAdd);
	int GetDefendersOnPlanet(Simulator::cPlanetRecord* planet);

	static Simulator::Attribute ATTRIBUTES[];

private:
	static cPlayerDefensiveShipsManager* instance;

	int elapsedTime;
	int reinforceTime;
	int playerColonyDefenders;
	int playerHomeworldDefenders;
	int playerOutpostDefenders;
	int softCapThreshold;
	int reserveToMobilize;

	struct planetReinforcementStruct {
		cPlanetRecordPtr planet;
		int timeToReinforce;
	};

	eastl::queue<planetReinforcementStruct> planetsToReinforce;
	eastl::map<cPlanetRecordPtr, int> planetDefenderShips;
	eastl::map<cPlanetRecordPtr, int> planetColonyCounts;
};
