SafariZone_EventScript_OutOfBallsMidBattle::
	setvar VAR_SAFARI_ZONE_STATE, 1
	special ExitSafariMode
	setwarp MAP_ROUTE121_SAFARI_ZONE_ENTRANCE, 2, 5
	end

SafariZone_EventScript_Exit::
	setvar VAR_SAFARI_ZONE_STATE, 1
	special ExitSafariMode
	warp MAP_ROUTE121_SAFARI_ZONE_ENTRANCE, 2, 5
	waitstate
	end

SafariZone_EventScript_RetirePrompt::
	lockall
	msgbox SafariZone_Text_WouldYouLikeToExit, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, SafariZone_EventScript_Retire
	releaseall
	end

SafariZone_EventScript_Retire::
	goto SafariZone_EventScript_Exit

SafariZone_EventScript_TimesUp::
	lockall
	playse SE_DING_DONG
	message SafariZone_Text_TimesUp
	waitmessage
	waitbuttonpress
	releaseall
	goto SafariZone_EventScript_Exit

SafariZone_EventScript_OutOfBalls::
	lockall
	playse SE_DING_DONG
	message SafariZone_Text_OutOfBalls
	waitmessage
	waitbuttonpress
	releaseall
	goto SafariZone_EventScript_Exit

EventScript_PokeBlockFeeder::
	lockall
	special GetPokeblockFeederInFront
	goto_if_ne VAR_RESULT, 0xFFFF, SafariZone_EventScript_PokeblockPresent
	msgbox SafariZone_Text_PlacePokeblockOnFeeder, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, SafariZone_EventScript_ChoosePokeblock
	releaseall
	end

SafariZone_EventScript_ChoosePokeblock::
	fadescreen FADE_TO_BLACK
	special OpenPokeblockCaseOnFeeder
	waitstate
	goto_if_ne VAR_RESULT, 0xFFFF, SafariZone_EventScript_PokeblockPlaced
#ifdef BUGFIX
	releaseall  @ Only gets called from EventScript_PokeBlockFeeder which uses lockall.
#endif
	end

SafariZone_EventScript_PokeblockPlaced::
	message SafariZone_Text_PokeblockWasPlaced
	waitmessage
	waitbuttonpress
	releaseall
	end

SafariZone_EventScript_PokeblockPresent::
	message SafariZone_Text_PokeblockStillHere
	waitmessage
	waitbuttonpress
	releaseall
	end

SafariZone_Text_WouldYouLikeToExit:
	.string "Quer sair da ZONA DE SAFÁRI\n"
	.string "agora mesmo?$"

SafariZone_Text_TimesUp:
	.string "Dim-dom! Acabou o tempo!\n"
	.string "O seu Jogo do SAFÁRI terminou.$"

SafariZone_Text_OutOfBalls:
	.string "Acabaram as suas BOLAS SAFÁRI.\n"
	.string "O seu Jogo do SAFÁRI terminou.$"

SafariZone_Text_PlacePokeblockOnFeeder:
	.string "Quer pôr um {POKEBLOCK} no\n"
	.string "COMEDOURO DE {POKEBLOCK}?$"

SafariZone_Text_PokeblockStillHere:
	.string "O {STR_VAR_1} que você deixou\n"
	.string "antes ainda está aqui.$"

SafariZone_Text_PokeblockWasPlaced:
	.string "O {STR_VAR_1} foi colocado\n"
	.string "no COMEDOURO DE {POKEBLOCK}.$"

Route121_SafariZoneEntrance_Text_WelcomeToSafariZone:
	.string "Boas-vindas à ZONA DE SAFÁRI!\p"
	.string "Aqui você vê muitos tipos de POKéMON\n"
	.string "que quase não aparecem em HOENN.\p"
	.string "E vê todos eles selvagens, livres,\n"
	.string "no seu habitat natural.\p"
	.string "E mais: abrimos os portões para os\n"
	.string "TREINADORES POKéMON.\p"
	.string "Você pode pegar POKéMON pra ficar\n"
	.string "com eles!\p"
	.string "Entre e aproveite a ZONA DE SAFÁRI!$"

Route121_SafariZoneEntrance_Text_WelcomeFirstTime:
	.string "Boas-vindas à ZONA DE SAFÁRI!\n"
	.string "É a sua primeira vez aqui?$"

