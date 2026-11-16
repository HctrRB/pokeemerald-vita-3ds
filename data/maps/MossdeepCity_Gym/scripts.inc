MossdeepCity_Gym_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, MossdeepCity_Gym_OnLoad
	.byte 0

/* NOTE: Mossdeep Gym was redesigned between R/S and E. Leftover (and now functionally unused) scripts are commented below*/

/* All the below checks are leftover from RS. FLAG_MOSSDEEP_GYM_SWITCH_X is never set*/
MossdeepCity_Gym_OnLoad:
	goto_if_set FLAG_MOSSDEEP_GYM_SWITCH_1, MossdeepCity_Gym_EventScript_SetSwitch1Metatiles
	goto MossdeepCity_Gym_EventScript_CheckSwitch2
	end

MossdeepCity_Gym_EventScript_CheckSwitch2::
	goto_if_set FLAG_MOSSDEEP_GYM_SWITCH_2, MossdeepCity_Gym_EventScript_SetSwitch2Metatiles
	goto MossdeepCity_Gym_EventScript_CheckSwitch3
	end

MossdeepCity_Gym_EventScript_CheckSwitch3::
	goto_if_set FLAG_MOSSDEEP_GYM_SWITCH_3, MossdeepCity_Gym_EventScript_SetSwitch3Metatiles
	goto MossdeepCity_Gym_EventScript_CheckSwitch4
	end

MossdeepCity_Gym_EventScript_CheckSwitch4::
	goto_if_set FLAG_MOSSDEEP_GYM_SWITCH_4, MossdeepCity_Gym_EventScript_SetSwitch4Metatiles
	end

/* All the below set metatile scripts are leftover from RS and are functionally unused*/
MossdeepCity_Gym_EventScript_SetSwitch1Metatiles::
	setmetatile 5, 5, METATILE_RSMossdeepGym_RedArrow_Right, FALSE
	setmetatile 2, 7, METATILE_RSMossdeepGym_Switch_Down, TRUE
	goto MossdeepCity_Gym_EventScript_CheckSwitch2
	end

MossdeepCity_Gym_EventScript_SetSwitch2Metatiles::
	setmetatile 8, 14, METATILE_RSMossdeepGym_RedArrow_Right, FALSE
	setmetatile 8, 10, METATILE_RSMossdeepGym_Switch_Down, TRUE
	goto MossdeepCity_Gym_EventScript_CheckSwitch3
	end

MossdeepCity_Gym_EventScript_SetSwitch3Metatiles::
	setmetatile 15, 17, METATILE_RSMossdeepGym_RedArrow_Left, FALSE
	setmetatile 17, 15, METATILE_RSMossdeepGym_Switch_Down, TRUE
	goto MossdeepCity_Gym_EventScript_CheckSwitch4
	end

MossdeepCity_Gym_EventScript_SetSwitch4Metatiles::
	setmetatile 1, 23, METATILE_RSMossdeepGym_RedArrow_Up, FALSE
	setmetatile 5, 24, METATILE_RSMossdeepGym_Switch_Down, TRUE
	end

MossdeepCity_Gym_EventScript_TateAndLiza::
	trainerbattle_double TRAINER_TATE_AND_LIZA_1, MossdeepCity_Gym_Text_TateAndLizaIntro, MossdeepCity_Gym_Text_TateAndLizaDefeat, MossdeepCity_Gym_Text_TateAndLizaNeedTwoMons, MossdeepCity_Gym_EventScript_TateAndLizaDefeated, NO_MUSIC
	specialvar VAR_RESULT, ShouldTryRematchBattle
	goto_if_eq VAR_RESULT, TRUE, MossdeepCity_Gym_EventScript_TateAndLizaRematch
	goto_if_unset FLAG_RECEIVED_TM_CALM_MIND, MossdeepCity_Gym_EventScript_GiveCalmMind2
	msgbox MossdeepCity_Gym_Text_TateAndLizaPostBattle, MSGBOX_DEFAULT
	release
	end

