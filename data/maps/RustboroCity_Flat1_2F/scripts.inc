RustboroCity_Flat1_2F_MapScripts::
	.byte 0

RustboroCity_Flat1_2F_EventScript_WaldasDad::
	lock
	faceplayer
	specialvar VAR_RESULT, TryBufferWaldaPhrase
	goto_if_eq VAR_RESULT, FALSE, RustboroCity_Flat1_2F_EventScript_WaldasDadFirstPhrase
	goto_if_eq VAR_RESULT, TRUE, RustboroCity_Flat1_2F_EventScript_WaldasDadNewPhrase

RustboroCity_Flat1_2F_EventScript_GivePhrase::
	special DoWaldaNamingScreen
	waitstate
	goto_if_eq VAR_0x8004, 1, RustboroCity_Flat1_2F_EventScript_CancelGivePhrase
	goto_if_eq VAR_0x8004, 2, RustboroCity_Flat1_2F_EventScript_CancelGiveFirstPhrase
	specialvar VAR_RESULT, TryGetWallpaperWithWaldaPhrase
	goto_if_eq VAR_RESULT, TRUE, RustboroCity_Flat1_2F_EventScript_WaldaLikesPhrase
	goto_if_eq VAR_RESULT, FALSE, RustboroCity_Flat1_2F_EventScript_WaldaDoesntLikePhrase
	end

RustboroCity_Flat1_2F_EventScript_WaldasDadFirstPhrase::
	msgbox RustboroCity_Flat1_2F_Text_HelloDoYouKnowFunnyPhrase, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, RustboroCity_Flat1_2F_EventScript_DeclineGivePhrase
	msgbox RustboroCity_Flat1_2F_Text_WonderfulLetsHearSuggestion, MSGBOX_DEFAULT
	goto RustboroCity_Flat1_2F_EventScript_GivePhrase

RustboroCity_Flat1_2F_EventScript_WaldasDadNewPhrase::
	msgbox RustboroCity_Flat1_2F_Text_BeenSayingXDoYouKnowBetterPhrase, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, RustboroCity_Flat1_2F_EventScript_DeclineGivePhrase
	msgbox RustboroCity_Flat1_2F_Text_WonderfulLetsHearSuggestion, MSGBOX_DEFAULT
	goto RustboroCity_Flat1_2F_EventScript_GivePhrase

RustboroCity_Flat1_2F_EventScript_DeclineGivePhrase::
	msgbox RustboroCity_Flat1_2F_Text_OhIsThatRight, MSGBOX_DEFAULT
	release
	end

RustboroCity_Flat1_2F_EventScript_CancelGivePhrase::
	msgbox RustboroCity_Flat1_2F_Text_OhYouDontKnowAny, MSGBOX_DEFAULT
	release
	end

RustboroCity_Flat1_2F_EventScript_CancelGiveFirstPhrase::
	msgbox RustboroCity_Flat1_2F_Text_ThinkOfMyOwnPhrase, MSGBOX_DEFAULT
	call RustboroCity_Flat1_2F_EventScript_WaldasDadFaceWalda
	msgbox RustboroCity_Flat1_2F_Text_ShesNotSmilingAtAll2, MSGBOX_DEFAULT
	release
	end

RustboroCity_Flat1_2F_EventScript_WaldaLikesPhrase::
	msgbox RustboroCity_Flat1_2F_Text_LetsGiveItATry2, MSGBOX_DEFAULT
	call RustboroCity_Flat1_2F_EventScript_WaldasDadFaceWalda
	msgbox RustboroCity_Flat1_2F_Text_OhShesLaughing, MSGBOX_DEFAULT
	applymovement LOCALID_WALDAS_DAD, Common_Movement_FacePlayer
	waitmovement 0
	msgbox RustboroCity_Flat1_2F_Text_ThankYouIllGiveYouWallpaper, MSGBOX_DEFAULT
	release
	end

RustboroCity_Flat1_2F_EventScript_WaldaDoesntLikePhrase::
	msgbox RustboroCity_Flat1_2F_Text_LetsGiveItATry, MSGBOX_DEFAULT
	call RustboroCity_Flat1_2F_EventScript_WaldasDadFaceWalda
	msgbox RustboroCity_Flat1_2F_Text_ShesNotSmilingAtAll, MSGBOX_DEFAULT
	release
	end

RustboroCity_Flat1_2F_EventScript_WaldasDadFaceWalda::
	turnobject LOCALID_WALDAS_DAD, DIR_EAST
	return

RustboroCity_Flat1_2F_EventScript_WaldasMom::
	msgbox RustboroCity_Flat1_2F_Text_ComingUpWithMealsIsHard, MSGBOX_NPC
	end

RustboroCity_Flat1_2F_EventScript_PokeDoll::
	msgbox RustboroCity_Flat1_2F_Text_ItsAPokemonPlushDoll, MSGBOX_SIGN
	end

