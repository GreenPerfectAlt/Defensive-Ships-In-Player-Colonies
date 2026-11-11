#pragma once

#include <Spore\Simulator\Serialization.h>
#include <Spore\BasicIncludes.h>
#include <Spore\Simulator\cCity.h>
#include <EASTL\vector.h>
#include <EASTL\map.h>

#define cPlayerDefensiveShipsManagerPtr intrusive_ptr<cPlayerDefensiveShipsManager>
namespace Simulator {
	class XmlSerializer;
}


class cPlayerDefensiveShipsManager 
	: public Simulator::cStrategy
	, public App::IMessageListener
{
public:
	static const uint32_t TYPE = 0xF5A81B20;
	static const uint32_t NOUN_ID = TYPE;
	cPlayerDefensiveShipsManager();
	~cPlayerDefensiveShipsManager();

	int AddRef() override;
	int Release() override;
	void Initialize() override;
	void Dispose() override;
	const char* GetName() const override;
	bool Write(Simulator::ISerializerStream* stream) override;
	bool Read(Simulator::ISerializerStream* stream) override;
	bool WriteToXML(Simulator::XmlSerializer* xml) override;
	void Update(int deltaTime, int deltaGameTime) override;
	void OnModeEntered(uint32_t previousModeID, uint32_t newModeID) override;
	bool HandleMessage(uint32_t messageID, void* msg) override;

	static Simulator::Attribute ATTRIBUTES[];

	static cPlayerDefensiveShipsManager* Get();

	int GetDynamicSoftCap(Simulator::cEmpire* empire);
	float GetSpiceBasePrice(Simulator::cPlanetRecord* planet);
	int CalculateShipBuildCost(Simulator::cEmpire* empire);

	int GetMaxDefenders(Simulator::cPlanetRecord* planet);
	int GetPeacePatrolCount(Simulator::cPlanetRecord* planet);
	int GetColonyCount(Simulator::cPlanetRecord* planet);
	int GetSpawnBatch(Simulator::cPlanetRecord* planet);
	int GetDynamicReinforceTime(Simulator::cPlanetRecord* planet);
	float CalculateShipHealth(Simulator::cPlanetRecord* planet, Simulator::cEmpire* empire);
	bool IsPlanetUnderAttack(Simulator::cPlanetRecord* planet);
	bool ShouldTriggerRebellion(Simulator::cPlanetRecord* planet, Simulator::cEmpire* empire);

	Simulator::cGameDataUFO* SpawnDefensiveShip(Simulator::cPlanetRecord* planet, Simulator::cEmpire* empire, bool canRebel = false);
	
	void ManagePlanetDefenders(Simulator::cPlanetRecord* planet);
	void DecreaseDefendersOnPlanet(Simulator::cPlanetRecord* planet);
	void AddDefenderToPlanet(Simulator::cPlanetRecord* planetToReinforce, int countToAdd);
	int GetDefendersOnPlanet(Simulator::cPlanetRecord* planet);

private:
	bool IsPlanetQueuedForReinforcement(Simulator::cPlanetRecord* planet) const;

	static cPlayerDefensiveShipsManager* instance;

	int elapsedTime;
	int reinforceTime;
	int playerColonyDefenders;
	int playerHomeworldDefenders;
	int playerOutpostDefenders;
	int softCapThreshold;
	int reserveToMobilize;
	float configFleetMultiplier;
	float configRebellionMultiplier;

	cPlanetRecordPtr lastActivePlanet;

	struct planetReinforcementStruct {
		cPlanetRecordPtr planet;
		int timeToReinforce;
	};

	eastl::vector<planetReinforcementStruct> planetsToReinforce;
	eastl::map<cPlanetRecordPtr, int> planetDefenderShips;
	eastl::map<cPlanetRecordPtr, int> planetColonyCounts;
};

