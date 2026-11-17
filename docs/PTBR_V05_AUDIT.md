# Auditoria de localização PT-BR — integração v0.5

## Cobertura

- Catálogos de texto do jogo: 14.253 entradas em 405 arquivos, verificados
  contra os arquivos ingleses originais pelo `tools/localize_portuguese.py --check`.
- Dicionário da UI nativa: 716 entradas antes da revisão, **958** após a revisão.
- Foram acrescentadas **242** traduções para a UI v0.5, especialmente o tema
  GBA, Pokédex, equipe, mochila, PC, PokéNav, insígnias e novas opções HD-2D.
- **56** mensagens construídas com `snprintf` passaram a consultar o idioma,
  incluindo números, destinos e nomes de Pokémon.
- Medidas de altura/peso do Pokédex: PT-BR em metros e quilogramas;
  inglês mantém pés e libras.
- Pergunta de batalha GBA composta com o nome do Pokémon: agora respeita o idioma.
- Entradas do dicionário ordenadas para busca binária; formatos de texto
  conferidos para preservar quantidade e tipos de argumentos.

## Textos preservados / intencionais

- Nomes próprios de líderes de ginásio e localidades (Roxanne, Brawly,
  Rustboro, Dewford, etc.) continuam como no jogo original.
- Termos técnicos universais como STAB, nome de arquivo e caminhos de
  armazenamento, `AUTO` e símbolos de controle permanecem intencionalmente.
- Mensagens de diagnóstico do atlas (`[UI2]`) são logs de desenvolvedor,
  não elementos da interface jogável.
- Textos em partes, como ` POKéMON`, são complemento de nomes e não
  precisam ser traduzidos isoladamente.

## Validação

- A integração e o PT-BR foram compilados em Docker VitaSDK.
- O atlas gerado comportou 3.993 glifos em 33 fontes (altura fixa 1.144 px).
- O pacote `followers.pak` contém 424 entradas.
- **Não foi feita verificação visual completa em todas as telas de um PS Vita
  físico.** A análise estática pode deixar passar textos construídos
  dinamicamente em condições raras ou problemas de espaço na UI.
