Route109_SeashoreHouse_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, Route109_SeashoreHouse_OnTransition
	.byte 0

Route109_SeashoreHouse_OnTransition:
	setflag FLAG_LANDMARK_SEASHORE_HOUSE
	end

Route109_SeashoreHouse_EventScript_Owner::
	lock
	faceplayer
	goto_if_set FLAG_RECEIVED_6_SODA_POP, Route109_SeashoreHouse_EventScript_AlreadyReceivedSodaPop
	goto_if_set FLAG_DEFEATED_SEASHORE_HOUSE, Route109_SeashoreHouse_EventScript_DefeatedTrainers
	goto_if_set FLAG_TEMP_2, Route109_SeashoreHouse_EventScript_AlreadyGaveIntroduction
	msgbox Route109_SeashoreHouse_Text_SeashoreHouseIntro, MSGBOX_DEFAULT
	setflag FLAG_TEMP_2
	release
	end

Route109_SeashoreHouse_EventScript_AlreadyGaveIntroduction::
	msgbox Route109_SeashoreHouse_Text_ShowMeSomeHotMatches, MSGBOX_DEFAULT
	release
	end

Route109_SeashoreHouse_EventScript_DefeatedTrainers::
	msgbox Route109_SeashoreHouse_Text_TakeTheseSodaPopBottles, MSGBOX_DEFAULT
	giveitem ITEM_SODA_POP, 6
	goto_if_eq VAR_RESULT, FALSE, Route109_SeashoreHouse_EventScript_BagFull
	setflag FLAG_RECEIVED_6_SODA_POP
	release
	end

Route109_SeashoreHouse_EventScript_BagFull::
	msgbox Route109_SeashoreHouse_Text_BagFull, MSGBOX_DEFAULT
	release
	end

Route109_SeashoreHouse_EventScript_AlreadyReceivedSodaPop::
	showmoneybox 0, 0
	msgbox Route109_SeashoreHouse_Text_WantToBuySodaPop, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, Route109_SeashoreHouse_EventScript_BuySodaPop
	msgbox Route109_SeashoreHouse_Text_ThatsTooBad, MSGBOX_DEFAULT
	hidemoneybox
	release
	end

Route109_SeashoreHouse_EventScript_BuySodaPop::
	checkmoney 300
	goto_if_eq VAR_RESULT, FALSE, Route109_SeashoreHouse_EventScript_NotEnoughMoney
	checkitemspace ITEM_SODA_POP
	goto_if_eq VAR_RESULT, FALSE, Route109_SeashoreHouse_EventScript_NotEnoughSpace
	msgbox Route109_SeashoreHouse_Text_HereYouGo, MSGBOX_DEFAULT
	removemoney 300
	updatemoneybox
	giveitem ITEM_SODA_POP
	hidemoneybox
	release
	end

Route109_SeashoreHouse_EventScript_NotEnoughMoney::
	msgbox Route109_SeashoreHouse_Text_NotEnoughMoney, MSGBOX_DEFAULT
	hidemoneybox
	release
	end

Route109_SeashoreHouse_EventScript_NotEnoughSpace::
	msgbox gText_TooBadBagIsFull, MSGBOX_DEFAULT
	hidemoneybox
	release
	end

Route109_SeashoreHouse_EventScript_Dwayne::
	trainerbattle_single TRAINER_DWAYNE, Route109_SeashoreHouse_Text_DwayneIntro, Route109_SeashoreHouse_Text_DwayneDefeated, Route109_SeashoreHouse_EventScript_CheckTrainersCompletion
	msgbox Route109_SeashoreHouse_Text_DwaynePostBattle, MSGBOX_AUTOCLOSE
	end

Route109_SeashoreHouse_EventScript_Johanna::
	trainerbattle_single TRAINER_JOHANNA, Route109_SeashoreHouse_Text_JohannaIntro, Route109_SeashoreHouse_Text_JohannaDefeated, Route109_SeashoreHouse_EventScript_CheckTrainersCompletion
	msgbox Route109_SeashoreHouse_Text_JohannaPostBattle, MSGBOX_AUTOCLOSE
	end

Route109_SeashoreHouse_EventScript_Simon::
	trainerbattle_single TRAINER_SIMON, Route109_SeashoreHouse_Text_SimonIntro, Route109_SeashoreHouse_Text_SimonDefeated, Route109_SeashoreHouse_EventScript_CheckTrainersCompletion
	msgbox Route109_SeashoreHouse_Text_SimonPostBattle, MSGBOX_AUTOCLOSE
	end

