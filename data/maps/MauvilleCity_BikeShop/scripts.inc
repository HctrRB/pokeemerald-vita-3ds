MauvilleCity_BikeShop_MapScripts::
	.byte 0

MauvilleCity_BikeShop_EventScript_Rydel::
	lock
	faceplayer
	goto_if_set FLAG_RECEIVED_BIKE, MauvilleCity_BikeShop_EventScript_AskSwitchBikes
	goto_if_set FLAG_DECLINED_BIKE, MauvilleCity_BikeShop_EventScript_SkipGreeting
	msgbox MauvilleCity_BikeShop_Text_RydelGreeting, MSGBOX_DEFAULT
	msgbox MauvilleCity_BikeShop_Text_DidYouComeFromFarAway, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, MauvilleCity_BikeShop_EventScript_YesFar
	goto_if_eq VAR_RESULT, NO, MauvilleCity_BikeShop_EventScript_NotFar
	end

MauvilleCity_BikeShop_EventScript_SkipGreeting::
	msgbox MauvilleCity_BikeShop_Text_DidYouComeFromFarAway, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, MauvilleCity_BikeShop_EventScript_YesFar
	goto_if_eq VAR_RESULT, NO, MauvilleCity_BikeShop_EventScript_NotFar
	end

MauvilleCity_BikeShop_EventScript_ChooseBike::
	message MauvilleCity_BikeShop_Text_ExplainBikesChooseWhichOne
	waitmessage
	multichoice 21, 8, MULTI_BIKE, TRUE
	switch VAR_RESULT
	case 0, MauvilleCity_BikeShop_EventScript_GetMachBike
	case 1, MauvilleCity_BikeShop_EventScript_GetAcroBike
	end

MauvilleCity_BikeShop_EventScript_NotFar::
	setflag FLAG_DECLINED_BIKE
	msgbox MauvilleCity_BikeShop_Text_GuessYouDontNeedBike, MSGBOX_DEFAULT
	release
	end

MauvilleCity_BikeShop_EventScript_YesFar::
	setflag FLAG_RECEIVED_BIKE
	goto MauvilleCity_BikeShop_EventScript_ChooseBike
	end

MauvilleCity_BikeShop_EventScript_GetMachBike::
	msgbox MauvilleCity_BikeShop_Text_ChoseMachBike, MSGBOX_DEFAULT
	giveitem ITEM_MACH_BIKE
	goto MauvilleCity_BikeShop_EventScript_ComeBackToSwitchBikes
	end

MauvilleCity_BikeShop_EventScript_GetAcroBike::
	msgbox MauvilleCity_BikeShop_Text_ChoseAcroBike, MSGBOX_DEFAULT
	giveitem ITEM_ACRO_BIKE
	goto MauvilleCity_BikeShop_EventScript_ComeBackToSwitchBikes
	end

MauvilleCity_BikeShop_EventScript_ComeBackToSwitchBikes::
	msgbox MauvilleCity_BikeShop_Text_ComeBackToSwitchBikes, MSGBOX_DEFAULT
	special SwapRegisteredBike
	release
	end

MauvilleCity_BikeShop_EventScript_AskSwitchBikes::
	msgbox MauvilleCity_BikeShop_Text_WantToSwitchBikes, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, MauvilleCity_BikeShop_EventScript_SwitchBikes
	goto_if_eq VAR_RESULT, NO, MauvilleCity_BikeShop_EventScript_KeepBike
	end

/* If the player does not have a bike on them Rydel assumes its stored in the PC*/
MauvilleCity_BikeShop_EventScript_SwitchBikes::
	msgbox MauvilleCity_BikeShop_Text_IllSwitchBikes, MSGBOX_DEFAULT
	checkitem ITEM_ACRO_BIKE
	goto_if_eq VAR_RESULT, TRUE, MauvilleCity_BikeShop_EventScript_SwitchAcroForMach
	checkitem ITEM_MACH_BIKE
	goto_if_eq VAR_RESULT, TRUE, MauvilleCity_BikeShop_EventScript_SwitchMachForAcro
	msgbox MauvilleCity_BikeShop_Text_OhYourBikeIsInPC, MSGBOX_DEFAULT
	release
	end

