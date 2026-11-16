VerdanturfTown_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, VerdanturfTown_OnTransition
	.byte 0

VerdanturfTown_OnTransition:
	setflag FLAG_VISITED_VERDANTURF_TOWN
	setvar VAR_CONTEST_HALL_STATE, 0
	end

VerdanturfTown_EventScript_Twin::
	lock
	faceplayer
	goto_if_set FLAG_RUSTURF_TUNNEL_OPENED, VerdanturfTown_EventScript_TwinTunnelOpen
	msgbox VerdanturfTown_Text_ManTryingToDigTunnel, MSGBOX_DEFAULT
	applymovement LOCALID_VERDANTURF_TWIN, Common_Movement_FaceOriginalDirection
	waitmovement 0
	release
	end

VerdanturfTown_EventScript_TwinTunnelOpen::
	msgbox VerdanturfTown_Text_ManDugTunnelForLove, MSGBOX_DEFAULT
	applymovement LOCALID_VERDANTURF_TWIN, Common_Movement_FaceOriginalDirection
	waitmovement 0
	release
	end

VerdanturfTown_EventScript_Man::
	msgbox VerdanturfTown_Text_AirCleanHere, MSGBOX_NPC
	end

VerdanturfTown_EventScript_Camper::
	msgbox VerdanturfTown_Text_MakeBattleTentDebut, MSGBOX_NPC
	end

VerdanturfTown_EventScript_Boy::
	lock
	faceplayer
	goto_if_set FLAG_RUSTURF_TUNNEL_OPENED, VerdanturfTown_EventScript_BoyTunnelOpen
	msgbox VerdanturfTown_Text_GuyTryingToBustThroughCave, MSGBOX_DEFAULT
	release
	end

VerdanturfTown_EventScript_BoyTunnelOpen::
	msgbox VerdanturfTown_Text_EasyToGetToRustboroNow, MSGBOX_DEFAULT
	release
	end

VerdanturfTown_EventScript_TownSign::
	msgbox VerdanturfTown_Text_TownSign, MSGBOX_SIGN
	end

VerdanturfTown_EventScript_WandasHouseSign::
	msgbox VerdanturfTown_Text_WandasHouse, MSGBOX_SIGN
	end

VerdanturfTown_EventScript_BattleTentSign::
	msgbox VerdanturfTown_Text_BattleTentSign, MSGBOX_SIGN
	end

VerdanturfTown_EventScript_RusturfTunnelSign::
	msgbox VerdanturfTown_Text_RusturfTunnelSign, MSGBOX_SIGN
	end

VerdanturfTown_Text_ManTryingToDigTunnel:
	.string "O meu pai me contou.\p"
	.string "Ele diz que este túnel é cheio de\n"
	.string "POKéMON medrosos.\p"
	.string "Eles se assustam com barulho alto e\n"
	.string "fazem um tumulto enorme.\p"
	.string "Por isso tiveram que parar a obra\n"
	.string "do túnel.\p"
	.string "Mas tem um homem que está tentando\n"
	.string "cavar o túnel sozinho!$"

VerdanturfTown_Text_ManDugTunnelForLove:
	.string "Teve um homem que cavou um túnel por\n"
	.string "uma moça que ele amava.\p"
	.string "Eu não entendo muito bem, mas, enfim!$"

VerdanturfTown_Text_AirCleanHere:
	.string "Pelo jeito que o vento sopra, a cinza\n"
	.string "do vulcão nunca vem pra cá.\p"
	.string "O ar aqui é limpo e gostoso.\n"
	.string "Morar aqui faz bem até pra gente\l"
	.string "frágil e doente.$"

VerdanturfTown_Text_MakeBattleTentDebut:
	.string "Eu e os meus POKéMON estamos numa\n"
	.string "sequência de vitórias quente.\p"
	.string "Então resolvi estrear na TENDA\n"
	.string "DE BATALHA nesta cidade.$"

VerdanturfTown_Text_GuyTryingToBustThroughCave:
	.string "Viu a caverna do lado do\n"
	.string "POKé MART?\p"
	.string "Tem um cara lá dentro tentando\n"
	.string "quebrar as pedras pra conseguir\l"
	.string "atravessar pro outro lado.\p"
	.string "Seria ótimo se desse pra passar…\n"
	.string "Ia ficar fácil visitar RUSTBORO.$"

VerdanturfTown_Text_EasyToGetToRustboroNow:
	.string "A caverna do lado do POKé MART\n"
	.string "agora é um túnel pro outro lado.\p"
	.string "É ótimo: agora é fácil ir a RUSTBORO\n"
	.string "comprar os produtos novos da DEVON.$"

VerdanturfTown_Text_TownSign:
	.string "CIDADE DE VERDANTURF\p"
	.string "“Planalto ventoso com o\n"
	.string "cheiro doce da grama.”$"

VerdanturfTown_Text_WandasHouse:
	.string "CASA DA WANDA$"

VerdanturfTown_Text_BattleTentSign:
	.string "TENDA DE BATALHA DE VERDANTURF\n"
	.string "“Encha os olhos de batalhas!”$"

VerdanturfTown_Text_RusturfTunnelSign:
	.string "TÚNEL RUSTURF\n"
	.string "“Ligando RUSTBORO e VERDANTURF\p"
	.string "“A obra do túnel foi\n"
	.string "cancelada.”$"