Route109_SeashoreHouse_EventScript_CheckTrainersCompletion::
	goto_if_not_defeated TRAINER_DWAYNE, Route109_SeashoreHouse_EventScript_TrainersNotCompleted
	goto_if_not_defeated TRAINER_JOHANNA, Route109_SeashoreHouse_EventScript_TrainersNotCompleted
	goto_if_not_defeated TRAINER_SIMON, Route109_SeashoreHouse_EventScript_TrainersNotCompleted
	setflag FLAG_DEFEATED_SEASHORE_HOUSE
	release
	end

Route109_SeashoreHouse_EventScript_TrainersNotCompleted::
	release
	end

Route109_SeashoreHouse_Text_SeashoreHouseIntro:
	.string "Eu sou o dono da CASA DE PRAIA.\n"
	.string "Mas pode me chamar de SR. MAR!\p"
	.string "O que eu mais amo é ver batalhas\n"
	.string "POKéMON pegando fogo.\p"
	.string "Mostre que o seu coração ferve!\p"
	.string "Se vencer todos os TREINADORES\n"
	.string "daqui, vou recompensar o seu esforço.$"

Route109_SeashoreHouse_Text_ShowMeSomeHotMatches:
	.string "Me mostre umas batalhas quentes!\p"
	.string "É só por isso que eu tenho esta\n"
	.string "CASA DE PRAIA!$"

Route109_SeashoreHouse_Text_TakeTheseSodaPopBottles:
	.string "Você está pegando fogo!\n"
	.string "Que batalhas ardentes!\l"
	.string "Estou mais que satisfeito!\p"
	.string "Pra agradecer por essa sequência\n"
	.string "quente, quero que fique com isto.\p"
	.string "São seis garrafas de REFRIGERANTE!$"

Route109_SeashoreHouse_Text_BagFull:
	.string "Ah, mas a sua BOLSA está lotada.\n"
	.string "Vou guardar isto pra você.$"

Route109_SeashoreHouse_Text_WantToBuySodaPop:
	.string "Quer comprar REFRIGERANTE?\n"
	.string "Os POKéMON adoram!\p"
	.string "Só ¥300 a garrafa!\n"
	.string "Compre!$"

Route109_SeashoreHouse_Text_HereYouGo:
	.string "Aqui está!$"

Route109_SeashoreHouse_Text_NotEnoughMoney:
	.string "Você não tem dinheiro.$"

Route109_SeashoreHouse_Text_ThatsTooBad:
	.string "Não?\n"
	.string "Que pena.$"

Route109_SeashoreHouse_Text_DwayneIntro:
	.string "Se quer batalha na CASA DE PRAIA,\n"
	.string "não vai achar TREINADOR mais\l"
	.string "quente que eu, marujo!$"

Route109_SeashoreHouse_Text_DwayneDefeated:
	.string "Que batalha quente!\n"
	.string "Aceito essa derrota, marujo!$"

Route109_SeashoreHouse_Text_DwaynePostBattle:
	.string "Sempre que estou em SLATEPORT, curto\n"
	.string "batalha quente e REFRIGERANTE gelado!$"

Route109_SeashoreHouse_Text_JohannaIntro:
	.string "Batalha chata não vale o esforço.\p"
	.string "São as batalhas pegando fogo que\n"
	.string "fortalecem TREINADORES e POKéMON!$"

Route109_SeashoreHouse_Text_JohannaDefeated:
	.string "Que quente!$"

Route109_SeashoreHouse_Text_JohannaPostBattle:
	.string "Ufa, estou com sede.\n"
	.string "Acho que vou tomar um REFRIGERANTE.$"

Route109_SeashoreHouse_Text_SimonIntro:
	.string "Vou te mostrar como os meus POKéMON\n"
	.string "são incríveis, mas não chora!$"

Route109_SeashoreHouse_Text_SimonDefeated:
	.string "…Perdi, mas não vou chorar…$"

Route109_SeashoreHouse_Text_SimonPostBattle:
	.string "Se um POKéMON meu soubesse o golpe\n"
	.string "de me levar na água nas costas, eu\l"
	.string "podia largar esta boia.$"

