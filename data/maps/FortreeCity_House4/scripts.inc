FortreeCity_House4_MapScripts::
	.byte 0

FortreeCity_House4_EventScript_Woman::
	msgbox FortreeCity_House4_Text_BringsWorldCloserTogether, MSGBOX_NPC
	end

FortreeCity_House4_EventScript_Boy::
	lockall
	goto_if_set FLAG_RECEIVED_MENTAL_HERB, FortreeCity_House4_EventScript_ReceivedMentalHerb
	goto_if_set FLAG_WINGULL_DELIVERED_MAIL, FortreeCity_House4_EventScript_WingullReturned
	goto_if_set FLAG_WINGULL_SENT_ON_ERRAND, FortreeCity_House4_EventScript_WingullOnErrand
	msgbox FortreeCity_House4_Text_GoBirdPokemon, MSGBOX_DEFAULT
	closemessage
	setflag FLAG_WINGULL_SENT_ON_ERRAND
	clearflag FLAG_HIDE_MOSSDEEP_CITY_HOUSE_2_WINGULL
	applymovement LOCALID_FORTREE_HOUSE_WINGULL, FortreeCity_House4_Movement_WingullExit
	waitmovement 0
	removeobject LOCALID_FORTREE_HOUSE_WINGULL
	releaseall
	end

FortreeCity_House4_EventScript_WingullOnErrand::
	applymovement VAR_LAST_TALKED, Common_Movement_FacePlayer
	waitmovement 0
	msgbox FortreeCity_House4_Text_AskedWingullToRunErrand, MSGBOX_DEFAULT
	releaseall
	end

FortreeCity_House4_EventScript_WingullReturned::
	applymovement VAR_LAST_TALKED, Common_Movement_FacePlayer
	waitmovement 0
	msgbox FortreeCity_House4_Text_WelcomeWingullTakeMentalHerb, MSGBOX_DEFAULT
	giveitem ITEM_MENTAL_HERB
	goto_if_eq VAR_RESULT, FALSE, Common_EventScript_ShowBagIsFull
	setflag FLAG_RECEIVED_MENTAL_HERB
	releaseall
	end

FortreeCity_House4_EventScript_ReceivedMentalHerb::
	applymovement VAR_LAST_TALKED, Common_Movement_FacePlayer
	waitmovement 0
	msgbox FortreeCity_House4_Text_FriendsFarAwayThanksToWingull, MSGBOX_DEFAULT
	releaseall
	end

FortreeCity_House4_Movement_WingullExit:
	walk_fast_down
	walk_fast_down
	walk_fast_right
	walk_in_place_faster_down
	delay_8
	step_end

FortreeCity_House4_EventScript_Wingull::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_WINGULL, CRY_MODE_NORMAL
	msgbox FortreeCity_House4_Text_Wingull, MSGBOX_DEFAULT
	waitmoncry
	release
	end

FortreeCity_House4_Text_BringsWorldCloserTogether:
	.string "Junto com os POKéMON, as pessoas\n"
	.string "fazem cada vez mais amigos.\p"
	.string "E isso aproxima o mundo.\n"
	.string "Acho isso maravilhoso!$"

FortreeCity_House4_Text_GoBirdPokemon:
	.string "Vai!\n"
	.string "Vai, POKéMON PÁSSARO!$"

FortreeCity_House4_Text_AskedWingullToRunErrand:
	.string "Hehe, pedi pro meu WINGULL fazer\n"
	.string "um favor pra mim.$"

FortreeCity_House4_Text_WelcomeWingullTakeMentalHerb:
	.string "Isso!\n"
	.string "Bem-vindo de volta, WINGULL!\p"
	.string "Hã? O que é isso?\n"
	.string "O que ele está segurando?\p"
	.string "Uma ERVA MENTAL?\n"
	.string "Deve ter achado em algum lugar.\p"
	.string "Mas eu não sou TREINADOR, então\n"
	.string "pode ficar com ela.$"

FortreeCity_House4_Text_FriendsFarAwayThanksToWingull:
	.string "Graças ao meu WINGULL, tenho amigos\n"
	.string "que moram longe.$"

FortreeCity_House4_Text_Wingull:
	.string "WINGULL: Pihyoh!$"

