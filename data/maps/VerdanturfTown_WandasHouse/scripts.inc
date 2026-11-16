VerdanturfTown_WandasHouse_MapScripts::
	.byte 0

VerdanturfTown_WandasHouse_EventScript_Wally::
	lock
	faceplayer
	goto_if_set FLAG_WALLY_SPEECH, VerdanturfTown_WandasHouse_EventScript_WallyShortSpeech
	msgbox VerdanturfTown_WandasHouse_Text_StrongerSpeech, MSGBOX_DEFAULT
	setflag FLAG_WALLY_SPEECH
	release
	end

VerdanturfTown_WandasHouse_EventScript_WallyShortSpeech::
	msgbox VerdanturfTown_WandasHouse_Text_StrongerSpeechShort, MSGBOX_DEFAULT
	release
	end

VerdanturfTown_WandasHouse_EventScript_WallysUncle::
	lock
	faceplayer
	goto_if_set FLAG_DEFEATED_WALLY_VICTORY_ROAD, VerdanturfTown_WandasHouse_EventScript_WallysUncleEverGrande
	goto_if_set FLAG_DEFEATED_LAVARIDGE_GYM, VerdanturfTown_WandasHouse_EventScript_WallysUncleSlippedOff
	msgbox VerdanturfTown_WandasHouse_Text_WallysNextDoor, MSGBOX_DEFAULT
	release
	end

VerdanturfTown_WandasHouse_EventScript_WallysUncleSlippedOff::
	msgbox VerdanturfTown_WandasHouse_Text_WallySlippedOff, MSGBOX_DEFAULT
	release
	end

VerdanturfTown_WandasHouse_EventScript_WallysUncleEverGrande::
	msgbox VerdanturfTown_WandasHouse_Text_WallyGoneThatFar, MSGBOX_DEFAULT
	release
	end

VerdanturfTown_WandasHouse_EventScript_WandasBoyfriend::
	msgbox VerdanturfTown_WandasHouse_Text_CanSeeGirlfriendEveryDay, MSGBOX_NPC
	end

VerdanturfTown_WandasHouse_EventScript_Wanda::
	lock
	faceplayer
	goto_if_set FLAG_DEFEATED_LAVARIDGE_GYM, VerdanturfTown_WandasHouse_EventScript_WandaDontWorry
	goto_if_set FLAG_DEFEATED_WALLY_MAUVILLE, VerdanturfTown_WandasHouse_EventScript_MeetWanda
	msgbox VerdanturfTown_WandasHouse_Text_DontWorryAboutWally, MSGBOX_DEFAULT
	release
	end

VerdanturfTown_WandasHouse_EventScript_MeetWanda::
	msgbox VerdanturfTown_WandasHouse_Text_MeetWanda, MSGBOX_DEFAULT
	release
	end

VerdanturfTown_WandasHouse_EventScript_WandaDontWorry::
	msgbox VerdanturfTown_WandasHouse_Text_DontWorryAboutWally, MSGBOX_DEFAULT
	release
	end

VerdanturfTown_WandasHouse_EventScript_WallysAunt::
	lock
	faceplayer
	goto_if_set FLAG_DEFEATED_WALLY_VICTORY_ROAD, VerdanturfTown_WandasHouse_EventScript_WallysAuntEverGrande
	goto_if_set FLAG_DEFEATED_LAVARIDGE_GYM, VerdanturfTown_WandasHouse_EventScript_WallysAuntAnythingHappened
	goto_if_set FLAG_RUSTURF_TUNNEL_OPENED, VerdanturfTown_WandasHouse_EventScript_WallysAuntTunnelOpen
	msgbox VerdanturfTown_WandasHouse_Text_DaughtersBoyfriendDriven, MSGBOX_DEFAULT
	release
	end

VerdanturfTown_WandasHouse_EventScript_WallysAuntTunnelOpen::
	msgbox VerdanturfTown_WandasHouse_Text_DaughtersBoyfriendWasDigging, MSGBOX_DEFAULT
	release
	end

VerdanturfTown_WandasHouse_EventScript_WallysAuntAnythingHappened::
	msgbox VerdanturfTown_WandasHouse_Text_IfAnythingHappenedToWally, MSGBOX_DEFAULT
	release
	end

VerdanturfTown_WandasHouse_EventScript_WallysAuntEverGrande::
	msgbox VerdanturfTown_WandasHouse_Text_WallyWasInEverGrande, MSGBOX_DEFAULT
	release
	end