MossdeepCity_Gym_EventScript_TateAndLizaDefeated::
	message MossdeepCity_Gym_Text_ReceivedMindBadge
	waitmessage
	call Common_EventScript_PlayGymBadgeFanfare
	msgbox MossdeepCity_Gym_Text_ExplainMindBadgeTakeThis, MSGBOX_DEFAULT
	setflag FLAG_DEFEATED_MOSSDEEP_GYM
	setflag FLAG_BADGE07_GET
	setflag FLAG_HIDE_AQUA_HIDEOUT_GRUNTS
	clearflag FLAG_HIDE_SLATEPORT_CITY_STERNS_SHIPYARD_MR_BRINEY
	clearflag FLAG_UNKNOWN_0x393
	setvar VAR_0x8008, 7
	call Common_EventScript_SetGymTrainers
	clearflag FLAG_HIDE_SLATEPORT_CITY_HARBOR_PATRONS
	clearflag FLAG_HIDE_MOSSDEEP_CITY_TEAM_MAGMA
	clearflag FLAG_HIDE_MOSSDEEP_CITY_SPACE_CENTER_1F_TEAM_MAGMA
	clearflag FLAG_HIDE_MOSSDEEP_CITY_SPACE_CENTER_2F_TEAM_MAGMA
	clearflag FLAG_HIDE_MOSSDEEP_CITY_SPACE_CENTER_2F_STEVEN
	setvar VAR_MOSSDEEP_CITY_STATE, 1
	setflag FLAG_HIDE_MOSSDEEP_CITY_SPACE_CENTER_1F_STEVEN
	setvar VAR_MOSSDEEP_SPACE_CENTER_STATE, 1
	call MossdeepCity_Gym_EventScript_GiveCalmMind
	closemessage
	delay 30
	playfanfare MUS_REGISTER_MATCH_CALL
	msgbox MossdeepCity_Gym_Text_RegisteredTateAndLiza, MSGBOX_DEFAULT
	waitfanfare
	closemessage
	delay 30
	setflag FLAG_ENABLE_TATE_AND_LIZA_MATCH_CALL
	release
	end

MossdeepCity_Gym_EventScript_GiveCalmMind2::
	giveitem ITEM_TM_CALM_MIND
	goto_if_eq VAR_RESULT, FALSE, Common_EventScript_ShowBagIsFull
	msgbox MossdeepCity_Gym_Text_ExplainCalmMind, MSGBOX_DEFAULT
	setflag FLAG_RECEIVED_TM_CALM_MIND
	release
	end

MossdeepCity_Gym_EventScript_GiveCalmMind::
	giveitem ITEM_TM_CALM_MIND
	goto_if_eq VAR_RESULT, FALSE, Common_EventScript_BagIsFull
	msgbox MossdeepCity_Gym_Text_ExplainCalmMind, MSGBOX_DEFAULT
	setflag FLAG_RECEIVED_TM_CALM_MIND
	return

MossdeepCity_Gym_EventScript_TateAndLizaRematch::
	trainerbattle_rematch_double TRAINER_TATE_AND_LIZA_1, MossdeepCity_Gym_Text_TateAndLizaPreRematch, MossdeepCity_Gym_Text_TateAndLizaRematchDefeat, MossdeepCity_Gym_Text_TateAndLizaRematchNeedTwoMons
	msgbox MossdeepCity_Gym_Text_TateAndLizaPostRematch, MSGBOX_AUTOCLOSE
	end

/* All the below switch scripts are leftover from RS and are functionally unused*/
MossdeepCity_Gym_EventScript_Switch1::
	lockall
	goto_if_set FLAG_MOSSDEEP_GYM_SWITCH_1, MossdeepCity_Gym_EventScript_ClearSwitch1
	setflag FLAG_MOSSDEEP_GYM_SWITCH_1
	applymovement LOCALID_PLAYER, MossdeepCity_Gym_Movement_WaitAfterSwitchUse
	waitmovement 0
	setmetatile 5, 5, METATILE_RSMossdeepGym_RedArrow_Right, FALSE
	setmetatile 2, 7, METATILE_RSMossdeepGym_Switch_Down, TRUE
	goto MossdeepCity_Gym_EventScript_DrawMapAfterSwitchUsed
	end

MossdeepCity_Gym_EventScript_DrawMapAfterSwitchUsed::
	special DrawWholeMapView
	playse SE_CLICK
	releaseall
	end

