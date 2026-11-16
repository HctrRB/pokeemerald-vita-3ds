Route111_WinstrateFamilysHouse_MapScripts::
	.byte 0

Route111_WinstrateFamilysHouse_EventScript_Victor::
	lock
	faceplayer
	setvar VAR_0x8008, LOCALID_WINSTRATE_HOUSE_VICTOR
	msgbox Route111_WinstrateFamilysHouse_Text_MySonIsStrongerThanYou, MSGBOX_DEFAULT
	goto Route111_WinstrateFamilysHouse_EventScript_FaceOriginalDirection
	end

Route111_WinstrateFamilysHouse_EventScript_Victoria::
	lock
	faceplayer
	setvar VAR_0x8008, LOCALID_WINSTRATE_HOUSE_VICTORIA
	goto_if_set FLAG_RECEIVED_MACHO_BRACE, Route111_WinstrateFamilysHouse_EventScript_ReceivedMachoBrace
	msgbox Route111_WinstrateFamilysHouse_Text_LikeYouToHaveMachoBrace, MSGBOX_DEFAULT
	giveitem ITEM_MACHO_BRACE
	goto_if_eq VAR_RESULT, FALSE, Common_EventScript_ShowBagIsFull
	setflag FLAG_RECEIVED_MACHO_BRACE
	goto Route111_WinstrateFamilysHouse_EventScript_FaceOriginalDirection
	end

Route111_WinstrateFamilysHouse_EventScript_ReceivedMachoBrace::
	msgbox Route111_WinstrateFamilysHouse_Text_PassionateAboutBattles, MSGBOX_DEFAULT
	goto Route111_WinstrateFamilysHouse_EventScript_FaceOriginalDirection
	end

Route111_WinstrateFamilysHouse_EventScript_Vivi::
	lock
	faceplayer
	setvar VAR_0x8008, LOCALID_WINSTRATE_HOUSE_VIVI
	msgbox Route111_WinstrateFamilysHouse_Text_StrongerFamilyMembers, MSGBOX_DEFAULT
	goto Route111_WinstrateFamilysHouse_EventScript_FaceOriginalDirection
	end

Route111_WinstrateFamilysHouse_EventScript_Vicky::
	lock
	faceplayer
	setvar VAR_0x8008, LOCALID_WINSTRATE_HOUSE_VICKY
	goto_if_set FLAG_TEMP_4, Route111_WinstrateFamilysHouse_EventScript_AlreadySpokenTo
	msgbox Route111_WinstrateFamilysHouse_Text_GrandsonStrong, MSGBOX_DEFAULT
	setflag FLAG_TEMP_4
	goto Route111_WinstrateFamilysHouse_EventScript_FaceOriginalDirection
	end

Route111_WinstrateFamilysHouse_EventScript_AlreadySpokenTo::
	msgbox Route111_WinstrateFamilysHouse_Text_GrandsonStrongShort, MSGBOX_DEFAULT
	goto Route111_WinstrateFamilysHouse_EventScript_FaceOriginalDirection
	end

Route111_WinstrateFamilysHouse_EventScript_FaceOriginalDirection::
	closemessage
	applymovement VAR_0x8008, Common_Movement_FaceOriginalDirection
	waitmovement 0
	release
	end

Route111_WinstrateFamilysHouse_Text_MySonIsStrongerThanYou:
	.string "Nunca vi um TREINADOR usar POKéMON\n"
	.string "com tanta habilidade.\p"
	.string "Mas fique sabendo: o meu filho é\n"
	.string "mais forte que você.\p"
	.string "Ele até desafiou a LIGA POKéMON,\n"
	.string "viu?$"

Route111_WinstrateFamilysHouse_Text_LikeYouToHaveMachoBrace:
	.string "A gente usa esta PULSEIRA MACHO pra\n"
	.string "fortalecer melhor os POKéMON\l"
	.string "no treino.\p"
	.string "Você venceu todos nós, então não sei\n"
	.string "se precisa, mas queremos que\l"
	.string "fique com a nossa PULSEIRA MACHO.$"

Route111_WinstrateFamilysHouse_Text_PassionateAboutBattles:
	.string "Quando o assunto é batalha POKéMON,\n"
	.string "a gente se empolga mesmo.$"

Route111_WinstrateFamilysHouse_Text_StrongerFamilyMembers:
	.string "A mamãe é mais forte que o papai.\p"
	.string "Eu sou mais forte que a mamãe.\p"
	.string "E a vovó é mais forte que eu!\p"
	.string "Mas o meu irmão mais velho é mais\n"
	.string "forte que a vovó.$"

Route111_WinstrateFamilysHouse_Text_GrandsonStrong:
	.string "Não há dúvida de que você é forte.\p"
	.string "Mas, se enfrentasse o meu neto,\n"
	.string "acabaria chorando de raiva.\p"
	.string "Ele é mais forte que qualquer\n"
	.string "TREINADOR que a gente conhece.\p"
	.string "A esta altura, deve estar desafiando\n"
	.string "o CAMPEÃO da LIGA POKéMON.\p"
	.string "Conhecendo o meu neto, ele já pode\n"
	.string "ser o CAMPEÃO!$"

Route111_WinstrateFamilysHouse_Text_GrandsonStrongShort:
	.string "A esta altura, o meu neto deve estar\n"
	.string "desafiando o CAMPEÃO da LIGA POKéMON.\p"
	.string "Conhecendo o meu neto, ele já pode\n"
	.string "ser o CAMPEÃO!$"

