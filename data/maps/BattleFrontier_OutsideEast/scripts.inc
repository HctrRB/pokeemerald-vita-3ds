BattleFrontier_OutsideEast_MapScripts::
	map_script MAP_SCRIPT_ON_RESUME, BattleFrontier_OutsideEast_OnResume
	map_script MAP_SCRIPT_ON_TRANSITION, BattleFrontier_OutsideEast_OnTransition
	.byte 0

BattleFrontier_OutsideEast_OnResume:
	call_if_set FLAG_SYS_CTRL_OBJ_DELETE, BattleFrontier_OutsideEast_EventScript_TryRemoveSudowoodo
	end

BattleFrontier_OutsideEast_EventScript_TryRemoveSudowoodo::
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, Common_EventScript_NopReturn
	removeobject VAR_LAST_TALKED
	return

BattleFrontier_OutsideEast_OnTransition:
	setvar VAR_BRAVO_TRAINER_BATTLE_TOWER_ON, 0
	setflag FLAG_HIDE_BATTLE_TOWER_REPORTER
	call_if_unset FLAG_DEFEATED_SUDOWOODO, BattleFrontier_OutsideEast_EventScript_ShowSudowoodo
	end

BattleFrontier_OutsideEast_EventScript_ShowSudowoodo::
	clearflag FLAG_HIDE_BATTLE_FRONTIER_SUDOWOODO
	return

BattleFrontier_OutsideEast_EventScript_BattleTowerSign::
	msgbox BattleFrontier_OutsideEast_Text_BattleTowerSign, MSGBOX_SIGN
	end

BattleFrontier_OutsideEast_EventScript_BattlePalaceSign::
	msgbox BattleFrontier_OutsideEast_Text_BattlePalaceSign, MSGBOX_SIGN
	end

BattleFrontier_OutsideEast_EventScript_BattleArenaSign::
	msgbox BattleFrontier_OutsideEast_Text_BattleArenaSign, MSGBOX_SIGN
	end

BattleFrontier_OutsideEast_EventScript_BattlePyramidSign::
	msgbox BattleFrontier_OutsideEast_Text_BattlePyramidSign, MSGBOX_SIGN
	end

BattleFrontier_OutsideEast_EventScript_NinjaBoy::
	msgbox BattleFrontier_OutsideEast_Text_BattleTowerFeelsSpecial, MSGBOX_NPC
	end

BattleFrontier_OutsideEast_EventScript_Man1::
	msgbox BattleFrontier_OutsideEast_Text_ConquerLeagueAndFrontier, MSGBOX_NPC
	end

BattleFrontier_OutsideEast_EventScript_Hiker::
	msgbox BattleFrontier_OutsideEast_Text_PyramidTooHarsh, MSGBOX_NPC
	end

BattleFrontier_OutsideEast_EventScript_HexManiac::
	msgbox BattleFrontier_OutsideEast_Text_ThriveInDarkness, MSGBOX_NPC
	end

BattleFrontier_OutsideEast_EventScript_BlackBelt1::
	msgbox BattleFrontier_OutsideEast_Text_PutTogetherUltimateTeam, MSGBOX_NPC
	end

BattleFrontier_OutsideEast_EventScript_Cook::
	lock
	msgbox BattleFrontier_OutsideEast_Text_BelieveInYouBuddy, MSGBOX_DEFAULT
	release
	end

BattleFrontier_OutsideEast_EventScript_Zigzagoon::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_ZIGZAGOON, CRY_MODE_NORMAL
	msgbox BattleFrontier_OutsideEast_Text_ZigzagoonLooksVacant, MSGBOX_DEFAULT
	waitmoncry
	release
	end

BattleFrontier_OutsideEast_EventScript_RichBoy::
	msgbox BattleFrontier_OutsideEast_Text_PeopleCallMeBusybody, MSGBOX_NPC
	end

BattleFrontier_OutsideEast_EventScript_ExpertF::
	msgbox BattleFrontier_OutsideEast_Text_OnceBeatGymLeader, MSGBOX_NPC
	end