RustboroCity_Flat1_2F_Text_ComingUpWithMealsIsHard:
	.string "Ai, todo dia é tão difícil…\p"
	.string "O que é difícil?\n"
	.string "Precisa perguntar?\p"
	.string "É descobrir o que fazer pra comer\n"
	.string "todo santo dia.\p"
	.string "Não é nada fácil pensar no\n"
	.string "cardápio todo dia.$"

RustboroCity_Flat1_2F_Text_HelloDoYouKnowFunnyPhrase:
	.string "Ah, olá!\n"
	.string "Esta é a casa da família PEPPER.\p"
	.string "Tenho uma pergunta pra você.\n"
	.string "Você já cuidou de bebê?\p"
	.string "É que eu sou pai de primeira viagem,\n"
	.string "então criar uma criança é novidade.\p"
	.string "E tenho um problema. A minha filha,\n"
	.string "a WALDA, não ri muito.\p"
	.string "Acho que ela riria se eu dissesse\n"
	.string "alguma coisa engraçada.\p"
	.string "Você conhece alguma palavra ou\n"
	.string "frase engraçada pra me ensinar?$"

RustboroCity_Flat1_2F_Text_BeenSayingXDoYouKnowBetterPhrase:
	.string "Ultimamente eu digo “{STR_VAR_1}”\n"
	.string "pra fazer ela rir.\p"
	.string "Você conhece alguma palavra ou\n"
	.string "frase melhor que funcione?$"

RustboroCity_Flat1_2F_Text_WonderfulLetsHearSuggestion:
	.string "Ah, que maravilha.\n"
	.string "Então, vamos ouvir a sua sugestão.$"

RustboroCity_Flat1_2F_Text_OhIsThatRight:
	.string "Ah, é mesmo?\p"
	.string "Bom, se pensar numa boa sugestão,\n"
	.string "sou todo ouvidos.$"

RustboroCity_Flat1_2F_Text_LetsGiveItATry2:
	.string "Ah, entendi.\n"
	.string "Bom, vamos experimentar, então?$"

RustboroCity_Flat1_2F_Text_OhShesLaughing:
	.string "{STR_VAR_1}.\n"
	.string "{STR_VAR_1}.\p"
	.string "Ah, isso! Ela está rindo!\n"
	.string "Estou tão feliz quanto ela!$"

RustboroCity_Flat1_2F_Text_LetsGiveItATry:
	.string "Ah, entendi.\n"
	.string "Bom, vamos experimentar, então?$"

RustboroCity_Flat1_2F_Text_ShesNotSmilingAtAll:
	.string "{STR_VAR_1}.\n"
	.string "{STR_VAR_1}.\p"
	.string "Hmmm… Ela nem sorriu.\n"
	.string "Vai ver a WALDA é uma criança séria…$"

RustboroCity_Flat1_2F_Text_ThinkOfMyOwnPhrase:
	.string "Ah, você não sabe nenhuma palavra boa.\n"
	.string "Melhor eu pensar sozinho, então.\p"
	.string "Hmm…\n"
	.string "Que tal “{STR_VAR_1}”?\l"
	.string "Vamos ver se funciona.$"

RustboroCity_Flat1_2F_Text_ShesNotSmilingAtAll2:
	.string "{STR_VAR_1}.\n"
	.string "{STR_VAR_1}.\p"
	.string "Hmmm… Ela nem sorriu.\n"
	.string "Vai ver a WALDA é uma criança séria…$"

RustboroCity_Flat1_2F_Text_OhYouDontKnowAny:
	.string "Ah, você não sabe nenhuma palavra boa.\n"
	.string "Vou tentar fazer ela rir com\l"
	.string "a frase que eu usava antes.\p"
	.string "Enfim, se tiver uma boa sugestão,\n"
	.string "não deixe de me contar, tá?$"

RustboroCity_Flat1_2F_Text_ThankYouIllGiveYouWallpaper:
	.string "Obrigado!\p"
	.string "Graças a você, a minha querida WALDA\n"
	.string "riu pra mim!\p"
	.string "Na verdade, posso não parecer grande\n"
	.string "coisa, mas sou um dos melhores\l"
	.string "pesquisadores da CORPORAÇÃO DEVON.\p"
	.string "Então, que tal eu retribuir?\p"
	.string "Já sei, vou pôr papéis de parede novos\n"
	.string "nas CAIXAS do Sistema de\l"
	.string "Armazenamento de POKéMON do PC.\p"
	.string "No menu de papel de parede,\n"
	.string "escolha “AMIGOS”.\p"
	.string "Assim você vai ter acesso aos\n"
	.string "papéis de parede novos.$"

RustboroCity_Flat1_2F_Text_ItsAPokemonPlushDoll:
	.string "É um BONECO de pelúcia de POKéMON!$"