MossdeepCity_Gym_EventScript_ClearSwitch1::
	clearflag FLAG_MOSSDEEP_GYM_SWITCH_1
	applymovement LOCALID_PLAYER, MossdeepCity_Gym_Movement_WaitAfterSwitchUse
	waitmovement 0
	setmetatile 5, 5, METATILE_RSMossdeepGym_RedArrow_Left, FALSE
	setmetatile 2, 7, METATILE_RSMossdeepGym_Switch_Up, TRUE
	goto MossdeepCity_Gym_EventScript_DrawMapAfterSwitchUsed
	end

MossdeepCity_Gym_EventScript_Switch2::
	lockall
	goto_if_set FLAG_MOSSDEEP_GYM_SWITCH_2, MossdeepCity_Gym_EventScript_ClearSwitch2
	setflag FLAG_MOSSDEEP_GYM_SWITCH_2
	applymovement LOCALID_PLAYER, MossdeepCity_Gym_Movement_WaitAfterSwitchUse
	waitmovement 0
	setmetatile 8, 14, METATILE_RSMossdeepGym_RedArrow_Right, FALSE
	setmetatile 8, 10, METATILE_RSMossdeepGym_Switch_Down, TRUE
	goto MossdeepCity_Gym_EventScript_DrawMapAfterSwitchUsed
	end

MossdeepCity_Gym_EventScript_ClearSwitch2::
	clearflag FLAG_MOSSDEEP_GYM_SWITCH_2
	applymovement LOCALID_PLAYER, MossdeepCity_Gym_Movement_WaitAfterSwitchUse
	waitmovement 0
	setmetatile 8, 14, METATILE_RSMossdeepGym_RedArrow_Down, FALSE
	setmetatile 8, 10, METATILE_RSMossdeepGym_Switch_Up, TRUE
	goto MossdeepCity_Gym_EventScript_DrawMapAfterSwitchUsed
	end

MossdeepCity_Gym_EventScript_Switch3::
	lockall
	goto_if_set FLAG_MOSSDEEP_GYM_SWITCH_3, MossdeepCity_Gym_EventScript_ClearSwitch3
	setflag FLAG_MOSSDEEP_GYM_SWITCH_3
	applymovement LOCALID_PLAYER, MossdeepCity_Gym_Movement_WaitAfterSwitchUse
	waitmovement 0
	setmetatile 15, 17, METATILE_RSMossdeepGym_RedArrow_Left, FALSE
	setmetatile 17, 15, METATILE_RSMossdeepGym_Switch_Down, TRUE
	goto MossdeepCity_Gym_EventScript_DrawMapAfterSwitchUsed
	end

MossdeepCity_Gym_EventScript_ClearSwitch3::
	clearflag FLAG_MOSSDEEP_GYM_SWITCH_3
	applymovement LOCALID_PLAYER, MossdeepCity_Gym_Movement_WaitAfterSwitchUse
	waitmovement 0
	setmetatile 15, 17, METATILE_RSMossdeepGym_RedArrow_Right, FALSE
	setmetatile 17, 15, METATILE_RSMossdeepGym_Switch_Up, TRUE
	goto MossdeepCity_Gym_EventScript_DrawMapAfterSwitchUsed
	end

MossdeepCity_Gym_EventScript_Switch4::
	lockall
	goto_if_set FLAG_MOSSDEEP_GYM_SWITCH_4, MossdeepCity_Gym_EventScript_ClearSwitch4
	setflag FLAG_MOSSDEEP_GYM_SWITCH_4
	applymovement LOCALID_PLAYER, MossdeepCity_Gym_Movement_WaitAfterSwitchUse
	waitmovement 0
	setmetatile 1, 23, METATILE_RSMossdeepGym_RedArrow_Up, FALSE
	setmetatile 5, 24, METATILE_RSMossdeepGym_Switch_Down, TRUE
	goto MossdeepCity_Gym_EventScript_DrawMapAfterSwitchUsed
	end