BattleFrontier_OutsideEast_EventScript_TriathleteF::
	msgbox BattleFrontier_OutsideEast_Text_FastOnBikeAndBattles, MSGBOX_NPC
	end

BattleFrontier_OutsideEast_EventScript_Twin::
	msgbox BattleFrontier_OutsideEast_Text_BetterThanDaddyAtPokemon, MSGBOX_NPC
	end

BattleFrontier_OutsideEast_EventScript_Man2::
	lock
	msgbox BattleFrontier_OutsideEast_Text_GoRackUpSomeWinsForDaddy, MSGBOX_DEFAULT
	release
	end

BattleFrontier_OutsideEast_EventScript_TriathleteM::
	msgbox BattleFrontier_OutsideEast_Text_DidScottBringYouHere, MSGBOX_NPC
	end

BattleFrontier_OutsideEast_EventScript_Sudowoodo::
	lock
	faceplayer
	waitse
	playse SE_SUDOWOODO_SHAKE
	applymovement LOCALID_FRONTIER_SUDOWOODO, BattleFrontier_OutsideEast_Movement_SudowoodoShake
	waitmovement 0
	release
	end

BattleFrontier_OutsideEast_EventScript_WaterSudowoodo::
	lock
	faceplayer
	special DoWateringBerryTreeAnim
	waitstate
	waitse
	playse SE_SUDOWOODO_SHAKE
	applymovement LOCALID_FRONTIER_SUDOWOODO, BattleFrontier_OutsideEast_Movement_SudowoodoShake
	waitmovement 0
	msgbox gText_Sudowoodo_Attacked, MSGBOX_DEFAULT
	closemessage
	waitse
	playmoncry SPECIES_SUDOWOODO, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setvar VAR_LAST_TALKED, LOCALID_FRONTIER_SUDOWOODO
	setwildbattle SPECIES_SUDOWOODO, 40
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	dowildbattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_eq VAR_RESULT, B_OUTCOME_WON, BattleFrontier_OutsideEast_EventScript_DefeatedSudowoodo
	goto_if_eq VAR_RESULT, B_OUTCOME_RAN, BattleFrontier_OutsideEast_EventScript_DefeatedSudowoodo
	goto_if_eq VAR_RESULT, B_OUTCOME_PLAYER_TELEPORTED, BattleFrontier_OutsideEast_EventScript_DefeatedSudowoodo
	setflag FLAG_DEFEATED_SUDOWOODO
	release
	end

BattleFrontier_OutsideEast_EventScript_DefeatedSudowoodo::
	setflag FLAG_DEFEATED_SUDOWOODO
	goto Common_EventScript_RemoveStaticPokemon
	end

BattleFrontier_OutsideEast_Movement_SudowoodoShake:
	face_right
	delay_8
	face_down
	delay_8
	face_left
	delay_8
	face_down
	delay_8
	face_right
	delay_8
	face_down
	delay_8
	face_left
	delay_8
	face_down
	step_end

BattleFrontier_OutsideEast_EventScript_Maniac1::
	msgbox BattleFrontier_OutsideEast_Text_HeardPrettyGirlAtBattleArena, MSGBOX_NPC
	end

BattleFrontier_OutsideEast_EventScript_Girl::
	msgbox BattleFrontier_OutsideEast_Text_SometimesImportantOldManInThere, MSGBOX_NPC
	end

BattleFrontier_OutsideEast_EventScript_PsychicM::
	msgbox BattleFrontier_OutsideEast_Text_LegendOfBattlePyramid, MSGBOX_NPC
	end

BattleFrontier_OutsideEast_EventScript_Man3::
	msgbox BattleFrontier_OutsideEast_Text_GotWipedOut, MSGBOX_NPC
	end

BattleFrontier_OutsideEast_EventScript_Woman1::
	msgbox BattleFrontier_OutsideEast_Text_ToughTrainerInBattleTower, MSGBOX_NPC
	end

