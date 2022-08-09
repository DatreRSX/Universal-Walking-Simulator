#pragma once

#include <UE/structs.h>
#include <Gameplay/helper.h>

/*



*/

TMap<FName, uint8_t*> GetRowMap(UObject* DataTable)
{
	static auto RowStructOffset = GetOffset(DataTable, "RowStruct");
	return *(TMap<FName, uint8_t*>*)(__int64(DataTable) + (RowStructOffset + sizeof(UObject*))); // because after rowstruct is rowmap
}

UObject* GetLootPackages()
{
	UObject* Playlist = Helper::GetPlaylist();

	auto LootPackagesSoft = Playlist->Member<TSoftObjectPtr>(_("LootPackages"));
	std::cout << "AssetPathName: " << LootPackagesSoft->ObjectID.AssetPathName.ToString() << '\n';

	auto LootPackages = FindObject(LootPackagesSoft->ObjectID.AssetPathName.ToString());
	std::cout << "LootPackages: " << LootPackages << '\n';

	if (LootPackages)
		std::cout << "LootPackages Name: " << LootPackages->GetFullName() << '\n';
	else
		return nullptr;

	auto LootPackagesRowMap = GetRowMap(LootPackages);
	
	auto Men = LootPackagesRowMap.Pairs.Elements.Data;

	std::cout << "Men Num: " << Men.Num() << '\n';

	std::cout << "RowStructName: " << (*LootPackages->Member<UObject*>(_("RowStruct")))->GetFullName() << '\n';

	for (int i = 0; i < Men.Num(); i++)
	{
		auto& Man = Men.At(i);
		auto& Pair = Man.ElementData.Value;
		auto RowName = Pair.First.ToString();
		auto LootPackageDataOfRow = Pair.Second; // ScriptStruct FortniteGame.FortLootPackageData
		// std::cout << std::format("[{}] {}\n", i, RowName);

		if (RowName.starts_with("WorldList.AthenaLoot"))
		{
			static auto off = FindOffsetStruct("ScriptStruct /Script/FortniteGame.FortLootPackageData", "ItemDefinition");
			static auto countOff = FindOffsetStruct("ScriptStruct /Script/FortniteGame.FortLootPackageData", "Count");

			auto ItemDef = (TSoftObjectPtr*)(__int64(LootPackageDataOfRow) + off);
			auto Count = (int*)(__int64(LootPackageDataOfRow) + countOff);

			std::cout << std::format("Count: {} ItemDef: {}\n", *Count, ItemDef->ObjectID.AssetPathName.ToString());

			/*
			
				"LootPackageID": "WorldList.AthenaLoot.Weapon.HighAssaultAuto",
				"Weight": 0.4,
				"NamedWeightMult": "None",
				"PotentialNamedWeights": [],
				"Count": 1,
				"LootPackageCategory": 0,
				"GameplayTags": [],
				"RequiredTag": "None",
				"LootPackageCall": "",
				"ItemDefinition": {
				  "AssetPathName": "/Game/Athena/Items/Weapons/WID_Assault_SemiAuto_Athena_UC_Ore_T03.WID_Assault_SemiAuto_Athena_UC_Ore_T03",
				  "SubPathString": ""
				},
				"PersistentLevel": "",
				"MinWorldLevel": -1,
				"MaxWorldLevel": -1,
				"bAllowBonusDrops": true,
				"Annotation": ";List:WorldList.AthenaLoot.Weapon.HighAssaultAuto.C0;Item:WID.Assault.SemiAuto.Athena.UC.Ore.T03"
			
			*/
		}
	}

	return LootPackages;
}