Route121_SafariZoneEntrance_Text_ComeInAndEnjoy:
	.string "Entre e aproveite a ZONA DE SAFÁRI!$"

Route121_SafariZoneEntrance_Text_FirstTimeInfo:
	.string "Ao entrar na ZONA DE SAFÁRI, você\n"
	.string "recebe 30 BOLAS SAFÁRI pra pegar\l"
	.string "POKéMON.\p"
	.string "O Jogo do SAFÁRI termina quando as\n"
	.string "BOLAS SAFÁRI acabam ou quando você\l"
	.string "anda 500 passos.\p"
	.string "Entre e aproveite a ZONA DE SAFÁRI!$"

Route121_SafariZoneEntrance_Text_WouldYouLikeToPlay:
	.string "Boas-vindas à ZONA DE SAFÁRI!\p"
	.string "Pegue quantos quiser por só ¥500!\n"
	.string "Quer jogar o Jogo do SAFÁRI?$"

Route121_SafariZoneEntrance_Text_PlayAnotherTime:
	.string "Certo.\n"
	.string "Venha jogar outra hora!$"

Route121_SafariZoneEntrance_Text_NotEnoughMoney:
	.string "Você não tem dinheiro suficiente.\n"
	.string "Desculpe.$"

Route121_SafariZoneEntrance_Text_ThatWillBe500Please:
	.string "São ¥500, por favor.$"

Route121_SafariZoneEntrance_Text_HereAreYourSafariBalls:
	.string "Aqui estão as suas BOLAS SAFÁRI.$"

Route121_SafariZoneEntrance_Text_Received30SafariBalls:
	.string "{PLAYER} recebeu 30 BOLAS SAFÁRI.$"

Route121_SafariZoneEntrance_Text_PleaseEnjoyYourself:
	.string "Vamos avisar quando o jogo acabar.\p"
	.string "Até lá, divirta-se, por favor!\n"
	.string "Boa expedição pela natureza!$"

Route121_SafariZoneEntrance_Text_PCIsFull:
	.string "Com licença!\n"
	.string "A sua CAIXA do PC está cheia.$"

Route121_SafariZoneEntrance_Text_YouNeedPokeblockCase:
	.string "Com licença!\n"
	.string "Parece que você está sem um ESTOJO\l"
	.string "{POKEBLOCK}.\p"
	.string "O seu Jogo do SAFÁRI rende muito mais\n"
	.string "se você usar {POKEBLOCK}S.\p"
	.string "Volte com um ESTOJO {POKEBLOCK},\n"
	.string "por favor.\p"
	.string "Você pode conseguir um ESTOJO\n"
	.string "{POKEBLOCK} no SALÃO DE CONCURSOS\l"
	.string "de LILYCOVE.$"

SafariZone_South_Text_StillHaveTimeExit:
	.string "Você ainda tem tempo. Quer sair\n"
	.string "da ZONA DE SAFÁRI agora?$"

SafariZone_South_Text_EnjoyTheRestOfYourAdventure:
	.string "Aproveite o resto da sua aventura\n"
	.string "pela natureza!$"

SafariZone_South_Text_ExitEarlyThankYouForPlaying:
	.string "Certo.\p"
	.string "Vou pegar de volta as BOLAS SAFÁRI\n"
	.string "que sobraram.\p"
	.string "Obrigado por jogar.\n"
	.string "Esperamos ver você de novo.$"

SafariZone_South_Text_GoodLuck:
	.string "Boa sorte!\p"
	.string "Se precisar de alguma coisa, é só\n"
	.string "me falar!$"

SafariZone_South_Text_Boy:
	.string "Sabia?\p"
	.string "Se você puser um {POKEBLOCK} naquela\n"
	.string "caixa quadrada, os POKéMON se juntam.$"

SafariZone_South_Text_Man:
	.string "Quero ir mais pro fundo, mas esqueci\n"
	.string "de trazer a BICICLETA.\p"
	.string "Alguma coisa me diz que POKéMON\n"
	.string "raros vivem nas áreas mais afastadas.$"

