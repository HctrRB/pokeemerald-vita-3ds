FortreeCity_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, FortreeCity_OnTransition
	map_script MAP_SCRIPT_ON_RESUME, FortreeCity_OnResume
	.byte 0

FortreeCity_OnTransition:
	setflag FLAG_VISITED_FORTREE_CITY
	end

FortreeCity_OnResume:
	setstepcallback STEP_CB_FORTREE_BRIDGE
	end

FortreeCity_EventScript_Man::
	msgbox FortreeCity_Text_SawGiganticPokemonInSky, MSGBOX_NPC
	end

FortreeCity_EventScript_Woman::
	lock
	faceplayer
	goto_if_set FLAG_KECLEON_FLED_FORTREE, FortreeCity_EventScript_WomanGymAccessible
	msgbox FortreeCity_Text_SomethingBlockingGym, MSGBOX_DEFAULT
	release
	end

FortreeCity_EventScript_WomanGymAccessible::
	msgbox FortreeCity_Text_ThisTimeIllBeatWinona, MSGBOX_DEFAULT
	release
	end

FortreeCity_EventScript_Girl::
	msgbox FortreeCity_Text_TreesGrowByDrinkingRainwater, MSGBOX_NPC
	end

FortreeCity_EventScript_OldMan::
	msgbox FortreeCity_Text_EveryoneHealthyAndLively, MSGBOX_NPC
	end

FortreeCity_EventScript_Boy::
	msgbox FortreeCity_Text_BugPokemonComeThroughWindow, MSGBOX_NPC
	end

FortreeCity_EventScript_GameboyKid::
	msgbox FortreeCity_Text_PokemonThatEvolveWhenTraded, MSGBOX_NPC
	end

FortreeCity_EventScript_CitySign::
	msgbox FortreeCity_Text_CitySign, MSGBOX_SIGN
	end

FortreeCity_EventScript_GymSign::
	msgbox FortreeCity_Text_GymSign, MSGBOX_SIGN
	end

FortreeCity_EventScript_Kecleon::
	lock
	faceplayer
	checkitem ITEM_DEVON_SCOPE
	goto_if_eq VAR_RESULT, TRUE, FortreeCity_EventScript_AskUseDevonScope
	msgbox FortreeCity_Text_SomethingUnseeable, MSGBOX_DEFAULT
	release
	end

FortreeCity_EventScript_AskUseDevonScope::
	msgbox FortreeCity_Text_UnseeableUseDevonScope, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, FortreeCity_EventScript_UseDevonScope
	release
	end

FortreeCity_EventScript_UseDevonScope::
	msgbox FortreeCity_Text_UsedDevonScopePokemonFled, MSGBOX_DEFAULT
	closemessage
	applymovement VAR_LAST_TALKED, Movement_KecleonAppears
	waitmovement 0
	waitse
	playmoncry SPECIES_KECLEON, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	applymovement VAR_LAST_TALKED, FortreeCity_Movement_KecleonFlee
	waitmovement 0
	removeobject VAR_LAST_TALKED
	setflag FLAG_KECLEON_FLED_FORTREE
	release
	end

FortreeCity_Movement_KecleonFlee:
	walk_right
	step_end

FortreeCity_Text_SawGiganticPokemonInSky:
	.string "Ninguém acredita, mas eu vi um\n"
	.string "POKéMON gigante no céu.\p"
	.string "Ele parecia se contorcer enquanto\n"
	.string "voava pra ROTA 131.\p"
	.string "Aliás… Snif, snif…\n"
	.string "Hã… Você está com cheiro de queimado.\p"
	.string "Esteve num vulcão, por acaso?$"

FortreeCity_Text_SomethingBlockingGym:
	.string "Quero ir ao GINÁSIO POKéMON, mas\n"
	.string "tem alguma coisa no caminho.\p"
	.string "Depois de todo aquele treino na\n"
	.string "ROTA 120…$"

FortreeCity_Text_ThisTimeIllBeatWinona:
	.string "Estou com os meus POKéMON de\n"
	.string "estimação. Desta vez, venço a WINONA.$"

FortreeCity_Text_TreesGrowByDrinkingRainwater:
	.string "A terra absorve a água da chuva, e\n"
	.string "as árvores crescem bebendo essa água…\p"
	.string "A nossa CIDADE DE FORTREE existe\n"
	.string "porque tem água e terra.$"

FortreeCity_Text_EveryoneHealthyAndLively:
	.string "A CIDADE é feita de casas em cima\n"
	.string "das árvores.\p"
	.string "Talvez por esse jeito de viver,\n"
	.string "todo mundo é saudável e animado.\p"
	.string "Até eu me sinto trinta anos mais\n"
	.string "jovem.$"

FortreeCity_Text_BugPokemonComeThroughWindow:
	.string "Morar em cima das árvores é bom.\p"
	.string "Mas às vezes entram POKéMON INSETO\n"
	.string "pela janela.\l"
	.string "É um susto e tanto.$"

FortreeCity_Text_PokemonThatEvolveWhenTraded:
	.string "Tem POKéMON que evoluem quando você\n"
	.string "os troca! Foi o que eu ouvi.$"

FortreeCity_Text_SomethingUnseeable:
	.string "Tem alguma coisa invisível no caminho.$"

FortreeCity_Text_UnseeableUseDevonScope:
	.string "Tem alguma coisa invisível no caminho.\p"
	.string "Quer usar o ESCOPO DEVON?$"

FortreeCity_Text_UsedDevonScopePokemonFled:
	.string "{PLAYER} usou o ESCOPO DEVON.\p"
	.string "Um POKéMON invisível ficou\n"
	.string "completamente visível!\p"
	.string "O POKéMON se assustou e fugiu!$"

FortreeCity_Text_CitySign:
	.string "CIDADE DE FORTREE\n"
	.string "“A cidade na copa das árvores que\l"
	.string "brinca com a natureza.”$"

FortreeCity_Text_GymSign:
	.string "GINÁSIO POKéMON DE FORTREE\n"
	.string "LÍDER: WINONA\p"
	.string "“A domadora de pássaros que alça\n"
	.string "voo pelo mundo.”$"

