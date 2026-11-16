@ All the LOCALID constants used in this file are essentially redundant.
@ They're all used to have the NPC the player talked to turn back so they're facing
@ their original direction (because they're watching/participating in the contests).
@ They could all be substituted for VAR_LAST_TALKED.
LilycoveCity_ContestHall_MapScripts::
	.byte 0

LilycoveCity_ContestHall_EventScript_Boy1::
	msgbox LilycoveCity_ContestHall_Text_TodayWonSmartnessContest, MSGBOX_NPC
	end

LilycoveCity_ContestHall_EventScript_Boy2::
	msgbox LilycoveCity_ContestHall_Text_EnteredBunchOfContests, MSGBOX_NPC
	end

LilycoveCity_ContestHall_EventScript_Girl::
	msgbox LilycoveCity_ContestHall_Text_ManWhoWonEarlierHadPokeblocks, MSGBOX_NPC
	end

LilycoveCity_ContestHall_EventScript_Sailor::
	msgbox LilycoveCity_ContestHall_Text_IsntThisPlaceHumongous, MSGBOX_NPC
	end

LilycoveCity_ContestHall_EventScript_SmartContestMC::
	lock
	faceplayer
	msgbox LilycoveCity_ContestHall_Text_GiveItBestSmartAppeal, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_SMART_MC, Common_Movement_FaceOriginalDirection
	waitmovement 0
	release
	end

LilycoveCity_ContestHall_EventScript_SmartContestJudge::
	lock
	faceplayer
	msgbox LilycoveCity_ContestHall_Text_AreYouEnjoyingThisContest, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_SMART_JUDGE, Common_Movement_FaceOriginalDirection
	waitmovement 0
	release
	end

LilycoveCity_ContestHall_EventScript_SmartContestant1::
	lock
	faceplayer
	msgbox LilycoveCity_ContestHall_Text_EnteredWrongContest, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_SMART_CONTESTANT_1, Common_Movement_FaceOriginalDirection
	waitmovement 0
	release
	end

LilycoveCity_ContestHall_EventScript_SmartContestant2::
	lock
	faceplayer
	msgbox LilycoveCity_ContestHall_Text_RaisedMonToBeSmart, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_SMART_CONTESTANT_2, Common_Movement_FaceOriginalDirection
	waitmovement 0
	release
	end

LilycoveCity_ContestHall_EventScript_SmartContestant3::
	lock
	faceplayer
	msgbox LilycoveCity_ContestHall_Text_IfMonPullsSmartMoveNext, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_SMART_CONTESTANT_3, Common_Movement_FaceOriginalDirection
	waitmovement 0
	release
	end

LilycoveCity_ContestHall_EventScript_SmartContestant4::
	lock
	faceplayer
	msgbox LilycoveCity_ContestHall_Text_DontAppreciateCuteLeechLife, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_SMART_CONTESTANT_4, Common_Movement_FaceOriginalDirection
	waitmovement 0
	release
	end

LilycoveCity_ContestHall_EventScript_SmartContestAudience1::
	msgbox LilycoveCity_ContestHall_Text_YoureBeautifulGrandpa, MSGBOX_SIGN
	end

LilycoveCity_ContestHall_EventScript_SmartContestAudience2::
	lock
	faceplayer
	msgbox LilycoveCity_ContestHall_Text_AllSeemToUseDifferentMoves, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_SMART_AUDIENCE_2, Common_Movement_FaceOriginalDirection
	waitmovement 0
	release
	end

LilycoveCity_ContestHall_EventScript_SmartContestAudience3::
	lock
	faceplayer
	msgbox LilycoveCity_ContestHall_Text_PokemonSmarterThanTrainers, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_SMART_AUDIENCE_3, Common_Movement_FaceOriginalDirection
	waitmovement 0
	release
	end

LilycoveCity_ContestHall_EventScript_SmartContestAudience4::
	lock
	faceplayer
	msgbox LilycoveCity_ContestHall_Text_StillLoveSmartnessContests, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_SMART_AUDIENCE_4, Common_Movement_FaceOriginalDirection
	waitmovement 0
	release
	end

LilycoveCity_ContestHall_EventScript_BeautyContestMC::
	lock
	faceplayer
	msgbox LilycoveCity_ContestHall_Text_AreYouEnteringBeautyContest, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_BEAUTY_MC, Common_Movement_FaceOriginalDirection
	waitmovement 0
	release
	end

LilycoveCity_ContestHall_EventScript_BeautyContestJudge::
	lock
	faceplayer
	msgbox LilycoveCity_ContestHall_Text_EveryPokemonPristineBeauty, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_BEAUTY_JUDGE, Common_Movement_FaceOriginalDirection
	waitmovement 0
	release
	end

LilycoveCity_ContestHall_EventScript_BeautyContestant1::
	lock
	faceplayer
	msgbox LilycoveCity_ContestHall_Text_EyesWillBeGluedToMyBeauty, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_BEAUTY_CONTESTANT_1, Common_Movement_FaceOriginalDirection
	waitmovement 0
	release
	end

LilycoveCity_ContestHall_EventScript_BeautyContestant2::
	lock
	faceplayer
	msgbox LilycoveCity_ContestHall_Text_OverdidGrooming, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_BEAUTY_CONTESTANT_2, Common_Movement_FaceOriginalDirection
	waitmovement 0
	release
	end

LilycoveCity_ContestHall_EventScript_BeautyContestant3::
	lock
	faceplayer
	msgbox LilycoveCity_ContestHall_Text_JudgeWontSeeAuroraBeam, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_BEAUTY_CONTESTANT_3, Common_Movement_FaceOriginalDirection
	waitmovement 0
	release
	end

LilycoveCity_ContestHall_EventScript_BeautyContestant4::
	lock
	faceplayer
	msgbox LilycoveCity_ContestHall_Text_PokemonLooksLikeYoungerMe, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_BEAUTY_CONTESTANT_4, Common_Movement_FaceOriginalDirection
	waitmovement 0
	release
	end

LilycoveCity_ContestHall_EventScript_BeautyContestAudience1::
	lock
	faceplayer
	msgbox LilycoveCity_ContestHall_Text_WinBeautyContestMakesMeHappy, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_BEAUTY_AUDIENCE_1, Common_Movement_FaceOriginalDirection
	waitmovement 0
	release
	end

LilycoveCity_ContestHall_EventScript_BeautyContestAudience2::
	lock
	faceplayer
	msgbox LilycoveCity_ContestHall_Text_GanderAtAllThosePrettyPokemon, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_BEAUTY_AUDIENCE_2, Common_Movement_FaceOriginalDirection
	waitmovement 0
	release
	end

LilycoveCity_ContestHall_EventScript_BeautyContestAudience3::
	lock
	faceplayer
	msgbox LilycoveCity_ContestHall_Text_CantWinOnBeautyAlone, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_BEAUTY_AUDIENCE_3, Common_Movement_FaceOriginalDirection
	waitmovement 0
	release
	end

LilycoveCity_ContestHall_EventScript_CuteContestMC::
	lock
	faceplayer
	msgbox LilycoveCity_ContestHall_Text_InTheMiddleOfContest, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_CUTE_MC, Common_Movement_FaceOriginalDirection
	waitmovement 0
	release
	end

LilycoveCity_ContestHall_EventScript_CuteContestJudge::
	lock
	faceplayer
	msgbox LilycoveCity_ContestHall_Text_SuchCharmingCuteAppeals, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_CUTE_JUDGE, Common_Movement_FaceOriginalDirection
	waitmovement 0
	release
	end

LilycoveCity_ContestHall_EventScript_CuteContestant1::
	lock
	faceplayer
	msgbox LilycoveCity_ContestHall_Text_MyAzurillWasDistracted, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_CUTE_CONTESTANT_1, Common_Movement_FaceOriginalDirection
	waitmovement 0
	release
	end

LilycoveCity_ContestHall_EventScript_CuteContestant2::
	lock
	faceplayer
	msgbox LilycoveCity_ContestHall_Text_NeverWonBattleButContest, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_CUTE_CONTESTANT_2, Common_Movement_FaceOriginalDirection
	waitmovement 0
	release
	end

LilycoveCity_ContestHall_EventScript_CuteContestant3::
	lock
	faceplayer
	msgbox LilycoveCity_ContestHall_Text_PetalDanceIsMarvel, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_CUTE_CONTESTANT_3, Common_Movement_FaceOriginalDirection
	waitmovement 0
	release
	end

LilycoveCity_ContestHall_EventScript_CuteContestant4::
	lock
	faceplayer
	msgbox LilycoveCity_ContestHall_Text_MyMonAppealSoMuchCuter, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_CUTE_CONTESTANT_4, Common_Movement_FaceOriginalDirection
	waitmovement 0
	release
	end

LilycoveCity_ContestHall_EventScript_CuteContestAudience1::
	lockall
	applymovement LOCALID_CUTE_AUDIENCE_1, Common_Movement_FacePlayer
	waitmovement 0
	msgbox LilycoveCity_ContestHall_Text_MyChildIsInContest, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_CUTE_AUDIENCE_1, Common_Movement_FaceOriginalDirection
	waitmovement 0
	delay 25
	msgbox LilycoveCity_ContestHall_Text_ComeOnDear, MSGBOX_DEFAULT
	releaseall
	end

LilycoveCity_ContestHall_EventScript_CuteContestAudience2::
	lock
	faceplayer
	msgbox LilycoveCity_ContestHall_Text_WantCuteMonOfMyOwn, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_CUTE_AUDIENCE_2, Common_Movement_FaceOriginalDirection
	waitmovement 0
	release
	end

LilycoveCity_ContestHall_EventScript_CuteContestAudience3::
	lock
	faceplayer
	msgbox LilycoveCity_ContestHall_Text_ThatGirlThereIsCutest, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_CUTE_AUDIENCE_3, Common_Movement_FaceOriginalDirection
	waitmovement 0
	release
	end

LilycoveCity_ContestHall_EventScript_BeautyStageSign::
	msgbox LilycoveCity_ContestHall_Text_BeautyContestStage, MSGBOX_SIGN
	end

LilycoveCity_ContestHall_EventScript_CuteStageSign::
	msgbox LilycoveCity_ContestHall_Text_CuteContestStage, MSGBOX_SIGN
	end

LilycoveCity_ContestHall_EventScript_SmartStageSign::
	msgbox LilycoveCity_ContestHall_Text_SmartContestStage, MSGBOX_SIGN
	end

LilycoveCity_ContestHall_Text_TodayWonSmartnessContest:
	.string "Este POKéMON já ganhou o CONCURSO\n"
	.string "DE BELEZA aqui.\p"
	.string "E adivinha? Hoje ele ganhou um\n"
	.string "CONCURSO DE ESPERTEZA!\p"
	.string "O meu talento me assusta até…$"

LilycoveCity_ContestHall_Text_EnteredBunchOfContests:
	.string "Já entrei em um monte de CONCURSOS,\n"
	.string "então estou pegando o jeito.\p"
	.string "Num CONCURSO DE ESTILO, golpes\n"
	.string "estilosos fazem sucesso.\p"
	.string "Mas golpes espertos e fofos não\n"
	.string "fazem sucesso, por exemplo.$"

LilycoveCity_ContestHall_Text_ManWhoWonEarlierHadPokeblocks:
	.string "Aquele moço que ganhou agora há pouco\n"
	.string "tinha um monte de {POKEBLOCK}S diferentes.\p"
	.string "Será que dá pra ganhar com tantos\n"
	.string "desses?$"

LilycoveCity_ContestHall_Text_IsntThisPlaceHumongous:
	.string "Nossa, que lugar gigante!\p"
	.string "A tensão no ar… Não é como um\n"
	.string "salão de CONCURSO do interior.$"

LilycoveCity_ContestHall_Text_GiveItBestSmartAppeal:
	.string "APRESENTADOR: Certo, POKéMON ESPERTOS\n"
	.string "e TREINADORES, prontos?!\p"
	.string "Deem o seu melhor!\n"
	.string "Vamos às apresentações!$"

LilycoveCity_ContestHall_Text_AreYouEnjoyingThisContest:
	.string "JUIZ: Está gostando deste CONCURSO?\p"
	.string "Volte com três amigos, e todos\n"
	.string "vocês podem entrar num CONCURSO!$"

LilycoveCity_ContestHall_Text_EnteredWrongContest:
	.string "Aiaiaiai…\n"
	.string "Entrei no CONCURSO errado.\p"
	.string "Inscrevi este POKéMON durão no\n"
	.string "CONCURSO DE ESPERTEZA…\p"
	.string "Vamos, impressione com um ESMAGA\n"
	.string "PEDRA de cara esperta.$"

LilycoveCity_ContestHall_Text_RaisedMonToBeSmart:
	.string "Passei muitos anos, e toda a minha\n"
	.string "sabedoria, treinando este POKéMON\l"
	.string "pra ser esperto.\p"
	.string "Ele não vai perder fácil pro POKéMON\n"
	.string "de um novato qualquer.\p"
	.string "Querida, está vendo isto?$"

LilycoveCity_ContestHall_Text_IfMonPullsSmartMoveNext:
	.string "Agora!\p"
	.string "Se o meu POKéMON fizer um golpe esperto\n"
	.string "agora, a plateia vai ao delírio!$"

LilycoveCity_ContestHall_Text_DontAppreciateCuteLeechLife:
	.string "Ai, não!\p"
	.string "O SUGA-VIDAS do meu querido ZUBAT é\n"
	.string "tão fofo que usei sem parar!\p"
	.string "Mas esse povo malvado não dá o\n"
	.string "menor valor!$"

LilycoveCity_ContestHall_Text_YoureBeautifulGrandpa:
	.string "Kiaaah! Você está lindo, vovô!$"

LilycoveCity_ContestHall_Text_AllSeemToUseDifferentMoves:
	.string "Mesmo quando TREINADORES inscrevem o\n"
	.string "mesmo tipo de POKéMON, cada um usa\l"
	.string "golpes diferentes nas apresentações.\p"
	.string "É igual ao jeito que cada pessoa\n"
	.string "tem um estilo de batalhar.$"

LilycoveCity_ContestHall_Text_PokemonSmarterThanTrainers:
	.string "Acho que os POKéMON parecem mais\n"
	.string "espertos que os TREINADORES. E muito.$"

LilycoveCity_ContestHall_Text_StillLoveSmartnessContests:
	.string "No fim das contas, eu ainda amo o\n"
	.string "CONCURSO DE ESPERTEZA.\p"
	.string "Aquele verde intelectual…\n"
	.string "É tão… Tão… Estiloso.$"

LilycoveCity_ContestHall_Text_AreYouEnteringBeautyContest:
	.string "APRESENTADOR: Também vai entrar no\n"
	.string "CONCURSO DE BELEZA? Boa sorte!$"

LilycoveCity_ContestHall_Text_EveryPokemonPristineBeauty:
	.string "JUIZ: Ah, todo POKéMON aqui é um\n"
	.string "modelo de beleza imaculada!\p"
	.string "Até quase esqueci de dar as notas!$"

LilycoveCity_ContestHall_Text_EyesWillBeGluedToMyBeauty:
	.string "Um POKéMON tão lindo…\n"
	.string "Não existe outro igual.\p"
	.string "Todos os olhos vão grudar na\n"
	.string "minha beleza.$"

LilycoveCity_ContestHall_Text_OverdidGrooming:
	.string "Buááá!\p"
	.string "Escovei e arrumei o meu POKéMON com\n"
	.string "cuidado pra este CONCURSO…\p"
	.string "Mas exagerei…\n"
	.string "O pelo dele ficou todo arrepiado…\p"
	.string "O que eu faço?$"

LilycoveCity_ContestHall_Text_JudgeWontSeeAuroraBeam:
	.string "Este RAIO AURORA é tão ofuscante\n"
	.string "que o JUIZ nem vai conseguir ver.\p"
	.string "Hã, pera aí…\n"
	.string "Então não vai adiantar nada!$"

LilycoveCity_ContestHall_Text_PokemonLooksLikeYoungerMe:
	.string "Este POKéMON bonito é igualzinho\n"
	.string "a mim quando eu era mais nova.\p"
	.string "Quando eu disse isso, o meu marido\n"
	.string "cuspiu o café que estava bebendo.\p"
	.string "Eu disse alguma coisa engraçada?$"

LilycoveCity_ContestHall_Text_WinBeautyContestMakesMeHappy:
	.string "Sabe, ganhar um CONCURSO DE BELEZA\n"
	.string "me deixa feliz. Mais feliz que o\l"
	.string "normal, pelo menos.$"

LilycoveCity_ContestHall_Text_GanderAtAllThosePrettyPokemon:
	.string "Wahahahah!\p"
	.string "Dá só uma olhada em todos esses\n"
	.string "POKéMON bonitos!\l"
	.string "Eu adoro esse brilho todo!$"

LilycoveCity_ContestHall_Text_CantWinOnBeautyAlone:
	.string "Nem sempre dá pra ganhar só com\n"
	.string "beleza.\p"
	.string "Tem que cuidar do POKéMON pra ele\n"
	.string "ficar bonito e brilhante como o meu.$"

LilycoveCity_ContestHall_Text_InTheMiddleOfContest:
	.string "APRESENTADOR: Opa! Olá!\n"
	.string "Estamos no meio de um CONCURSO!\p"
	.string "Faça a inscrição no balcão e\n"
	.string "venha, tá bom?$"

LilycoveCity_ContestHall_Text_SuchCharmingCuteAppeals:
	.string "JUIZ: Ah, que apresentações\n"
	.string "encantadoras e fofas!\p"
	.string "Minha nossa! Que apresentação de\n"
	.string "ESPORTE ÁGUA mais adorável!$"

LilycoveCity_ContestHall_Text_MyAzurillWasDistracted:
	.string "Ah, não… O meu AZURILL querido se\n"
	.string "distraiu com outro POKéMON.$"

LilycoveCity_ContestHall_Text_NeverWonBattleButContest:
	.string "O meu POKéMON nunca ganhou uma\n"
	.string "batalha, mas num CONCURSO é outra\l"
	.string "história!$"

LilycoveCity_ContestHall_Text_PetalDanceIsMarvel:
	.string "A DANÇA PÉTALA do meu POKéMON é uma\n"
	.string "maravilha de elegância.\p"
	.string "Não vou deixar ninguém atrapalhar\n"
	.string "a apresentação dele.$"

LilycoveCity_ContestHall_Text_MyMonAppealSoMuchCuter:
	.string "Os POKéMON de todo mundo são\n"
	.string "muito fofos.\p"
	.string "Mas, se o meu POKéMON fizesse uma\n"
	.string "apresentação fofa…\p"
	.string "Tenho certeza de que seria muito\n"
	.string "mais fofo que os outros.$"

LilycoveCity_ContestHall_Text_MyChildIsInContest:
	.string "O meu filho está neste CONCURSO.$"

LilycoveCity_ContestHall_Text_ComeOnDear:
	.string "Vai, filho. Manda ver!\n"
	.string "O seu POKéMON é o melhor!$"

LilycoveCity_ContestHall_Text_ThatGirlThereIsCutest:
	.string "Acho aquela menina ali a mais fofa\n"
	.string "de todas.\p"
	.string "Como? Eles julgam os POKéMON pela\n"
	.string "aparência?$"

LilycoveCity_ContestHall_Text_WantCuteMonOfMyOwn:
	.string "Ah, vendo esses POKéMON fofos, deu\n"
	.string "vontade de ter os meus!\p"
	.string "Vou pegar alguns!$"

LilycoveCity_ContestHall_Text_BeautyContestStage:
	.string "PALCO DO CONCURSO DE BELEZA\n"
	.string "ENCANTE-SE COM POKéMON LINDOS!$"

LilycoveCity_ContestHall_Text_CuteContestStage:
	.string "PALCO DO CONCURSO DE FOFURA\n"
	.string "DERRETA-SE COM POKéMON FOFOS!$"

LilycoveCity_ContestHall_Text_SmartContestStage:
	.string "PALCO DO CONCURSO DE ESPERTEZA\n"
	.string "IMPRESSIONE-SE COM POKéMON ESPERTOS!$"