VerdanturfTown_WandasHouse_Text_StrongerSpeech:
	.string "WALLY: Perdi pra você, {PLAYER}, mas não\n"
	.string "estou mais pra baixo.\p"
	.string "Porque agora tenho um novo objetivo.\n"
	.string "Junto com o meu RALTS, vou desafiar\l"
	.string "os GINÁSIOS POKéMON e virar um\l"
	.string "grande TREINADOR.\p"
	.string "Fica de olho em mim, {PLAYER}.\n"
	.string "Vou ficar mais forte que você.\p"
	.string "Quando isso acontecer, vou te\n"
	.string "desafiar pra outra batalha.$"

VerdanturfTown_WandasHouse_Text_StrongerSpeechShort:
	.string "WALLY: Fica de olho em mim, {PLAYER}.\n"
	.string "Vou ficar mais forte que você.\p"
	.string "Quando isso acontecer, vou te\n"
	.string "desafiar pra outra batalha.$"

VerdanturfTown_WandasHouse_Text_WallysNextDoor:
	.string "TIO: Ah! {PLAYER}{KUN}!\n"
	.string "O WALLY está na casa ao lado.\p"
	.string "Mas, olha, tenho uma coisa pra\n"
	.string "te contar.\p"
	.string "A natureza daqui está fazendo\n"
	.string "maravilhas pela saúde do WALLY.\p"
	.string "Talvez não seja só a natureza.\n"
	.string "Podem ser os POKéMON que estão dando\l"
	.string "esperança ao menino.$"

VerdanturfTown_WandasHouse_Text_WallySlippedOff:
	.string "O WALLY foi embora…\n"
	.string "Saiu escondido, sozinho…$"

VerdanturfTown_WandasHouse_Text_WallyGoneThatFar:
	.string "TIO: É mesmo?\n"
	.string "O WALLY foi tão longe assim\l"
	.string "sozinho…\p"
	.string "Bom, tenho que dar o braço a torcer:\n"
	.string "ele é filho do meu irmão caçula.$"

VerdanturfTown_WandasHouse_Text_MeetWanda:
	.string "WANDA: Quem é você?\n"
	.string "Ah, claro, já sei!\p"
	.string "Você é {PLAYER}, de quem o WALLY\n"
	.string "me falou.\p"
	.string "Eu sou prima do WALLY.\n"
	.string "Prazer em te conhecer!\p"
	.string "Acho que o WALLY ficou muito mais\n"
	.string "animado e saudável desde que\l"
	.string "veio pra cá.$"

VerdanturfTown_WandasHouse_Text_DontWorryAboutWally:
	.string "WANDA: Não se preocupe com o WALLY.\n"
	.string "Ele vai ficar bem.\p"
	.string "Eu conheço o meu priminho, e ele\n"
	.string "também tem POKéMON com ele.$"

VerdanturfTown_WandasHouse_Text_CanSeeGirlfriendEveryDay:
	.string "Graças a você, posso ver a minha\n"
	.string "namorada todo dia.\l"
	.string "Se estou feliz? Pode apostar!$"

VerdanturfTown_WandasHouse_Text_DaughtersBoyfriendDriven:
	.string "O namorado da minha filha é uma pessoa\n"
	.string "muito determinada e apaixonada.\p"
	.string "Ele está cavando um túnel sem parar\n"
	.string "só pra poder ver a minha filha.\p"
	.string "A minha filha está meio preocupada,\n"
	.string "então vai muito até o túnel.$"

VerdanturfTown_WandasHouse_Text_DaughtersBoyfriendWasDigging:
	.string "É impressionante. O namorado da minha\n"
	.string "filha cavava o túnel com as mãos!\p"
	.string "É inacreditável!$"

VerdanturfTown_WandasHouse_Text_IfAnythingHappenedToWally:
	.string "Se acontecesse alguma coisa com o\n"
	.string "WALLY, eu nunca mais ia conseguir\l"
	.string "olhar nos olhos dos pais dele,\l"
	.string "em PETALBURG…$"

VerdanturfTown_WandasHouse_Text_WallyWasInEverGrande:
	.string "O WALLY estava em EVER GRANDE?\p"
	.string "Os pais dele, em PETALBURG, iam ficar\n"
	.string "boquiabertos com isso!$"