MauvilleCity_BikeShop_EventScript_KeepBike::
	msgbox MauvilleCity_BikeShop_Text_HappyYouLikeIt, MSGBOX_DEFAULT
	release
	end

MauvilleCity_BikeShop_EventScript_SwitchAcroForMach::
	incrementgamestat GAME_STAT_TRADED_BIKES
	msgbox MauvilleCity_BikeShop_Text_ExchangedAcroForMach, MSGBOX_DEFAULT
	removeitem ITEM_ACRO_BIKE
	giveitem ITEM_MACH_BIKE
	goto MauvilleCity_BikeShop_EventScript_ComeBackToSwitchBikes
	end

MauvilleCity_BikeShop_EventScript_SwitchMachForAcro::
	incrementgamestat GAME_STAT_TRADED_BIKES
	msgbox MauvilleCity_BikeShop_Text_ExchangedMachForAcro, MSGBOX_DEFAULT
	removeitem ITEM_MACH_BIKE
	giveitem ITEM_ACRO_BIKE
	goto MauvilleCity_BikeShop_EventScript_ComeBackToSwitchBikes
	end

MauvilleCity_BikeShop_EventScript_Assistant::
	msgbox MauvilleCity_BikeShop_Text_HandbooksAreInBack, MSGBOX_NPC
	end

MauvilleCity_BikeShop_EventScript_MachBikeHandbook::
	message MauvilleCity_BikeShop_Text_MachHandbookWhichPage
	waitmessage
	goto MauvilleCity_BikeShop_EventScript_ChooseMachHandbookPage
	end

MauvilleCity_BikeShop_EventScript_ChooseMachHandbookPage::
	multichoice 0, 0, MULTI_MACH_BIKE_INFO, FALSE
	switch VAR_RESULT
	case 0, MauvilleCity_BikeShop_EventScript_HowToRide
	case 1, MauvilleCity_BikeShop_EventScript_HowToTurn
	case 2, MauvilleCity_BikeShop_EventScript_SandySlopes
	case 3, MauvilleCity_BikeShop_EventScript_ExitMachHandbook
	case MULTI_B_PRESSED, MauvilleCity_BikeShop_EventScript_ExitMachHandbook
	end

MauvilleCity_BikeShop_EventScript_HowToRide::
	message MauvilleCity_BikeShop_Text_HowToRideMachBike
	waitmessage
	goto MauvilleCity_BikeShop_EventScript_ChooseMachHandbookPage
	end

MauvilleCity_BikeShop_EventScript_HowToTurn::
	message MauvilleCity_BikeShop_Text_HowToTurnMachBike
	waitmessage
	goto MauvilleCity_BikeShop_EventScript_ChooseMachHandbookPage
	end

MauvilleCity_BikeShop_EventScript_SandySlopes::
	message MauvilleCity_BikeShop_Text_SandySlopes
	waitmessage
	goto MauvilleCity_BikeShop_EventScript_ChooseMachHandbookPage
	end

MauvilleCity_BikeShop_EventScript_ExitMachHandbook::
	release
	end

MauvilleCity_BikeShop_EventScript_AcroBikeHandbook::
	message MauvilleCity_BikeShop_Text_AcroHandbookWhichPage
	waitmessage
	goto MauvilleCity_BikeShop_EventScript_ChooseAcroHandbookPage
	end

MauvilleCity_BikeShop_EventScript_ChooseAcroHandbookPage::
	multichoice 0, 0, MULTI_ACRO_BIKE_INFO, FALSE
	switch VAR_RESULT
	case 0, MauvilleCity_BikeShop_EventScript_Wheelies
	case 1, MauvilleCity_BikeShop_EventScript_BunnyHops
	case 2, MauvilleCity_BikeShop_EventScript_Jumps
	case 3, MauvilleCity_BikeShop_EventScript_ExitAcroHandbook
	case MULTI_B_PRESSED, MauvilleCity_BikeShop_EventScript_ExitAcroHandbook
	end