BattleFrontier_OutsideEast_EventScript_Woman2::
	msgbox BattleFrontier_OutsideEast_Text_EnoughBattlePointsForDoll, MSGBOX_NPC
	end

BattleFrontier_OutsideEast_EventScript_BlackBelt2::
	msgbox BattleFrontier_OutsideEast_Text_LikeToHaveNameRecordedHere, MSGBOX_NPC
	end

BattleFrontier_OutsideEast_EventScript_Maniac2::
	lock
	msgbox BattleFrontier_OutsideEast_Text_CanDoAnythingWithYou, MSGBOX_DEFAULT
	release
	end

BattleFrontier_OutsideEast_EventScript_Woman3::
	lock
	msgbox BattleFrontier_OutsideEast_Text_PowerOfOurLoveWillOvercome, MSGBOX_DEFAULT
	release
	end

BattleFrontier_OutsideEast_EventScript_RankingHallSign::
	msgbox BattleFrontier_OutsideEast_Text_RankingHallSign, MSGBOX_SIGN
	end

BattleFrontier_OutsideEast_EventScript_ExchangeCornerSign::
	msgbox BattleFrontier_OutsideEast_Text_ExchangeCornerSign, MSGBOX_SIGN
	end

BattleFrontier_OutsideEast_EventScript_Gentleman::
	msgbox BattleFrontier_OutsideEast_Text_FrontierNotExclusivelyForToughTrainers, MSGBOX_NPC
	end

BattleFrontier_OutsideEast_EventScript_OldWoman::
	lock
	faceplayer
	goto_if_set FLAG_DEFEATED_SUDOWOODO, BattleFrontier_OutsideEast_EventScript_OldWomanSudowoodoGone
	msgbox BattleFrontier_OutsideEast_Text_OddTreeHereSeemsToWiggle, MSGBOX_DEFAULT
	release
	end

BattleFrontier_OutsideEast_EventScript_OldWomanSudowoodoGone::
	msgbox BattleFrontier_OutsideEast_Text_OnceAnOddTreePastHere, MSGBOX_DEFAULT
	release
	end

BattleFrontier_OutsideEast_EventScript_Camper::
	msgbox BattleFrontier_OutsideEast_Text_StickyMonWithLongTail, MSGBOX_NPC
	end

BattleFrontier_OutsideEast_Text_BattleTowerSign:
	.string "Esta é a TORRE DE BATALHA!\n"
	.string "Mantenha a sequência de vitórias\l"
	.string "como o TREINADOR mais forte!$"

BattleFrontier_OutsideEast_Text_BattlePalaceSign:
	.string "Este é o PALÁCIO DE BATALHA!\n"
	.string "De olho nas batalhas POKéMON!$"

BattleFrontier_OutsideEast_Text_BattleArenaSign:
	.string "Esta é a ARENA DE BATALHA!\n"
	.string "Que venham as equipes mais fortes!$"

BattleFrontier_OutsideEast_Text_BattlePyramidSign:
	.string "Esta é a PIRÂMIDE DE BATALHA!\n"
	.string "Avance pela Missão de Batalha!$"

BattleFrontier_OutsideEast_Text_RankingHallSign:
	.string "SALÃO DO RANKING DA FRONTEIRA\n"
	.string "Mire em novos recordes!$"

BattleFrontier_OutsideEast_Text_ExchangeCornerSign:
	.string "CENTRAL DE TROCAS DE PONTOS\n"
	.string "Troque os seus Pontos de Batalha!$"

BattleFrontier_OutsideEast_Text_BattleTowerFeelsSpecial:
	.string "Uau!\n"
	.string "Que enorme!\p"
	.string "A TORRE DE BATALHA parece especial.\n"
	.string "É diferente das outras.$"

BattleFrontier_OutsideEast_Text_ConquerLeagueAndFrontier:
	.string "A minha grande ambição é conquistar a\n"
	.string "FRONTEIRA DE BATALHA e a LIGA\l"
	.string "POKéMON.\p"
	.string "E, se o meu sonho se realizar…\p"
	.string "E-eu vou perguntar à MIMI, a vizinha,\n"
	.string "se ela quer ser minha amiga!$"