MossdeepCity_Gym_EventScript_ClearSwitch4::
	clearflag FLAG_MOSSDEEP_GYM_SWITCH_4
	applymovement LOCALID_PLAYER, MossdeepCity_Gym_Movement_WaitAfterSwitchUse
	waitmovement 0
	setmetatile 1, 23, METATILE_RSMossdeepGym_RedArrow_Right, FALSE
	setmetatile 5, 24, METATILE_RSMossdeepGym_Switch_Up, TRUE
	goto MossdeepCity_Gym_EventScript_DrawMapAfterSwitchUsed
	end

MossdeepCity_Gym_EventScript_WarpToEntrance::
	lockall
	warpmossdeepgym MAP_MOSSDEEP_CITY_GYM, 7, 30
	waitstate
	releaseall
	end

MossdeepCity_Gym_EventScript_Preston::
	trainerbattle_single TRAINER_PRESTON, MossdeepCity_Gym_Text_PrestonIntro, MossdeepCity_Gym_Text_PrestonDefeat
	msgbox MossdeepCity_Gym_Text_PrestonPostBattle, MSGBOX_AUTOCLOSE
	end

MossdeepCity_Gym_EventScript_Virgil::
	trainerbattle_single TRAINER_VIRGIL, MossdeepCity_Gym_Text_VirgilIntro, MossdeepCity_Gym_Text_VirgilDefeat
	msgbox MossdeepCity_Gym_Text_VirgilPostBattle, MSGBOX_AUTOCLOSE
	end

MossdeepCity_Gym_EventScript_Blake::
	trainerbattle_single TRAINER_BLAKE, MossdeepCity_Gym_Text_BlakeIntro, MossdeepCity_Gym_Text_BlakeDefeat
	msgbox MossdeepCity_Gym_Text_BlakePostBattle, MSGBOX_AUTOCLOSE
	end

MossdeepCity_Gym_EventScript_Hannah::
	trainerbattle_single TRAINER_HANNAH, MossdeepCity_Gym_Text_HannahIntro, MossdeepCity_Gym_Text_HannahDefeat
	msgbox MossdeepCity_Gym_Text_HannahPostBattle, MSGBOX_AUTOCLOSE
	end

MossdeepCity_Gym_EventScript_Samantha::
	trainerbattle_single TRAINER_SAMANTHA, MossdeepCity_Gym_Text_SamanthaIntro, MossdeepCity_Gym_Text_SamanthaDefeat
	msgbox MossdeepCity_Gym_Text_SamanthaPostBattle, MSGBOX_AUTOCLOSE
	end

MossdeepCity_Gym_EventScript_Maura::
	trainerbattle_single TRAINER_MAURA, MossdeepCity_Gym_Text_MauraIntro, MossdeepCity_Gym_Text_MauraDefeat
	msgbox MossdeepCity_Gym_Text_MauraPostBattle, MSGBOX_AUTOCLOSE
	end

MossdeepCity_Gym_EventScript_Sylvia::
	trainerbattle_single TRAINER_SYLVIA, MossdeepCity_Gym_Text_SylviaIntro, MossdeepCity_Gym_Text_SylviaDefeat
	msgbox MossdeepCity_Gym_Text_SylviaPostBattle, MSGBOX_AUTOCLOSE
	end

MossdeepCity_Gym_EventScript_Nate::
	trainerbattle_single TRAINER_NATE, MossdeepCity_Gym_Text_NateIntro, MossdeepCity_Gym_Text_NateDefeat
	msgbox MossdeepCity_Gym_Text_NatePostBattle, MSGBOX_AUTOCLOSE
	end

MossdeepCity_Gym_EventScript_Macey::
	trainerbattle_single TRAINER_MACEY, MossdeepCity_Gym_Text_MaceyIntro, MossdeepCity_Gym_Text_MaceyDefeat
	msgbox MossdeepCity_Gym_Text_MaceyPostBattle, MSGBOX_AUTOCLOSE
	end

MossdeepCity_Gym_EventScript_Clifford::
	trainerbattle_single TRAINER_CLIFFORD, MossdeepCity_Gym_Text_CliffordIntro, MossdeepCity_Gym_Text_CliffordDefeat
	msgbox MossdeepCity_Gym_Text_CliffordPostBattle, MSGBOX_AUTOCLOSE
	end

