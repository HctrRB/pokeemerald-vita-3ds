RustboroCity_DevonCorp_1F_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, RustboroCity_DevonCorp_1F_OnTransition
	.byte 0

RustboroCity_DevonCorp_1F_OnTransition:
	call_if_unset FLAG_RETURNED_DEVON_GOODS, RustboroCity_DevonCorp_1F_EventScript_BlockStairs
	end

RustboroCity_DevonCorp_1F_EventScript_BlockStairs::
	setobjectxyperm LOCALID_DEVON_CORP_STAIR_GUARD, 14, 2
	setobjectmovementtype LOCALID_DEVON_CORP_STAIR_GUARD, MOVEMENT_TYPE_FACE_DOWN
	return

RustboroCity_DevonCorp_1F_EventScript_Employee::
	lock
	faceplayer
	goto_if_set FLAG_RETURNED_DEVON_GOODS, RustboroCity_DevonCorp_1F_EventScript_GoodsRecovered
	goto_if_set FLAG_DEVON_GOODS_STOLEN, RustboroCity_DevonCorp_1F_EventScript_RobberWasntBright
	msgbox RustboroCity_DevonCorp_1F_Text_ThoseShoesAreOurProduct, MSGBOX_DEFAULT
	release
	end

RustboroCity_DevonCorp_1F_EventScript_RobberWasntBright::
	msgbox RustboroCity_DevonCorp_1F_Text_RobberWasntVeryBright, MSGBOX_DEFAULT
	release
	end

RustboroCity_DevonCorp_1F_EventScript_GoodsRecovered::
	msgbox RustboroCity_DevonCorp_1F_Text_SoundsLikeStolenGoodsRecovered, MSGBOX_DEFAULT
	release
	end

RustboroCity_DevonCorp_1F_EventScript_StairGuard::
	lock
	faceplayer
	goto_if_set FLAG_RETURNED_DEVON_GOODS, RustboroCity_DevonCorp_1F_EventScript_AlwaysWelcome
	goto_if_set FLAG_RECOVERED_DEVON_GOODS, RustboroCity_DevonCorp_1F_EventScript_GotRobbed
	goto_if_set FLAG_DEVON_GOODS_STOLEN, RustboroCity_DevonCorp_1F_EventScript_GotRobbed
	msgbox RustboroCity_DevonCorp_1F_Text_OnlyAuthorizedPeopleEnter, MSGBOX_DEFAULT
	release
	end

RustboroCity_DevonCorp_1F_EventScript_AlwaysWelcome::
	msgbox RustboroCity_DevonCorp_1F_Text_YoureAlwaysWelcomeHere, MSGBOX_DEFAULT
	release
	end

RustboroCity_DevonCorp_1F_EventScript_GotRobbed::
	msgbox RustboroCity_DevonCorp_1F_Text_HowCouldWeGetRobbed, MSGBOX_DEFAULT
	release
	end

RustboroCity_DevonCorp_1F_EventScript_Greeter::
	lock
	faceplayer
	goto_if_set FLAG_RETURNED_DEVON_GOODS, RustboroCity_DevonCorp_1F_EventScript_WelcomeToDevonCorp
	goto_if_set FLAG_RECOVERED_DEVON_GOODS, RustboroCity_DevonCorp_1F_EventScript_StaffGotRobbed
	goto_if_set FLAG_DEVON_GOODS_STOLEN, RustboroCity_DevonCorp_1F_EventScript_StaffGotRobbed
	msgbox RustboroCity_DevonCorp_1F_Text_WelcomeToDevonCorp, MSGBOX_DEFAULT
	release
	end

RustboroCity_DevonCorp_1F_EventScript_WelcomeToDevonCorp::
	msgbox RustboroCity_DevonCorp_1F_Text_WelcomeToDevonCorp, MSGBOX_DEFAULT
	release
	end

RustboroCity_DevonCorp_1F_EventScript_StaffGotRobbed::
	msgbox RustboroCity_DevonCorp_1F_Text_StaffGotRobbed, MSGBOX_DEFAULT
	release
	end

RustboroCity_DevonCorp_1F_EventScript_RocksMetalDisplay::
	msgbox RustboroCity_DevonCorp_1F_Text_RocksMetalDisplay, MSGBOX_SIGN
	end

RustboroCity_DevonCorp_1F_EventScript_ProductsDisplay::
	msgbox RustboroCity_DevonCorp_1F_Text_ProductDisplay, MSGBOX_SIGN
	end

RustboroCity_DevonCorp_1F_Text_WelcomeToDevonCorp:
	.string "Olá, boas-vindas à CORPORAÇÃO\n"
	.string "DEVON.\p"
	.string "Temos orgulho de fabricar itens e\n"
	.string "remédios que melhoram a sua vida.$"

RustboroCity_DevonCorp_1F_Text_StaffGotRobbed:
	.string "Um dos nossos pesquisadores bobeou\n"
	.string "e teve um pacote importante roubado.$"

RustboroCity_DevonCorp_1F_Text_ThoseShoesAreOurProduct:
	.string "Ei, esse TÊNIS DE CORRER!\n"
	.string "É um dos nossos produtos!\p"
	.string "Fico feliz quando vejo alguém\n"
	.string "usando algo que a gente fez.$"

RustboroCity_DevonCorp_1F_Text_RobberWasntVeryBright:
	.string "Aquele pacote roubado…\p"
	.string "Bom, é importante, claro, mas não é\n"
	.string "nada que qualquer um consiga usar.\p"
	.string "Na minha opinião, aquele ladrão\n"
	.string "não devia ser muito esperto.$"

RustboroCity_DevonCorp_1F_Text_SoundsLikeStolenGoodsRecovered:
	.string "Parece que recuperaram as PEÇAS\n"
	.string "DEVON que tinham sido roubadas.$"

RustboroCity_DevonCorp_1F_Text_OnlyAuthorizedPeopleEnter:
	.string "Desculpe, só pessoas autorizadas\n"
	.string "podem entrar aqui.$"

RustboroCity_DevonCorp_1F_Text_HowCouldWeGetRobbed:
	.string "Que vacilo absurdo.\n"
	.string "Como deixamos nos roubarem?$"

RustboroCity_DevonCorp_1F_Text_YoureAlwaysWelcomeHere:
	.string "Oi!\n"
	.string "Aqui a porta está sempre aberta!$"

RustboroCity_DevonCorp_1F_Text_RocksMetalDisplay:
	.string "Amostras de pedras e metais estão\n"
	.string "expostas na vitrine.\p"
	.string "Tem um painel com um texto…\p"
	.string "“A CORPORAÇÃO DEVON começou\n"
	.string "produzindo pedras de pedreiras.\p"
	.string "“A empresa também produzia ferro\n"
	.string "a partir da limalha da areia.\p"
	.string "“Desse começo humilde, produzindo\n"
	.string "matéria-prima, a DEVON cresceu.\p"
	.string "“Hoje, a DEVON fabrica uma grande\n"
	.string "variedade de produtos industriais.”$"

RustboroCity_DevonCorp_1F_Text_ProductDisplay:
	.string "Protótipos e produtos em teste\n"
	.string "enchem a vitrine.\p"
	.string "Tem um painel com uma descrição…\p"
	.string "“Além de produtos industriais, a DEVON\n"
	.string "hoje vende artigos diversos e\l"
	.string "remédios para uma vida melhor.\p"
	.string "“Há pouco, a DEVON começou a vender\n"
	.string "ferramentas para TREINADORES, como\l"
	.string "POKé BOLAS e o POKéNAV.”$"