BattleFrontier_OutsideEast_Text_PyramidTooHarsh:
	.string "A PIRÂMIDE DE BATALHA é dura demais!\n"
	.string "Não consigo chegar ao topo!\p"
	.string "Sem opção, talvez eu possa escalar\n"
	.string "por fora…$"

BattleFrontier_OutsideEast_Text_ThriveInDarkness:
	.string "Eu floresço na escuridão…\n"
	.string "Sim… O que é digno de mim?\l"
	.string "Nada menos que a PIRÂMIDE…\p"
	.string "Que tal vagar na escuridão, em\n"
	.string "desespero total e absoluto?$"

BattleFrontier_OutsideEast_Text_PutTogetherUltimateTeam:
	.string "Fiquei uma semana sem dormir, mas\n"
	.string "montei a equipe definitiva!\p"
	.string "Não me vejo perdendo, sério.\n"
	.string "Vamos invadir a ARENA DE BATALHA!$"

BattleFrontier_OutsideEast_Text_DidScottBringYouHere:
	.string "O SCOTT também te trouxe?\n"
	.string "Qual é a desse cara?\p"
	.string "Parece que ele sabe mesmo achar\n"
	.string "TREINADORES talentosos, mas…\p"
	.string "Nunca vi ele batalhando.$"

BattleFrontier_OutsideEast_Text_BelieveInYouBuddy:
	.string "ZIGG!\n"
	.string "Eu acredito em você, amiguinho!\p"
	.string "Se ganhar, faço todos os petiscos\n"
	.string "que o meu ZIGG gosta!$"

BattleFrontier_OutsideEast_Text_ZigzagoonLooksVacant:
	.string "O ZIGZAGOON parece aéreo…$"

BattleFrontier_OutsideEast_Text_PeopleCallMeBusybody:
	.string "Dizem que eu sou intrometida,\n"
	.string "mas não consigo evitar.\p"
	.string "O seu chapéu está torto!\n"
	.string "Ai, lixo no chão!\l"
	.string "Opa, já está quase na hora da janta!\p"
	.string "Não sei se aguento só assistir no\n"
	.string "PALÁCIO DE BATALHA…$"

BattleFrontier_OutsideEast_Text_OnceBeatGymLeader:
	.string "Posso não impressionar agora, mas\n"
	.string "já venci um LÍDER de GINÁSIO.\p"
	.string "Quem sabe eu não tento a glória\n"
	.string "de novo.$"

BattleFrontier_OutsideEast_Text_FastOnBikeAndBattles:
	.string "Sou rápido de BICICLETA, e na\n"
	.string "batalha também.\p"
	.string "Venço num piscar de olhos!$"

BattleFrontier_OutsideEast_Text_BetterThanDaddyAtPokemon:
	.string "Sou melhor que o meu papai\n"
	.string "em POKéMON.$"

BattleFrontier_OutsideEast_Text_GoRackUpSomeWinsForDaddy:
	.string "Vai, minha fofura!\n"
	.string "Ganha umas vitórias pro papai!\p"
	.string "Você é igualzinha à sua mãe,\n"
	.string "então vai arrasar!$"

BattleFrontier_OutsideEast_Text_HeardPrettyGirlAtBattleArena:
	.string "Não foi fácil chegar aqui, mas eu\n"
	.string "ouvi falar de uma moça linda de\l"
	.string "morrer na ARENA DE BATALHA.\p"
	.string "E sabe o que eu achei no lugar\n"
	.string "dela?\p"
	.string "Um bando de lutadores suados,\n"
	.string "fedidos e primitivos!$"

BattleFrontier_OutsideEast_Text_SometimesImportantOldManInThere:
	.string "Às vezes vejo um velhinho com cara\n"
	.string "de importante entrando ali.\p"
	.string "Ele tem uns POKéMON incríveis.\n"
	.string "Mas parece ser bem gente boa.$"