MauvilleCity_BikeShop_EventScript_Wheelies::
	message MauvilleCity_BikeShop_Text_Wheelies
	waitmessage
	goto MauvilleCity_BikeShop_EventScript_ChooseAcroHandbookPage
	end

MauvilleCity_BikeShop_EventScript_BunnyHops::
	message MauvilleCity_BikeShop_Text_BunnyHops
	waitmessage
	goto MauvilleCity_BikeShop_EventScript_ChooseAcroHandbookPage
	end

MauvilleCity_BikeShop_EventScript_Jumps::
	message MauvilleCity_BikeShop_Text_Jumps
	waitmessage
	goto MauvilleCity_BikeShop_EventScript_ChooseAcroHandbookPage
	end

MauvilleCity_BikeShop_EventScript_ExitAcroHandbook::
	release
	end

MauvilleCity_BikeShop_Text_RydelGreeting:
	.string "Ora, ora, o que temos aqui?\n"
	.string "Que freguesia cheia de energia!\p"
	.string "Eu? Pode me chamar de RYDEL.\n"
	.string "Sou o dono desta bicicletaria.$"

MauvilleCity_BikeShop_Text_DidYouComeFromFarAway:
	.string "RYDEL: O seu TÊNIS DE CORRER…\n"
	.string "Está sujo à beça.\p"
	.string "Você veio de longe?$"

MauvilleCity_BikeShop_Text_GuessYouDontNeedBike:
	.string "RYDEL: É mesmo?\p"
	.string "Então acho que você não precisa de\n"
	.string "nenhuma das minhas BICICLETAS.$"

MauvilleCity_BikeShop_Text_ExplainBikesChooseWhichOne:
	.string "RYDEL: Hm, hm… … … … …\p"
	.string "Você está dizendo que veio lá de\n"
	.string "LITTLEROOT até aqui?\p"
	.string "Minha nossa!\n"
	.string "Isso é longe demais!\p"
	.string "Com uma das minhas BICICLETAS, você\n"
	.string "ia a qualquer lugar fácil, sentindo\l"
	.string "o carinho suave do vento!\p"
	.string "Já sei!\n"
	.string "Vou te dar uma BICICLETA!\p"
	.string "Ah, espera um pouco!\p"
	.string "Esqueci de dizer que existem dois\n"
	.string "tipos de BICICLETA!\p"
	.string "A BICICLETA MACH e a\n"
	.string "BICICLETA ACRO!\p"
	.string "A BICICLETA MACH é pra quem quer\n"
	.string "sentir o vento no corpo!\p"
	.string "E a BICICLETA ACRO é pra quem\n"
	.string "prefere manobras técnicas!\p"
	.string "Sou um amor de pessoa, então você\n"
	.string "pode ficar com a que quiser!\p"
	.string "Qual você escolhe?$"

MauvilleCity_BikeShop_Text_ChoseMachBike:
	.string "{PLAYER} escolheu a BICICLETA MACH.$"

MauvilleCity_BikeShop_Text_ChoseAcroBike:
	.string "{PLAYER} escolheu a BICICLETA ACRO.$"

MauvilleCity_BikeShop_Text_ComeBackToSwitchBikes:
	.string "RYDEL: Se der vontade de trocar de\n"
	.string "BICICLETA, é só vir me ver!$"

MauvilleCity_BikeShop_Text_WantToSwitchBikes:
	.string "RYDEL: Hã? Está pensando em trocar\n"
	.string "de BICICLETA?$"

MauvilleCity_BikeShop_Text_IllSwitchBikes:
	.string "RYDEL: Tudo bem, sem problemas!\n"
	.string "Eu troco a BICICLETA pra você!$"

MauvilleCity_BikeShop_Text_ExchangedMachForAcro:
	.string "{PLAYER} trocou a BICICLETA MACH\n"
	.string "pela BICICLETA ACRO.$"

