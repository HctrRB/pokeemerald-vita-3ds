MossdeepCity_GameCorner_1F_MapScripts::
	map_script MAP_SCRIPT_ON_FRAME_TABLE, MossdeepCity_GameCorner_1F_OnFrame
	map_script MAP_SCRIPT_ON_WARP_INTO_MAP_TABLE, MossdeepCity_GameCorner_1F_OnWarp
	map_script MAP_SCRIPT_ON_LOAD, CableClub_OnLoad
	.byte 0

MossdeepCity_GameCorner_1F_OnWarp:
	map_script_2 VAR_CABLE_CLUB_STATE, USING_MINIGAME, CableClub_EventScript_CheckTurnAttendant
	.short 0

MossdeepCity_GameCorner_1F_OnFrame:
	map_script_2 VAR_CABLE_CLUB_STATE, USING_MINIGAME, CableClub_EventScript_ExitMinigameRoom
	.short 0

/* Script is redundant, the label in the goto also does lock and faceplayer*/
MossdeepCity_GameCorner_1F_EventScript_InfoMan::
	lock
	faceplayer
	goto MossdeepCity_GameCorner_1F_EventScript_InfoMan2
	release
	end

/* Script is redundant, the label in the goto also does lock and faceplayer*/
MossdeepCity_GameCorner_1F_EventScript_OldMan::
	lock
	faceplayer
	goto MossdeepCity_GameCorner_1F_EventScript_OldMan2
	release
	end


/* Everything below is leftover from RS*/
/* The house that was here in RS was moved to Sootopolis and replaced by the Game Corner*/
/* The text below is unused and duplicated in its replacement in Sootopolis City*/
/* And the BG Door event was moved inaccessibly into a wall*/

RS_MysteryEventsHouse_EventScript_Door::
	msgbox RS_MysteryEventsHouse_Text_DoorIsLocked, MSGBOX_SIGN
	end

RS_MysteryEventsHouse_Text_OldManGreeting:
	.string "Quando eu era jovem, viajei o mundo\n"
	.string "como TREINADOR POKéMON.\p"
	.string "Agora que virei um velho caduco, a\n"
	.string "minha única diversão é ver jovens\l"
	.string "TREINADORES batalhando.$"

RS_MysteryEventsHouse_Text_DoorIsLocked:
	.string "A porta parece trancada.$"

RS_MysteryEventsHouse_Text_ChallengeVisitingTrainer:
	.string "Um TREINADOR chamado {STR_VAR_1}\n"
	.string "está visitando a minha casa.\p"
	.string "Quer desafiar {STR_VAR_1}?$"

RS_MysteryEventsHouse_Text_YouWontBattle:
	.string "Não vai batalhar? Que pena não\n"
	.string "poder ver você batalhar…$"

RS_MysteryEventsHouse_Text_KeepItToA3On3:
	.string "Ah, que bom, que bom!\p"
	.string "Mas a minha casa não é tão firme.\p"
	.string "Posso pedir que seja só uma\n"
	.string "batalha de 3 contra 3?$"

RS_MysteryEventsHouse_Text_SaveYourProgress:
	.string "Antes de vocês batalharem, é melhor\n"
	.string "salvar o jogo.$"

RS_MysteryEventsHouse_Text_HopeToSeeAGoodMatch:
	.string "Espero ver uma boa batalha!$"

RS_MysteryEventsHouse_Text_BattleTie:
	.string "Então deu empate.\p"
	.string "Foi uma batalha brilhante em que\n"
	.string "ninguém cedeu um passo!$"

RS_MysteryEventsHouse_Text_BattleWon:
	.string "Foi magnífico!\p"
	.string "Até parecia eu mesmo na minha\n"
	.string "juventude!$"

RS_MysteryEventsHouse_Text_BattleLost:
	.string "Ah, que pena!\p"
	.string "Mas foi uma boa batalha.\n"
	.string "Espero que ganhe da próxima vez.$"