BattleFrontier_OutsideEast_Text_LegendOfBattlePyramid:
	.string "Conhece?\n"
	.string "A lenda da PIRÂMIDE DE BATALHA?\p"
	.string "Quando vier um TREINADOR confiante\n"
	.string "buscando o topo dourado, surgirá\l"
	.string "um homem entre os homens.\p"
	.string "Não conhece essa lenda?\n"
	.string "Claro que não!\l"
	.string "Acabei de inventar!\p"
	.string "O que significa?\n"
	.string "Isso, amigo, não posso dizer!$"

BattleFrontier_OutsideEast_Text_GotWipedOut:
	.string "Cara! Ai, cara!\n"
	.string "Eu nunca tinha perdido!\p"
	.string "Mas fui arrasado sem dó nenhuma!\p"
	.string "E ainda me dizem que não tenho\n"
	.string "talento?! Cara!$"

BattleFrontier_OutsideEast_Text_ToughTrainerInBattleTower:
	.string "Esse cara topou com um TREINADOR\n"
	.string "terrivelmente forte no desafio da\l"
	.string "TORRE DE BATALHA.\p"
	.string "Agora olha pra ele…\p"
	.string "Tome cuidado se um dia for à\n"
	.string "TORRE DE BATALHA.$"

BattleFrontier_OutsideEast_Text_EnoughBattlePointsForDoll:
	.string "Isso!\n"
	.string "Finalmente tenho Pontos de Batalha!\p"
	.string "Acho que vou trocar por mais um\n"
	.string "BONECO de pelúcia gigante!$"

BattleFrontier_OutsideEast_Text_LikeToHaveNameRecordedHere:
	.string "Ah, é, tá!\n"
	.string "Então este é o SALÃO DO RANKING!\p"
	.string "Quero entrar pra história como um\n"
	.string "supercampeão e ter o meu nome\l"
	.string "registrado aqui pra sempre.$"

BattleFrontier_OutsideEast_Text_CanDoAnythingWithYou:
	.string "Com você do meu lado, consigo tudo.$"

BattleFrontier_OutsideEast_Text_PowerOfOurLoveWillOvercome:
	.string "Ai, querido, você é maravilhoso!\p"
	.string "Ai, não aguento mais esperar!\p"
	.string "Vamos pra uma SALA DE BATALHA\n"
	.string "MÚLTIPLA agora mesmo!\p"
	.string "Se a gente se juntar no SALÃO DE\n"
	.string "BATALHA, o poder do nosso amor\l"
	.string "vence todo mundo!\p"
	.string "Diante da gente, querido, tudo vai\n"
	.string "cair que nem dominó!$"

BattleFrontier_OutsideEast_Text_FrontierNotExclusivelyForToughTrainers:
	.string "A FRONTEIRA DE BATALHA não é só\n"
	.string "para TREINADORES fortes.\p"
	.string "Parece que muita gente com\n"
	.string "habilidades especiais foi convidada.\p"
	.string "Aquele SCOTT…\p"
	.string "Talvez tenha construído este lugar\n"
	.string "com um objetivo maior…$"

BattleFrontier_OutsideEast_Text_OddTreeHereSeemsToWiggle:
	.string "Com licença, jovem.\n"
	.string "Você enxerga bem?\p"
	.string "Tem uma árvore estranha mais pra\n"
	.string "frente, e pros meus olhos cansados\l"
	.string "ela parece se mexer.$"

BattleFrontier_OutsideEast_Text_OnceAnOddTreePastHere:
	.string "Com licença, jovem.\n"
	.string "Você tem boa memória?\p"
	.string "Tenho a impressão de que já houve\n"
	.string "uma árvore estranha mais pra frente.$"

BattleFrontier_OutsideEast_Text_StickyMonWithLongTail:
	.string "E-eu…\n"
	.string "Eu vi!\p"
	.string "Tinha um POKéMON meio grudento de\n"
	.string "rabo comprido lá na frente!\p"
	.string "Ele se escondia debaixo de uma\n"
	.string "pedra e não parava de me encarar!$"
