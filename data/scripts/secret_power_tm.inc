Route111_Text_MakingRoomUseTMToMakeYourOwn:
	.string "O quê?\n"
	.string "O que eu tô fazendo?\p"
	.string "Tô pensando em fazer meu próprio\n"
	.string "cantinho aqui com um golpe POKéMON.\p"
	.string "Já sei! Vou te dar este MT.\n"
	.string "Vai usar pra fazer o seu cantinho?$"

Route111_Text_ExplainSecretPower:
	.string "Procure uma árvore grande que pareça\n"
	.string "ter uns cipós pra soltar.\p"
	.string "Use PODER SECRETO na frente dela.\n"
	.string "Alguns cipós vão se soltar e cair,\l"
	.string "e aí dá pra subir na árvore.\p"
	.string "Lá tem muito espaço pra colocar\n"
	.string "as suas coisas favoritas.\p"
	.string "Vai ser o seu cantinho secreto…\n"
	.string "Uma BASE SECRETA!\p"
	.string "Você devia fazer uma também.\p"
	.string "Outra coisa: a BASE SECRETA\n"
	.string "não precisa ser numa árvore.\p"
	.string "Tente usar PODER SECRETO em paredes\n"
	.string "de pedra com pequenas fendas.\p"
	.string "Vou procurar outros lugares também.\n"
	.string "Tchau!$"

Route111_Text_DontWantThis:
	.string "Ah, você não quer?\n"
	.string "Se mudar de ideia, me fala, tá?$"

Route111_Text_DontHaveAnyRoom:
	.string "Ah, você não tem espaço pra isto.\p"
	.string "Eu guardo pra você, então\n"
	.string "volta outra hora pra buscar, tá?$"

Route111_EventScript_SecretPowerMan::
	lock
	faceplayer
	msgbox Route111_Text_MakingRoomUseTMToMakeYourOwn, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, Route111_EventScript_GiveSecretPower
	msgbox Route111_Text_DontWantThis, MSGBOX_DEFAULT
	release
	end

Route111_EventScript_GiveSecretPower::
	giveitem ITEM_TM_SECRET_POWER
	goto_if_eq VAR_RESULT, FALSE, Route111_EventScript_NoRoomForSecretPower
	msgbox Route111_Text_ExplainSecretPower, MSGBOX_DEFAULT
	closemessage
	setflag FLAG_RECEIVED_SECRET_POWER
	clearflag FLAG_HIDE_SLATEPORT_CITY_TM_SALESMAN
	call_if_eq VAR_FACING, DIR_WEST, Route111_EventScript_SecretPowerManExit
	call_if_eq VAR_FACING, DIR_EAST, Route111_EventScript_SecretPowerManExit
	call_if_eq VAR_FACING, DIR_NORTH, Route111_EventScript_SecretPowerManExitNorth
	removeobject VAR_LAST_TALKED
	release
	end

Route111_EventScript_SecretPowerManExit::
	applymovement VAR_LAST_TALKED, Route111_Movement_SecretPowerManExit
	waitmovement 0
	return

Route111_EventScript_SecretPowerManExitNorth::
	applymovement VAR_LAST_TALKED, Route111_Movement_SecretPowerManExitNorth
	waitmovement 0
	return

Route111_EventScript_NoRoomForSecretPower::
	msgbox Route111_Text_DontHaveAnyRoom, MSGBOX_DEFAULT
	release
	end

Route111_Movement_SecretPowerManExit:
	walk_down
	walk_down
	walk_down
	walk_down
	walk_down
	walk_left
	walk_left
	walk_left
	walk_left
	walk_down
	walk_down
	step_end

Route111_Movement_SecretPowerManExitNorth:
	walk_left
	walk_down
	walk_down
	walk_down
	walk_down
	walk_down
	walk_left
	walk_left
	walk_left
	walk_down
	walk_down
	step_end
