PetalburgCity_PokemonCenter_1F_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, PetalburgCity_PokemonCenter_1F_OnTransition
	map_script MAP_SCRIPT_ON_RESUME, CableClub_OnResume
	.byte 0

PetalburgCity_PokemonCenter_1F_OnTransition:
	setrespawn HEAL_LOCATION_PETALBURG_CITY
	call Common_EventScript_UpdateBrineyLocation
	end

PetalburgCity_PokemonCenter_1F_EventScript_Nurse::
	setvar VAR_0x800B, LOCALID_PETALBURG_NURSE
	call Common_EventScript_PkmnCenterNurse
	waitmessage
	waitbuttonpress
	release
	end

PetalburgCity_PokemonCenter_1F_EventScript_FatMan::
	msgbox PetalburgCity_PokemonCenter_1F_Text_PCStorageSystem, MSGBOX_NPC
	end

PetalburgCity_PokemonCenter_1F_EventScript_Youngster::
	msgbox PetalburgCity_PokemonCenter_1F_Text_OranBerryRegainedHP, MSGBOX_NPC
	end

PetalburgCity_PokemonCenter_1F_EventScript_Woman::
	lock
	faceplayer
	msgbox PetalburgCity_PokemonCenter_1F_Text_ManyTypesOfPokemon, MSGBOX_DEFAULT
	specialvar VAR_RESULT, IsStarterInParty
	goto_if_eq VAR_RESULT, TRUE, PetalburgCity_PokemonCenter_1F_EventScript_SayStarterTypeInfo
	release
	end

PetalburgCity_PokemonCenter_1F_EventScript_SayStarterTypeInfo::
	call_if_eq VAR_STARTER_MON, 0, PetalburgCity_PokemonCenter_1F_EventScript_SayTreeckoType
	call_if_eq VAR_STARTER_MON, 1, PetalburgCity_PokemonCenter_1F_EventScript_SayTorchicType
	call_if_eq VAR_STARTER_MON, 2, PetalburgCity_PokemonCenter_1F_EventScript_SayMudkipType
	release
	end

PetalburgCity_PokemonCenter_1F_EventScript_SayTreeckoType::
	msgbox PetalburgCity_PokemonCenter_1F_Text_TreeckoIsGrassType, MSGBOX_DEFAULT
	return

PetalburgCity_PokemonCenter_1F_EventScript_SayTorchicType::
	msgbox PetalburgCity_PokemonCenter_1F_Text_TorchicIsFireType, MSGBOX_DEFAULT
	return

PetalburgCity_PokemonCenter_1F_EventScript_SayMudkipType::
	msgbox PetalburgCity_PokemonCenter_1F_Text_MudkipIsWaterType, MSGBOX_DEFAULT
	return

PetalburgCity_PokemonCenter_1F_Text_PCStorageSystem:
	.string "Esse Sistema de Armazenamento de\n"
	.string "POKéMON pelo PC…\p"
	.string "Quem inventou deve ser algum tipo\n"
	.string "de gênio da ciência!$"

PetalburgCity_PokemonCenter_1F_Text_OranBerryRegainedHP:
	.string "Quando meu POKéMON comeu uma\n"
	.string "FRUTA ORAN, recuperou PS!$"

PetalburgCity_PokemonCenter_1F_Text_ManyTypesOfPokemon:
	.string "Existem muitos tipos de POKéMON.\p"
	.string "Todo tipo tem pontos fortes e\n"
	.string "fracos contra outros tipos.\p"
	.string "Dependendo dos tipos dos POKéMON,\n"
	.string "a batalha pode ser fácil ou difícil.$"

PetalburgCity_PokemonCenter_1F_Text_TreeckoIsGrassType:
	.string "Por exemplo, o seu TREECKO\n"
	.string "é do tipo PLANTA.\p"
	.string "Ele é forte contra os tipos ÁGUA e\n"
	.string "TERRESTRE.\p"
	.string "Mas é fraco contra POKéMON do\n"
	.string "tipo FOGO.$"

PetalburgCity_PokemonCenter_1F_Text_TorchicIsFireType:
	.string "Por exemplo, o seu TORCHIC\n"
	.string "é do tipo FOGO.\p"
	.string "Ele é forte contra os tipos PLANTA e\n"
	.string "INSETO.\p"
	.string "Mas é fraco contra POKéMON do\n"
	.string "tipo ÁGUA.$"

PetalburgCity_PokemonCenter_1F_Text_MudkipIsWaterType:
	.string "Por exemplo, o seu MUDKIP\n"
	.string "é do tipo ÁGUA.\p"
	.string "Ele é forte contra o tipo FOGO.\p"
	.string "Mas é fraco contra POKéMON dos\n"
	.string "tipos PLANTA e ELÉTRICO.$"