MossdeepCity_Gym_EventScript_Nicholas::
	trainerbattle_single TRAINER_NICHOLAS, MossdeepCity_Gym_Text_NicholasIntro, MossdeepCity_Gym_Text_NicholasDefeat
	msgbox MossdeepCity_Gym_Text_NicholasPostBattle, MSGBOX_AUTOCLOSE
	end

MossdeepCity_Gym_EventScript_Kathleen::
	trainerbattle_single TRAINER_KATHLEEN, MossdeepCity_Gym_Text_KathleenIntro, MossdeepCity_Gym_Text_KathleenDefeat
	msgbox MossdeepCity_Gym_Text_KathleenPostBattle, MSGBOX_AUTOCLOSE
	end

MossdeepCity_Gym_EventScript_GymGuide::
	lock
	faceplayer
	goto_if_set FLAG_DEFEATED_MOSSDEEP_GYM, MossdeepCity_Gym_EventScript_GymGuidePostVictory
	msgbox MossdeepCity_Gym_Text_GymGuideAdvice, MSGBOX_DEFAULT
	release
	end

MossdeepCity_Gym_EventScript_GymGuidePostVictory::
	msgbox MossdeepCity_Gym_Text_GymGuidePostVictory, MSGBOX_DEFAULT
	release
	end

/* Leftover from RS, functionally unused*/
MossdeepCity_Gym_Movement_WaitAfterSwitchUse:
	delay_16
	delay_16
	step_end

MossdeepCity_Gym_EventScript_LeftGymStatue::
	lockall
	goto_if_set FLAG_BADGE07_GET, MossdeepCity_Gym_EventScript_GymStatueCertified
	goto MossdeepCity_Gym_EventScript_GymStatue
	end

MossdeepCity_Gym_EventScript_RightGymStatue::
	lockall
	goto_if_set FLAG_BADGE07_GET, MossdeepCity_Gym_EventScript_GymStatueCertified
	goto MossdeepCity_Gym_EventScript_GymStatue
	end

MossdeepCity_Gym_EventScript_GymStatueCertified::
	msgbox MossdeepCity_Gym_Text_GymStatueCertified, MSGBOX_DEFAULT
	releaseall
	end

MossdeepCity_Gym_EventScript_GymStatue::
	msgbox MossdeepCity_Gym_Text_GymStatue, MSGBOX_DEFAULT
	releaseall
	end

MossdeepCity_Gym_EventScript_YellowFloorSwitch::
	playse SE_SWITCH
	waitse
	playse SE_REPEL
	initrotatingtilepuzzle FALSE
	moverotatingtileobjects 0
	waitmovement 0
	turnrotatingtileobjects
	waitmovement 0
	freerotatingtilepuzzle
	end

MossdeepCity_Gym_EventScript_BlueFloorSwitch::
	playse SE_SWITCH
	waitse
	playse SE_REPEL
	initrotatingtilepuzzle FALSE
	moverotatingtileobjects 1
	waitmovement 0
	turnrotatingtileobjects
	waitmovement 0
	freerotatingtilepuzzle
	end

MossdeepCity_Gym_EventScript_GreenFloorSwitch::
	playse SE_SWITCH
	waitse
	playse SE_REPEL
	initrotatingtilepuzzle FALSE
	moverotatingtileobjects 2
	waitmovement 0
	turnrotatingtileobjects
	waitmovement 0
	freerotatingtilepuzzle
	end

MossdeepCity_Gym_EventScript_PurpleFloorSwitch::
	playse SE_SWITCH
	waitse
	playse SE_REPEL
	initrotatingtilepuzzle FALSE
	moverotatingtileobjects 3
	waitmovement 0
	turnrotatingtileobjects
	waitmovement 0
	freerotatingtilepuzzle
	end

MossdeepCity_Gym_EventScript_RedFloorSwitch::
	playse SE_SWITCH
	waitse
	playse SE_REPEL
	initrotatingtilepuzzle FALSE
	moverotatingtileobjects 4
	waitmovement 0
	turnrotatingtileobjects
	waitmovement 0
	freerotatingtilepuzzle
	end