SafariZone_Southwest_Text_Woman:
	.string "Às vezes, quando jogo um {POKEBLOCK}\n"
	.string "num POKéMON, ele nem liga.\p"
	.string "Será que os POKéMON têm gostos pra\n"
	.string "comida?$"

SafariZone_Northwest_Text_Man:
	.string "Uf… Uf…\n"
	.string "Cheguei… até aqui… mas…\p"
	.string "Estou exausto… Não tenho energia\n"
	.string "pra pegar POKéMON…$"

SafariZone_North_Text_Fisherman:
	.string "A minha missão é achar POKéMON de\n"
	.string "ÁGUA que não aparecem em HOENN.\p"
	.string "Sabe onde fica o lago?$"

SafariZone_North_Text_Man:
	.string "Vou pegar um monte de POKéMON raros\n"
	.string "aqui e trocar com os meus amigos!$"

SafariZone_South_Text_Youngster:
	.string "Pus um {POKEBLOCK} no COMEDOURO\n"
	.string "DE {POKEBLOCK}. Mas parece que sumiu.\p"
	.string "Acho que algum POKéMON comeu sem\n"
	.string "eu perceber.$"

Route121_SafariZoneEntrance_Text_TrainerTip:
	.string "DICA DE TREINADOR DA ZONA DE SAFÁRI!\p"
	.string "Jogue {POKEBLOCK}S nos POKéMON selvagens\n"
	.string "pra eles fugirem menos.$"

SafariZone_Southwest_Text_RestHouseSign:
	.string "“Descanse os pés cansados.”\n"
	.string "CASA DE DESCANSO$"

SafariZone_RestHouse_Text_Youngster:
	.string "Não tenho {POKEBLOCK}S, mas peguei\n"
	.string "um bom tanto de POKéMON.\p"
	.string "Você aumenta a chance de pegar se\n"
	.string "chegar mais perto antes de jogar\l"
	.string "a BOLA SAFÁRI.$"

SafariZone_RestHouse_Text_PsychicM:
	.string "Com {POKEBLOCK}S, os POKéMON selvagens\n"
	.string "não fogem tão rápido.\p"
	.string "Não adianta muito dar {POKEBLOCK}S pra\n"
	.string "POKéMON que não fogem fácil.$"

SafariZone_RestHouse_Text_FatMan:
	.string "Se você puser um {POKEBLOCK} no COMEDOURO,\n"
	.string "os POKéMON são atraídos.\p"
	.string "Acho que POKéMON de natureza parecida\n"
	.string "são atraídos por um certo {POKEBLOCK}.$"

SafariZone_South_Text_AreaOffLimits1:
	.string "Esta área ainda está em obras.\n"
	.string "A entrada é proibida, desculpe!$"

SafariZone_Southeast_Text_ExpansionIsFinished:
	.string "A expansão da ZONA DE SAFÁRI\n"
	.string "terminou.\p"
	.string "Esperamos que goste da área nova.$"

SafariZone_South_Text_AreaOffLimits2:
	.string "Esta área ainda está em obras.\n"
	.string "A entrada é proibida, desculpe!$"

SafariZone_Southeast_Text_LittleGirl:
	.string "Uau! Oba! Nunca vi nenhum desses\n"
	.string "POKéMON antes!$"

SafariZone_Southeast_Text_FatMan:
	.string "Os POKéMON desta área são todos\n"
	.string "novos pra mim.\p"
	.string "E ainda posso pegar esses POKéMON\n"
	.string "raros! Que demais!$"

SafariZone_Southeast_Text_RichBoy:
	.string "Os POKéMON daqui parecem ser de\n"
	.string "algum lugar fora de HOENN.$"

SafariZone_Northeast_Text_Boy:
	.string "Só me sobraram umas BOLAS SAFÁRI.\p"
	.string "Estou com dificuldade pra decidir\n"
	.string "o que pegar.$"

SafariZone_Northeast_Text_Woman:
	.string "Ouvi dizer que dá pra ver PIKACHU\n"
	.string "aqui. Onde será que tem um?$"

SafariZone_Northeast_Text_Girl:
	.string "Ai, que droga!\n"
	.string "Não consigo pegar nada!\p"
	.string "Vou ter desperdiçado a entrada se\n"
	.string "não pegar alguma coisa!$"
