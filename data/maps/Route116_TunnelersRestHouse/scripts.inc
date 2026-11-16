Route116_TunnelersRestHouse_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, Route116_TunnelersRestHouse_OnTransition
	.byte 0

Route116_TunnelersRestHouse_OnTransition:
	setflag FLAG_LANDMARK_TUNNELERS_REST_HOUSE
	end

Route116_TunnelersRestHouse_EventScript_Tunneler1::
	msgbox Route116_TunnelersRestHouse_Text_WeHadToStopBoring, MSGBOX_NPC
	end

Route116_TunnelersRestHouse_EventScript_Tunneler2::
	msgbox Route116_TunnelersRestHouse_Text_ManDiggingHisWayToVerdanturf, MSGBOX_NPC
	end

Route116_TunnelersRestHouse_EventScript_Tunneler3::
	lock
	faceplayer
	goto_if_set FLAG_RUSTURF_TUNNEL_OPENED, Route116_TunnelersRestHouse_EventScript_TunnelOpened
	msgbox Route116_TunnelersRestHouse_Text_GetToVerdanturfWithoutTunnel, MSGBOX_DEFAULT
	release
	end

Route116_TunnelersRestHouse_EventScript_TunnelOpened::
	msgbox Route116_TunnelersRestHouse_Text_TunnelHasGoneThrough, MSGBOX_DEFAULT
	release
	end

Route116_TunnelersRestHouse_Text_WeHadToStopBoring:
	.string "Aquele TÚNEL RUSTURF ali…\p"
	.string "No começo, tinha uma equipe enorme\n"
	.string "furando a rocha com máquinas de ponta.\l"
	.string "Mas tivemos que parar.\p"
	.string "Descobrimos que a obra ia prejudicar\n"
	.string "os POKéMON selvagens da\l"
	.string "região.\p"
	.string "Então, não nos resta nada além de\n"
	.string "ficar à toa por aqui.$"

Route116_TunnelersRestHouse_Text_ManDiggingHisWayToVerdanturf:
	.string "Tem um homem cavando até VERDANTURF\n"
	.string "sozinho. Ele está louco pra\l"
	.string "conseguir atravessar.\p"
	.string "Ele diz que, cavando aos pouquinhos\n"
	.string "sem máquinas, não incomoda os\l"
	.string "POKéMON e não prejudica\l"
	.string "a natureza.\p"
	.string "Será que ele já conseguiu passar?$"

Route116_TunnelersRestHouse_Text_GetToVerdanturfWithoutTunnel:
	.string "Pra chegar a VERDANTURF sem usar\n"
	.string "este TÚNEL, você teria que cruzar o\l"
	.string "mar até DEWFORD, navegar até\l"
	.string "SLATEPORT e passar por MAUVILLE.$"

Route116_TunnelersRestHouse_Text_TunnelHasGoneThrough:
	.string "Ficou sabendo? O TÚNEL até VERDANTURF\n"
	.string "foi aberto!\p"
	.string "Às vezes, se você deseja com força,\n"
	.string "os sonhos se realizam.$"