MossdeepCity_Gym_Text_GymGuideAdvice:
	.string "E aí, como vai, {PLAYER},\n"
	.string "futura estrela da LIGA?\p"
	.string "Os LÍDERES deste GINÁSIO usam\n"
	.string "POKéMON do tipo PSÍQUICO.\p"
	.string "Se for contra eles com POKéMON de\n"
	.string "LUTADOR ou VENENO, eles vão causar\l"
	.string "um dano terrível!\p"
	.string "E ainda vêm com ataques combinados\n"
	.string "impressionantes!\p"
	.string "Pra ganhar, mostre a eles como você\n"
	.string "é unido aos seus POKéMON. Manda ver!$"

MossdeepCity_Gym_Text_GymGuidePostVictory:
	.string "Uau, você é incrível!\n"
	.string "Que TREINADOR fora de série!$"

MossdeepCity_Gym_Text_PrestonIntro:
	.string "Batalhas dependem da força do\n"
	.string "espírito! Quem tem espírito fraco\l"
	.string "nunca vence!$"

MossdeepCity_Gym_Text_PrestonDefeat:
	.string "Perdi!\n"
	.string "Preciso fortalecer o espírito de novo!$"

MossdeepCity_Gym_Text_PrestonPostBattle:
	.string "Os indecisos perdem.\n"
	.string "Fica o meu aviso.$"

MossdeepCity_Gym_Text_VirgilIntro:
	.string "Mostre o seu talento!$"

MossdeepCity_Gym_Text_VirgilDefeat:
	.string "Que talento espetacular!$"

MossdeepCity_Gym_Text_VirgilPostBattle:
	.string "Se existem prodígios no mundo, são\n"
	.string "os nossos LÍDERES de GINÁSIO!\p"
	.string "Mas talvez você tenha ainda mais\n"
	.string "talento que eles…$"

MossdeepCity_Gym_Text_BlakeIntro:
	.string "Fufufufu… Veja eu levitar uma POKé\n"
	.string "BOLA com telecinese!\p"
	.string "Uoooooooooaaar!\n"
	.string "… … … … … … …\p"
	.string "Às vezes eu saio de sintonia…$"

MossdeepCity_Gym_Text_BlakeDefeat:
	.string "As minhas batalhas POKéMON também\n"
	.string "saíram de sintonia!$"

MossdeepCity_Gym_Text_BlakePostBattle:
	.string "A POKé BOLA era pesada demais pra\n"
	.string "levantar com a mente. Mas esta bola\l"
	.string "de poeira…\p"
	.string "Fuuuuuuuuuuuuuh!\n"
	.string "… … … … … …\p"
	.string "Não, não estou roubando!\n"
	.string "Não assoprei! Juro!$"

MossdeepCity_Gym_Text_HannahIntro:
	.string "Quando perder pra mim, não se cobre\n"
	.string "demais.\p"
	.string "Não é fraqueza sua: eu é que sou\n"
	.string "forte demais!$"

MossdeepCity_Gym_Text_HannahDefeat:
	.string "Você é forte demais!$"

MossdeepCity_Gym_Text_HannahPostBattle:
	.string "Não vou me abater por perder.\n"
	.string "Só preciso treinar mais.\p"
	.string "Pensamento positivo: esse é o meu\n"
	.string "ponto forte!$"

MossdeepCity_Gym_Text_SamanthaIntro:
	.string "Estou vendo…\n"
	.string "Estou vendo o seu futuro…\p"
	.string "Se me vencer, eu conto o seu futuro!$"

MossdeepCity_Gym_Text_SamanthaDefeat:
	.string "Eu me rendo!$"

MossdeepCity_Gym_Text_SamanthaPostBattle:
	.string "Você vai travar uma batalha\n"
	.string "intensíssima com os nossos LÍDERES…\p"
	.string "O resultado…\n"
	.string "Não vai ouvir de mim!$"

MossdeepCity_Gym_Text_MauraIntro:
	.string "Os caminhos que você percorreu…\n"
	.string "O que você viveu…\p"
	.string "Venha com tudo o que aprendeu!$"

MossdeepCity_Gym_Text_MauraDefeat:
	.string "Você trilhou um caminho de grandeza!$"

MossdeepCity_Gym_Text_MauraPostBattle:
	.string "Um futuro brilhante espera quem\n"
	.string "trabalhou com dedicação.\p"
	.string "Pra quem ficou à toa, no fim só\n"
	.string "resta o desespero.\p"
	.string "Aqui se faz, aqui se paga…$"

