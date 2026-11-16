Route119_WeatherInstitute_1F_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, Route119_WeatherInstitute_1F_OnTransition
	.byte 0

Route119_WeatherInstitute_1F_OnTransition:
	call_if_eq VAR_WEATHER_INSTITUTE_STATE, 0, Route119_WeatherInstitute_1F_EventScript_SetLittleBoyPos
	end

Route119_WeatherInstitute_1F_EventScript_SetLittleBoyPos::
	setobjectxyperm LOCALID_WEATHER_INSTITUTE_LITTLE_BOY, 0, 5
	setobjectmovementtype LOCALID_WEATHER_INSTITUTE_LITTLE_BOY, MOVEMENT_TYPE_FACE_RIGHT
	return

Route119_WeatherInstitute_1F_EventScript_LittleBoy::
	lock
	faceplayer
	special GetPlayerBigGuyGirlString
	goto_if_eq VAR_WEATHER_INSTITUTE_STATE, 0, Route119_WeatherInstitute_1F_EventScript_LittleBoyTeamAquaHere
	msgbox Route119_WeatherInstitute_1F_Text_WowYoureStrong, MSGBOX_DEFAULT
	release
	end

Route119_WeatherInstitute_1F_EventScript_LittleBoyTeamAquaHere::
	msgbox Route119_WeatherInstitute_1F_Text_EveryoneWentUpstairs, MSGBOX_DEFAULT
	release
	end

Route119_WeatherInstitute_1F_EventScript_InstituteWorker1::
	lock
	faceplayer
	goto_if_unset FLAG_SYS_GAME_CLEAR, Route119_WeatherInstitute_1F_EventScript_StudyingRain
	setvar VAR_0x8004, 0
	call_if_set FLAG_DEFEATED_KYOGRE, Route119_WeatherInstitute_1F_EventScript_LegendaryDefeated
	call_if_set FLAG_DEFEATED_GROUDON, Route119_WeatherInstitute_1F_EventScript_LegendaryDefeated
	goto_if_eq VAR_0x8004, 2, Route119_WeatherInstitute_1F_EventScript_StudyingRain  /* Both defeated*/
	msgbox Route119_WeatherInstitute_1F_Text_NoticingAbnormalWeather, MSGBOX_DEFAULT
	release
	end

Route119_WeatherInstitute_1F_EventScript_LegendaryDefeated::
	addvar VAR_0x8004, 1
	return

Route119_WeatherInstitute_1F_EventScript_StudyingRain::
	msgbox Route119_WeatherInstitute_1F_Text_ProfStudyingRain, MSGBOX_DEFAULT
	release
	end

Route119_WeatherInstitute_1F_EventScript_InstituteWorker2::
	msgbox Route119_WeatherInstitute_1F_Text_WhatWereAquasUpTo, MSGBOX_NPC
	end

Route119_WeatherInstitute_1F_EventScript_Bed::
	lockall
	msgbox Route119_WeatherInstitute_1F_Text_TakeRestInBed, MSGBOX_DEFAULT
	closemessage
	call Common_EventScript_OutOfCenterPartyHeal
	releaseall
	end

Route119_WeatherInstitute_1F_EventScript_Grunt1::
	trainerbattle_single TRAINER_GRUNT_WEATHER_INST_1, Route119_WeatherInstitute_1F_Text_Grunt1Intro, Route119_WeatherInstitute_1F_Text_Grunt1Defeat
	msgbox Route119_WeatherInstitute_1F_Text_Grunt1PostBattle, MSGBOX_AUTOCLOSE
	end

Route119_WeatherInstitute_1F_EventScript_Grunt4::
	trainerbattle_single TRAINER_GRUNT_WEATHER_INST_4, Route119_WeatherInstitute_1F_Text_Grunt4Intro, Route119_WeatherInstitute_1F_Text_Grunt4Defeat
	msgbox Route119_WeatherInstitute_1F_Text_Grunt4PostBattle, MSGBOX_AUTOCLOSE
	end

Route119_WeatherInstitute_1F_Text_Grunt1Intro:
	.string "O CHEFE se interessou pela pesquisa\n"
	.string "que fazem aqui e mandou a gente.\p"
	.string "Pare de se meter!$"

Route119_WeatherInstitute_1F_Text_Grunt1Defeat:
	.string "Droga…\n"
	.string "Derrotado por uma criança…$"

Route119_WeatherInstitute_1F_Text_Grunt1PostBattle:
	.string "O nosso CHEFE sabe de tudo.\p"
	.string "Mas eu sou só um CAPANGA. Como vou\n"
	.string "saber o que ele está pensando?$"

Route119_WeatherInstitute_1F_Text_Grunt4Intro:
	.string "Hã?\n"
	.string "O que uma criança faz aqui?$"

Route119_WeatherInstitute_1F_Text_Grunt4Defeat:
	.string "Hã?\n"
	.string "Eu perdi?!$"

Route119_WeatherInstitute_1F_Text_Grunt4PostBattle:
	.string "Ah, não…\n"
	.string "Vou levar bronca por perder pra\l"
	.string "uma criança…\p"
	.string "Vou é tirar um cochilo na cama…$"

Route119_WeatherInstitute_1F_Text_EveryoneWentUpstairs:
	.string "Enquanto eu dormia, todo mundo\n"
	.string "subiu!$"

Route119_WeatherInstitute_1F_Text_WowYoureStrong:
	.string "Uau, você é forte demais!\p"
	.string "Queria ser TREINADOR POKéMON\n"
	.string "que nem você!$"

Route119_WeatherInstitute_1F_Text_ProfStudyingRain:
	.string "O PROFESSOR ama a chuva.\n"
	.string "Isso é fato.\p"
	.string "Mas, se não parar de chover, o povo\n"
	.string "se complica. Isso também é fato.\p"
	.string "Por isso, o PROFESSOR estuda se dá\n"
	.string "pra aproveitar a chuva pra algo bom.$"

Route119_WeatherInstitute_1F_Text_NoticingAbnormalWeather:
	.string "No 2º andar do INSTITUTO, estudamos\n"
	.string "o clima da região de HOENN.\p"
	.string "Ultimamente, notamos casos isolados\n"
	.string "e passageiros de seca e de chuva\l"
	.string "forte…$"

Route119_WeatherInstitute_1F_Text_WhatWereAquasUpTo:
	.string "Olá!\n"
	.string "Você salvou a gente!\p"
	.string "Só não entendo o que aquela\n"
	.string "EQUIPE AQUA estava aprontando.$"

Route119_WeatherInstitute_1F_Text_TakeRestInBed:
	.string "Tem uma cama…\n"
	.string "Vamos descansar.$"

