MossdeepCity_House4_MapScripts::
	.byte 0

MossdeepCity_House4_EventScript_Woman::
	lock
	faceplayer
	goto_if_set FLAG_SYS_GAME_CLEAR, MossdeepCity_House4_EventScript_CanBattleAtSecretBases
	msgbox MossdeepCity_House4_Text_BrotherLikesToFindBases, MSGBOX_DEFAULT
	release
	end

MossdeepCity_House4_EventScript_CanBattleAtSecretBases::
	msgbox MossdeepCity_House4_Text_BrotherLikesToVisitBasesAndBattle, MSGBOX_DEFAULT
	release
	end

MossdeepCity_House4_EventScript_NinjaBoy::
	lock
	faceplayer
	special CheckPlayerHasSecretBase
	goto_if_eq VAR_RESULT, FALSE, MossdeepCity_House4_EventScript_NoSecretBase
	special GetSecretBaseNearbyMapName
	msgbox MossdeepCity_House4_Text_YouMadeSecretBaseNearX, MSGBOX_DEFAULT
	release
	end

MossdeepCity_House4_EventScript_NoSecretBase::
	msgbox MossdeepCity_House4_Text_MakeSecretBase, MSGBOX_DEFAULT
	release
	end

MossdeepCity_House4_EventScript_Skitty::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_SKITTY, CRY_MODE_NORMAL
	msgbox MossdeepCity_House4_Text_Skitty, MSGBOX_DEFAULT
	waitmoncry
	release
	end

MossdeepCity_House4_Text_BrotherLikesToFindBases:
	.string "O meu irmão mais novo diz que gosta\n"
	.string "de achar as BASES SECRETAS dos outros.$"

MossdeepCity_House4_Text_BrotherLikesToVisitBasesAndBattle:
	.string "O meu irmão mais novo diz que gosta\n"
	.string "de visitar BASES SECRETAS e fazer\l"
	.string "batalhas POKéMON.$"

MossdeepCity_House4_Text_YouMadeSecretBaseNearX:
	.string "Foi você que fez uma BASE SECRETA\n"
	.string "perto de {STR_VAR_1}?$"

MossdeepCity_House4_Text_MakeSecretBase:
	.string "Faça uma BASE SECRETA em algum\n"
	.string "lugar. Eu vou achar!$"

MossdeepCity_House4_Text_Skitty:
	.string "SKITTY: Miaaau?$"