MossdeepCity_Gym_Text_SylviaIntro:
	.string "Nem que custe a vida, vou vencer\n"
	.string "esta batalha!$"

MossdeepCity_Gym_Text_SylviaDefeat:
	.string "O que você faz…\n"
	.string "Me apavora…$"

MossdeepCity_Gym_Text_SylviaPostBattle:
	.string "Como ousa me vencer…\p"
	.string "Não vai ser culpa minha se algo\n"
	.string "terrível te acontecer…$"

MossdeepCity_Gym_Text_NateIntro:
	.string "Hohoho.\p"
	.string "Vou te mostrar como o tipo PSÍQUICO\n"
	.string "pode ser tenaz.$"

MossdeepCity_Gym_Text_NateDefeat:
	.string "Oh! Ora, ora!\n"
	.string "O seu estilo de batalha é fantástico!$"

MossdeepCity_Gym_Text_NatePostBattle:
	.string "Eu… Eu não me a-abalo com derrotas,\n"
	.string "por p-piores que sejam.$"

MossdeepCity_Gym_Text_KathleenIntro:
	.string "Vou te fazer passar um momento\n"
	.string "horrível e apavorante!$"

MossdeepCity_Gym_Text_KathleenDefeat:
	.string "N-nãããão!$"

MossdeepCity_Gym_Text_KathleenPostBattle:
	.string "A sua vitalidade é contagiosa!\n"
	.string "Sai de perto de mim, rápido!$"

MossdeepCity_Gym_Text_CliffordIntro:
	.string "Posso não estar no auge, mas\n"
	.string "sugiro que não me subestime.$"

MossdeepCity_Gym_Text_CliffordDefeat:
	.string "Ah, você transborda o poder da\n"
	.string "juventude!$"

MossdeepCity_Gym_Text_CliffordPostBattle:
	.string "Parece que não consegui superar a\n"
	.string "sua energia jovem.$"

MossdeepCity_Gym_Text_MaceyIntro:
	.string "Você não vai chegar até o fim!\n"
	.string "Não se depender de mim!$"

MossdeepCity_Gym_Text_MaceyDefeat:
	.string "Como você pode ser tão absurdamente\n"
	.string "forte?$"

MossdeepCity_Gym_Text_MaceyPostBattle:
	.string "Hunf! Você pode ser forte, mas não\n"
	.string "combina com o tipo PSÍQUICO!\p"
	.string "O seu jeito de batalhar é meio bruto!$"

MossdeepCity_Gym_Text_NicholasIntro:
	.string "Uoooar! Prove o meu superpoder de\n"
	.string "batalha POKéMON!$"

MossdeepCity_Gym_Text_NicholasDefeat:
	.string "Oh!\n"
	.string "Acabaram comigo!$"

MossdeepCity_Gym_Text_NicholasPostBattle:
	.string "Certo! Acho que agora vou criar um\n"
	.string "superpoder POKéMON invencível.$"

MossdeepCity_Gym_Text_TateAndLizaIntro:
	.string "TATE: Hehehe… Levou um susto?\p"
	.string "LIZA: Fufufu… Levou um susto?\p"
	.string "TATE: Por serem dois LÍDERES?\n"
	.string "LIZA: Por serem dois LÍDERES?\p"
	.string "TATE: Somos gêmeos!\n"
	.string "LIZA: Somos gêmeos!\p"
	.string "TATE: Não precisamos falar porque…\n"
	.string "LIZA: Cada um sabe o que…\p"
	.string "TATE: O outro está pensando…\n"
	.string "LIZA: Tudo na nossa mente!\p"
	.string "TATE: Esta nossa combinação…\n"
	.string "LIZA: Consegue vencer?$"

MossdeepCity_Gym_Text_TateAndLizaDefeat:
	.string "TATE: O quê?! A nossa combinação…\n"
	.string "LIZA: Foi despedaçada!\p"
	.string "TATE: Não tem jeito. Você venceu…\n"
	.string "LIZA: Então, em reconhecimento, tome.$"

