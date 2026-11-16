LilycoveCity_CoveLilyMotel_2F_MapScripts::
	.byte 0

LilycoveCity_CoveLilyMotel_2F_EventScript_GameDesigner::
	lock
	faceplayer
	call_if_unset FLAG_TEMP_2, LilycoveCity_CoveLilyMotel_2F_EventScript_ShowMeCompletedDex
	call_if_set FLAG_TEMP_2, LilycoveCity_CoveLilyMotel_2F_EventScript_ShowDiploma
	specialvar VAR_RESULT, HasAllHoennMons
	goto_if_eq VAR_RESULT, TRUE, LilycoveCity_CoveLilyMotel_2F_EventScript_AllHoennMonsFanfare
	release
	end

LilycoveCity_CoveLilyMotel_2F_EventScript_ShowMeCompletedDex::
	msgbox LilycoveCity_CoveLilyMotel_2F_Text_ShowMeCompletedDex, MSGBOX_DEFAULT
	return

LilycoveCity_CoveLilyMotel_2F_EventScript_AllHoennMonsFanfare::
	setflag FLAG_TEMP_2
	playfanfare MUS_OBTAIN_ITEM
	waitfanfare
	goto LilycoveCity_CoveLilyMotel_2F_EventScript_ShowDiploma
	end

LilycoveCity_CoveLilyMotel_2F_EventScript_ShowDiploma::
	message LilycoveCity_CoveLilyMotel_2F_Text_FilledPokedexGiveYouThis
	waitmessage
	call Common_EventScript_PlayGymBadgeFanfare
	special Special_ShowDiploma
	waitstate
	release
	end

LilycoveCity_CoveLilyMotel_2F_EventScript_Programmer::
	msgbox LilycoveCity_CoveLilyMotel_2F_Text_ImTheProgrammer, MSGBOX_NPC
	end

LilycoveCity_CoveLilyMotel_2F_EventScript_GraphicArtist::
	msgbox LilycoveCity_CoveLilyMotel_2F_Text_ImTheGraphicArtist, MSGBOX_NPC
	end

LilycoveCity_CoveLilyMotel_2F_EventScript_FatMan::
	msgbox LilycoveCity_CoveLilyMotel_2F_Text_GirlsAreCute, MSGBOX_NPC
	end

LilycoveCity_CoveLilyMotel_2F_EventScript_Woman::
	msgbox LilycoveCity_CoveLilyMotel_2F_Text_SeaBreezeTicklesHeart, MSGBOX_NPC
	end

LilycoveCity_CoveLilyMotel_2F_EventScript_GameBoyKid::
	msgbox LilycoveCity_CoveLilyMotel_2F_Text_NeverLeaveWithoutGameBoy, MSGBOX_NPC
	end

LilycoveCity_CoveLilyMotel_2F_EventScript_Scott::
	lock
	faceplayer
	goto_if_set FLAG_MET_SCOTT_IN_LILYCOVE, LilycoveCity_CoveLilyMotel_2F_EventScript_MetScott
	msgbox LilycoveCity_CoveLilyMotel_2F_Text_SnoozingPreferBattles, MSGBOX_DEFAULT
	addvar VAR_SCOTT_STATE, 1
	setflag FLAG_MET_SCOTT_IN_LILYCOVE
	release
	end

LilycoveCity_CoveLilyMotel_2F_EventScript_MetScott::
	msgbox LilycoveCity_CoveLilyMotel_2F_Text_ContestsDoTakeStrategy, MSGBOX_DEFAULT
	release
	end

LilycoveCity_CoveLilyMotel_2F_Text_ShowMeCompletedDex:
	.string "Eu sou o DESIGNER DO JOGO.\p"
	.string "Ah, é mesmo?\n"
	.string "Você está montando uma POKéDEX?\p"
	.string "É difícil completar, mas não\n"
	.string "desista.\p"
	.string "Se completar, venha me mostrar.$"

LilycoveCity_CoveLilyMotel_2F_Text_FilledPokedexGiveYouThis:
	.string "Uau! Que incrível!\n"
	.string "É, incrível demais!\p"
	.string "Esta POKéDEX está completa!\n"
	.string "Você ama mesmo os POKéMON!\p"
	.string "Estou muito impressionado!\p"
	.string "Deixa eu te dar uma coisa pelo\n"
	.string "seu feito!$"

LilycoveCity_CoveLilyMotel_2F_Text_ImTheProgrammer:
	.string "Eu? Está falando comigo?\n"
	.string "Sou o PROGRAMADOR.\p"
	.string "Como serão os CAÇA-NÍQUEIS daqui?$"

LilycoveCity_CoveLilyMotel_2F_Text_ImTheGraphicArtist:
	.string "Sou o ARTISTA GRÁFICO! Os POKéMON\n"
	.string "de HOENN não são interessantes?$"

LilycoveCity_CoveLilyMotel_2F_Text_GirlsAreCute:
	.string "As meninas NADADORAS de boia são\n"
	.string "fofas, né? Batalhar com uma delas…\p"
	.string "Uhu, que demais!\p"
	.string "E as GÊMEAS! Não são fofas?\n"
	.string "Uma batalha 2 contra 2 com GÊMEAS…\p"
	.string "Uhu, divertido demais!$"

LilycoveCity_CoveLilyMotel_2F_Text_SeaBreezeTicklesHeart:
	.string "A brisa do mar faz cócegas no\n"
	.string "coração. Que delícia aqui!$"

LilycoveCity_CoveLilyMotel_2F_Text_NeverLeaveWithoutGameBoy:
	.string "Nunca se sabe quando e onde alguém\n"
	.string "vai te desafiar.\p"
	.string "Por isso nunca saio de casa sem o\n"
	.string "meu GAME BOY ADVANCE.$"

LilycoveCity_CoveLilyMotel_2F_Text_SnoozingPreferBattles:
	.string "SCOTT: … … … … …\n"
	.string "… … … … … Zzz…\p"
	.string "… … … … … Hã?!\n"
	.string "Ai, desculpa, desculpa! Cochilei!\p"
	.string "Vim conferir esse tal de CONCURSO\n"
	.string "POKéMON.\p"
	.string "Tenho que admitir, parece bem\n"
	.string "divertido, mas…\p"
	.string "Sou dos tradicionais: prefiro\n"
	.string "batalhas e TREINADORES fortes.\p"
	.string "Mas isso sou eu.\p"
	.string "{PLAYER}{KUN}, espero que você curta\n"
	.string "tudo: GINÁSIOS, CONCURSOS, TENDA\l"
	.string "DE BATALHA, tudo mesmo!$"

LilycoveCity_CoveLilyMotel_2F_Text_ContestsDoTakeStrategy:
	.string "SCOTT: Acho que é preciso estratégia\n"
	.string "pra ganhar um CONCURSO.\p"
	.string "Criar estratégias de CONCURSO é um\n"
	.string "jeito de virar um TREINADOR melhor.$"

