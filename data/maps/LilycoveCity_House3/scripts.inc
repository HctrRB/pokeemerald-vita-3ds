LilycoveCity_House3_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, LilycoveCity_House3_OnTransition
	.byte 0

LilycoveCity_House3_OnTransition:
	random 4
	copyvar VAR_TEMP_1, VAR_RESULT
	end

LilycoveCity_House3_EventScript_PokefanF::
	lock
	faceplayer
	msgbox LilycoveCity_House3_Text_LearnFromMasterOfPokeblocks, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, LilycoveCity_House3_EventScript_DeclinePokeblockLearn
	msgbox LilycoveCity_House3_Text_ExplainPokeblocks, MSGBOX_DEFAULT
	closemessage
	applymovement VAR_LAST_TALKED, Common_Movement_FaceOriginalDirection
	waitmovement 0
	release
	end

LilycoveCity_House3_EventScript_DeclinePokeblockLearn::
	msgbox LilycoveCity_House3_Text_OhAreYouSure, MSGBOX_DEFAULT
	closemessage
	applymovement VAR_LAST_TALKED, Common_Movement_FaceOriginalDirection
	waitmovement 0
	release
	end

LilycoveCity_House3_EventScript_Man::
	lock
	faceplayer
	msgbox LilycoveCity_House3_Text_HappyToHaveQuadruplets, MSGBOX_DEFAULT
	closemessage
	applymovement VAR_LAST_TALKED, Common_Movement_FaceOriginalDirection
	waitmovement 0
	release
	end

LilycoveCity_House3_EventScript_GameBoyKid1::
	lock
	faceplayer
	switch VAR_TEMP_1
	case 0, LilycoveCity_House3_EventScript_WereDoingMultiBattle
	case 1, LilycoveCity_House3_EventScript_WereMixingRecords
	case 2, LilycoveCity_House3_EventScript_WereBlendingBerries
	case 3, LilycoveCity_House3_EventScript_WereDoingContest
	end

LilycoveCity_House3_EventScript_GameBoyKid2::
	lock
	faceplayer
	switch VAR_TEMP_1
	case 0, LilycoveCity_House3_EventScript_WereDoingMultiBattle
	case 1, LilycoveCity_House3_EventScript_WereMixingRecords
	case 2, LilycoveCity_House3_EventScript_WereBlendingBerries
	case 3, LilycoveCity_House3_EventScript_WereDoingContest
	end

LilycoveCity_House3_EventScript_GameBoyKid3::
	lock
	faceplayer
	switch VAR_TEMP_1
	case 0, LilycoveCity_House3_EventScript_WereDoingMultiBattle
	case 1, LilycoveCity_House3_EventScript_WereMixingRecords
	case 2, LilycoveCity_House3_EventScript_WereBlendingBerries
	case 3, LilycoveCity_House3_EventScript_WereDoingContest
	end

LilycoveCity_House3_EventScript_GameBoyKid4::
	lock
	faceplayer
	switch VAR_TEMP_1
	case 0, LilycoveCity_House3_EventScript_WereDoingMultiBattle
	case 1, LilycoveCity_House3_EventScript_WereMixingRecords
	case 2, LilycoveCity_House3_EventScript_WereBlendingBerries
	case 3, LilycoveCity_House3_EventScript_WereDoingContest
	end

LilycoveCity_House3_EventScript_WereDoingMultiBattle::
	msgbox LilycoveCity_House3_Text_GoingToWinMultiBattles, MSGBOX_DEFAULT
	release
	end

LilycoveCity_House3_EventScript_WereMixingRecords::
	msgbox LilycoveCity_House3_Text_LikeMixingAtRecordCorner, MSGBOX_DEFAULT
	release
	end

LilycoveCity_House3_EventScript_WereBlendingBerries::
	msgbox LilycoveCity_House3_Text_MakePokeblocksWithBerryBlender, MSGBOX_DEFAULT
	release
	end

LilycoveCity_House3_EventScript_WereDoingContest::
	msgbox LilycoveCity_House3_Text_GoingToEnterContest, MSGBOX_DEFAULT
	release
	end

LilycoveCity_House3_Text_LearnFromMasterOfPokeblocks:
	.string "Ora, ora! Viajando por conta própria?\n"
	.string "Nessa idade! Que bom pra você!\p"
	.string "Os meus filhos podiam aprender\n"
	.string "uma coisa ou outra com você!\p"
	.string "Eu? Sou mestra em {POKEBLOCK}S.\p"
	.string "Se eu me esforçar só um pouquinho,\n"
	.string "faço {POKEBLOCK}S ótimos.\p"
	.string "Quer aprender comigo, uma mestra\n"
	.string "em {POKEBLOCK}S?$"

LilycoveCity_House3_Text_OhAreYouSure:
	.string "Hã? Tem certeza?\p"
	.string "Não dá pra fazer tudo sozinho\n"
	.string "sempre, meu bem!$"

LilycoveCity_House3_Text_ExplainPokeblocks:
	.string "Ah, que bom! Que cabeça boa!\n"
	.string "É meio longo, então preste atenção!\p"
	.string "Pronto?\p"
	.string "Olhando os {POKEBLOCK}S, você vê que\n"
	.string "eles têm uma nota de textura.\p"
	.string "Quanto menor a nota, melhor.\n"
	.string "Não esqueça disso!\p"
	.string "Um {POKEBLOCK} bom tem nota de\n"
	.string "textura baixa e nível alto.\p"
	.string "Um POKéMON também come mais\n"
	.string "{POKEBLOCK}S bons do que comuns.\p"
	.string "E isto é importante.\p"
	.string "Pra fazer {POKEBLOCK}S macios, use\n"
	.string "FRUTAS de tipos diferentes.\p"
	.string "Não seja pão-duro: o tipo de FRUTA\n"
	.string "afeta a maciez dos {POKEBLOCK}S.\p"
	.string "E mais uma coisa.\p"
	.string "Quanto mais gente batendo FRUTAS,\n"
	.string "mais macios os {POKEBLOCK}S.\p"
	.string "Por isso, converse com outras\n"
	.string "pessoas e façam {POKEBLOCK}S juntos.\p"
	.string "É basicamente isso que você precisa\n"
	.string "pra fazer {POKEBLOCK}S bons.\p"
	.string "Se todos tivessem POKéMON queridos,\n"
	.string "{POKEBLOCK}S macios e uma família amorosa,\l"
	.string "o mundo seria mais feliz.\p"
	.string "Não desista, meu bem!$"

LilycoveCity_House3_Text_HappyToHaveQuadruplets:
	.string "Quando a minha esposa teve\n"
	.string "quadrigêmeos, levei um susto e tanto.\p"
	.string "Mas agora, vendo eles brincarem\n"
	.string "juntos, fico feliz.$"

LilycoveCity_House3_Text_GoingToWinMultiBattles:
	.string "Estamos fazendo BATALHAS MÚLTIPLAS,\n"
	.string "mas sei que eu vou ganhar.$"

LilycoveCity_House3_Text_LikeMixingAtRecordCorner:
	.string "A gente gosta de misturar coisas\n"
	.string "no CANTO DOS RECORDES.\p"
	.string "Mas o que é que se mistura?$"

LilycoveCity_House3_Text_MakePokeblocksWithBerryBlender:
	.string "Vamos fazer {POKEBLOCK}S super com um\n"
	.string "LIQUIDIFICADOR DE FRUTAS!$"

LilycoveCity_House3_Text_GoingToEnterContest:
	.string "Quero mostrar como o meu POKéMON é\n"
	.string "forte, então vamos entrar num\l"
	.string "CONCURSO juntos.$"

