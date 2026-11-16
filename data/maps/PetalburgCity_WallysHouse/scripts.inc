PetalburgCity_WallysHouse_MapScripts::
	map_script MAP_SCRIPT_ON_FRAME_TABLE, PetalburgCity_WallysHouse_OnFrame
	map_script MAP_SCRIPT_ON_WARP_INTO_MAP_TABLE, PetalburgCity_WallysHouse_OnWarp
	.byte 0

PetalburgCity_WallysHouse_OnWarp:
	map_script_2 VAR_PETALBURG_CITY_STATE, 4, PetalburgCity_WallysHouse_EventScript_PlayerWallysDadFaceEachOther
	.short 0

PetalburgCity_WallysHouse_EventScript_PlayerWallysDadFaceEachOther::
	turnobject LOCALID_PLAYER, DIR_EAST
	turnobject LOCALID_WALLYS_HOUSE_WALLYS_DAD, DIR_WEST
	end

PetalburgCity_WallysHouse_OnFrame:
	map_script_2 VAR_PETALBURG_CITY_STATE, 4, PetalburgCity_WallysHouse_EventScript_GiveHMSurf
	.short 0

PetalburgCity_WallysHouse_EventScript_GiveHMSurf::
	lockall
	msgbox PetalburgCity_WallysHouse_Text_PleaseExcuseUs, MSGBOX_DEFAULT
	giveitem ITEM_HM_SURF
	setflag FLAG_RECEIVED_HM_SURF
	msgbox PetalburgCity_WallysHouse_Text_SurfGoAllSortsOfPlaces, MSGBOX_DEFAULT
	setvar VAR_PETALBURG_CITY_STATE, 5
	releaseall
	end

PetalburgCity_WallysHouse_EventScript_WallysDad::
	lock
	faceplayer
	goto_if_set FLAG_DEFEATED_WALLY_VICTORY_ROAD, PetalburgCity_WallysHouse_EventScript_DefeatedWallyInVictoryRoad
	goto_if_set FLAG_RECEIVED_HM_SURF, PetalburgCity_WallysHouse_EventScript_ReceievedHMSurf
	goto_if_set FLAG_THANKED_FOR_PLAYING_WITH_WALLY, PetalburgCity_WallysHouse_EventScript_PlayedWithWally
	msgbox PetalburgCity_WallysHouse_Text_ThanksForPlayingWithWally, MSGBOX_DEFAULT
	setflag FLAG_THANKED_FOR_PLAYING_WITH_WALLY
	release
	end

PetalburgCity_WallysHouse_EventScript_ReceievedHMSurf::
	msgbox PetalburgCity_WallysHouse_Text_WallyIsComingHomeSoon, MSGBOX_DEFAULT
	release
	end

PetalburgCity_WallysHouse_EventScript_DefeatedWallyInVictoryRoad::
	msgbox PetalburgCity_WallysHouse_Text_YouMetWallyInEverGrandeCity, MSGBOX_DEFAULT
	release
	end

PetalburgCity_WallysHouse_EventScript_PlayedWithWally::
	msgbox PetalburgCity_WallysHouse_Text_WonderHowWallyIsDoing, MSGBOX_DEFAULT
	release
	end

PetalburgCity_WallysHouse_EventScript_WallysMom::
	lock
	faceplayer
	goto_if_set FLAG_RECEIVED_HM_SURF, PetalburgCity_WallysHouse_EventScript_ReceivedHMSurf
	msgbox PetalburgCity_WallysHouse_Text_WallyWasReallyHappy, MSGBOX_DEFAULT
	release
	end

PetalburgCity_WallysHouse_EventScript_ReceivedHMSurf::
	msgbox PetalburgCity_WallysHouse_Text_WallyLeftWithoutTelling, MSGBOX_DEFAULT
	release
	end

PetalburgCity_WallysHouse_Text_ThanksForPlayingWithWally:
	.string "Você é…\n"
	.string "Ah, você deve ser {PLAYER}{KUN}, né?\p"
	.string "Obrigado por brincar com o WALLY\n"
	.string "agora há pouco.\p"
	.string "Ele é frágil e doente desde\n"
	.string "que era bebê.\p"
	.string "Mandamos ele ficar uns tempos com\n"
	.string "parentes na CIDADE DE VERDANTURF.\p"
	.string "O ar lá é muito mais limpo\n"
	.string "do que aqui.\p"
	.string "Como? Cadê o WALLY?\n"
	.string "Ele já foi, o nosso WALLY.\p"
	.string "Onde será que ele já\n"
	.string "chegou a essa altura?$"

PetalburgCity_WallysHouse_Text_WonderHowWallyIsDoing:
	.string "Como será que está o nosso WALLY?$"

PetalburgCity_WallysHouse_Text_PleaseExcuseUs:
	.string "{PLAYER}{KUN}! Desculpe a gente por\n"
	.string "trazer você aqui desse jeito.\p"
	.string "Mas o nosso WALLY ficou bem saudável\n"
	.string "desde que foi pra VERDANTURF.\p"
	.string "E devemos tudo isso a você!\p"
	.string "Quando o WALLY saiu da cidade, você\n"
	.string "ajudou ele a pegar um POKéMON, né?\p"
	.string "Acho que isso deixou o WALLY muito\n"
	.string "feliz.\p"
	.string "Na verdade, não só o WALLY.\n"
	.string "Me deixou feliz também, como pai.\p"
	.string "Feliz por ele ter ganhado uma amizade\n"
	.string "tão boa quanto a sua.\p"
	.string "Não é suborno nem nada, mas eu queria\n"
	.string "muito que você ficasse com isto.$"

PetalburgCity_WallysHouse_Text_SurfGoAllSortsOfPlaces:
	.string "Se o seu POKéMON souber SURFAR, você\n"
	.string "vai poder ir a tudo quanto é lugar.$"

PetalburgCity_WallysHouse_Text_WallyIsComingHomeSoon:
	.string "O WALLY volta pra casa logo, logo.\n"
	.string "Estou ansioso por isso.$"

PetalburgCity_WallysHouse_Text_YouMetWallyInEverGrandeCity:
	.string "Hã? Você encontrou o WALLY na\n"
	.string "CIDADE DE EVER GRANDE?\p"
	.string "Ah, {PLAYER}{KUN}, deixa de bobagem.\p"
	.string "Ele pode ter ficado saudável, mas não\n"
	.string "consegue ir pra um lugar tão longe\l"
	.string "assim sozinho.$"

PetalburgCity_WallysHouse_Text_WallyWasReallyHappy:
	.string "O WALLY ficou muito feliz quando contou\n"
	.string "pra gente que pegou um POKéMON.\p"
	.string "Fazia tempo que eu não via ele\n"
	.string "sorrir daquele jeito.$"

PetalburgCity_WallysHouse_Text_WallyLeftWithoutTelling:
	.string "Quero que isso fique em segredo\n"
	.string "do meu marido…\p"
	.string "Mas o nosso WALLY saiu da CIDADE DE\n"
	.string "VERDANTURF sem avisar ninguém.\p"
	.string "Sabe, o WALLY é frágil, mas ele\n"
	.string "é surpreendentemente teimoso.\p"
	.string "Tenho certeza de que um dia ele\n"
	.string "volta são e salvo!$"