MauvilleCity_BikeShop_Text_ExchangedAcroForMach:
	.string "{PLAYER} trocou a BICICLETA ACRO\n"
	.string "pela BICICLETA MACH.$"

MauvilleCity_BikeShop_Text_HappyYouLikeIt:
	.string "RYDEL: Ótimo, ótimo!\n"
	.string "Fico feliz que tenha gostado!$"

MauvilleCity_BikeShop_Text_OhYourBikeIsInPC:
	.string "Hã? O que aconteceu com a BICICLETA\n"
	.string "que eu te dei?\p"
	.string "Ah, entendi, você guardou no PC.\p"
	.string "Bom, tire do PC que eu troco\n"
	.string "com prazer!\p"
	.string "Que o vento sempre sopre a seu\n"
	.string "favor na aventura!$"

MauvilleCity_BikeShop_Text_HandbooksAreInBack:
	.string "Estou aprendendo sobre BICICLETAS\n"
	.string "enquanto trabalho aqui.\p"
	.string "Se precisar de dicas pra andar de\n"
	.string "BICICLETA, tem uns manuais\l"
	.string "lá no fundo.$"

MauvilleCity_BikeShop_Text_MachHandbookWhichPage:
	.string "É um manual da BICICLETA MACH.\p"
	.string "Qual página você quer ler?$"

MauvilleCity_BikeShop_Text_HowToRideMachBike:
	.string "A BICICLETA anda na direção em que\n"
	.string "você aperta o Botão Direcional.\p"
	.string "Ela acelera depois que pega embalo.\p"
	.string "Pra parar, solte o Botão Direcional.\n"
	.string "A BICICLETA vai desacelerar até parar.\p"
	.string "Quer ler outra página?$"

MauvilleCity_BikeShop_Text_HowToTurnMachBike:
	.string "A BICICLETA MACH é rápida, mas não\n"
	.string "consegue parar muito rápido.\p"
	.string "Fica um pouco difícil fazer\n"
	.string "as curvas.\p"
	.string "Solte o Botão Direcional um pouco\n"
	.string "antes da curva e desacelere.\p"
	.string "Quer ler outra página?$"

MauvilleCity_BikeShop_Text_SandySlopes:
	.string "Há pequenas rampas de areia por\n"
	.string "toda a região de HOENN.\p"
	.string "A areia fofa e solta não deixa\n"
	.string "subir do jeito normal.\p"
	.string "Mas, com uma BICICLETA MACH, você\n"
	.string "sobe uma rampa de areia num instante.\p"
	.string "Quer ler outra página?$"

MauvilleCity_BikeShop_Text_AcroHandbookWhichPage:
	.string "É um manual da BICICLETA ACRO.\p"
	.string "Qual página você quer ler?$"

MauvilleCity_BikeShop_Text_Wheelies:
	.string "Aperte o Botão B enquanto pedala, e a\n"
	.string "roda da frente sobe.\p"
	.string "Dá pra andar com a roda da frente\n"
	.string "levantada usando o Botão Direcional.\p"
	.string "Essa manobra se chama empinar.\p"
	.string "Quer ler outra página?$"

MauvilleCity_BikeShop_Text_BunnyHops:
	.string "Segurando o Botão B, a sua\n"
	.string "BICICLETA pula sem sair do lugar.\p"
	.string "Essa manobra se chama bunny hop.\p"
	.string "Também dá pra andar pulando.\p"
	.string "Quer ler outra página?$"

MauvilleCity_BikeShop_Text_Jumps:
	.string "Aperte o Botão B e o Botão Direcional\n"
	.string "ao mesmo tempo pra pular.\p"
	.string "Aperte o Botão Direcional pro lado\n"
	.string "pra pular de lado.\p"
	.string "Aperte pra trás pra BICICLETA\n"
	.string "mudar de direção enquanto pula.\p"
	.string "Quer ler outra página?$"