MossdeepCity_Gym_Text_ReceivedMindBadge:
	.string "{PLAYER} recebeu a INSÍGNIA DA MENTE\n"
	.string "de TATE e LIZA.$"

MossdeepCity_Gym_Text_ExplainMindBadgeTakeThis:
	.string "TATE: A INSÍGNIA DA MENTE aumenta o\n"
	.string "AT. ESP. e a DEF. ESP. dos POKéMON.\p"
	.string "LIZA: E deixa usar o golpe de MO\n"
	.string "MERGULHO fora da batalha.\p"
	.string "TATE: Leve isto também.$"

MossdeepCity_Gym_Text_ExplainCalmMind:
	.string "TATE: Essa MT04 contém…\n"
	.string "LIZA: MENTE CALMA!\p"
	.string "TATE: Aumenta o AT. ESP. e…\n"
	.string "LIZA: Aumenta a DEF. ESP.!\p"
	.string "TATE: É um golpe perfeito…\n"
	.string "LIZA: Pra POKéMON PSÍQUICOS!\p"
	.string "… … … … … …$"

MossdeepCity_Gym_Text_RegisteredTateAndLiza:
	.string "Os LÍDERES TATE & LIZA foram\n"
	.string "registrados no POKéNAV.$"

MossdeepCity_Gym_Text_TateAndLizaPostBattle:
	.string "TATE: Parece que o laço entre você e\n"
	.string "os seus POKéMON é bem mais forte\l"
	.string "que o nosso laço de gêmeos.\p"
	.string "LIZA: Você vai ficar ainda mais forte!\n"
	.string "Batalhamos com você, então sabemos.$"

MossdeepCity_Gym_Text_TateAndLizaNeedTwoMons:
	.string "TATE: Hehehe… Levou um susto?\p"
	.string "LIZA: Por serem dois LÍDERES?\p"
	.string "TATE: Ops, você só tem um…\n"
	.string "LIZA: POKéMON que pode batalhar.\p"
	.string "TATE: Assim não dá pra batalhar!\p"
	.string "LIZA: Se quiser nos desafiar,\n"
	.string "traga mais POKéMON.$"

MossdeepCity_Gym_Text_GymStatue:
	.string "GINÁSIO POKéMON DE MOSSDEEP$"

MossdeepCity_Gym_Text_GymStatueCertified:
	.string "GINÁSIO POKéMON DE MOSSDEEP\p"
	.string "APROVADOS POR LIZA E TATE:\n"
	.string "{PLAYER}$"

MossdeepCity_Gym_Text_TateAndLizaPreRematch:
	.string "TATE: POKéMON…\n"
	.string "LIZA: POKéMON…\p"
	.string "TATE: Mudando a equipe…\n"
	.string "LIZA: O estilo de batalha muda.\p"
	.string "TATE: Mas eu…\n"
	.string "LIZA: Mas eu…\p"
	.string "TATE: Sempre vou estar com a LIZA!\n"
	.string "LIZA: Sempre vou estar com o TATE!\p"
	.string "TATE: A nossa combinação…\n"
	.string "LIZA: Vamos mostrar de novo e de novo!$"

MossdeepCity_Gym_Text_TateAndLizaRematchDefeat:
	.string "TATE: Você e os seus POKéMON…\n"
	.string "LIZA: Parecem irmãos!$"

MossdeepCity_Gym_Text_TateAndLizaPostRematch:
	.string "TATE: Você pode nos derrubar,\n"
	.string "mas nunca ficamos no chão!\p"
	.string "LIZA: Porque nós dois sempre\n"
	.string "apoiamos um ao outro!$"

MossdeepCity_Gym_Text_TateAndLizaRematchNeedTwoMons:
	.string "TATE: Você voltou…\n"
	.string "LIZA: Você voltou…\p"
	.string "TATE: Pra uma revanche.\n"
	.string "LIZA: Pra uma revanche.\p"
	.string "TATE: Mas…\n"
	.string "LIZA: Pra nossa batalha…\p"
	.string "TATE: Você trouxe só um POKéMON!\n"
	.string "LIZA: Você trouxe só um POKéMON!\p"
	.string "TATE: Precisa trazer pelo menos…\n"
	.string "LIZA: Dois POKéMON quando voltar.$"
