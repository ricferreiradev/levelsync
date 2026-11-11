# Protocolo de comparação: LevelSync e Waves Vocal Rider

## Objetivo

Comparar estabilidade, erro de balanço, movimento de ganho e comportamento transitório em condições reproduzíveis. Os plugins podem usar detectores, janelas e estratégias de controle diferentes; o protocolo mede resultados e não presume equivalência interna nem superioridade.

## Preparação da sessão na DAW

1. Use uma cópia da sessão e desative normalização automática, outros riders, compressores e automações de volume que afetem o teste.
2. Escolha uma faixa vocal e um stem instrumental sem alterações entre as comparações. Anote sample rate, buffer, pan law e níveis dos faders.
3. Crie duas cópias idênticas da rota vocal, uma para cada plugin. Mantenha o mesmo ponto de inserção, antes ou depois dos mesmos processos, e envie ambas ao mesmo bus de monitoração. Nunca reproduza as duas simultaneamente para a avaliação auditiva.
4. Alimente o sidechain do LevelSync pelo mesmo stem instrumental usado na reprodução. Configure o Vocal Rider conforme sua documentação para receber condições equivalentes. Caso os roteamentos não sejam tecnicamente idênticos, registre a diferença.
5. Iguale o ganho inicial com os plugins em estado neutro. Evite compensar resultados posteriormente com faders diferentes.

## Regiões de teste

Marque previamente regiões contendo:

- voz sustentada com instrumental estável;
- mudança sustentada de ±6 dB no instrumental;
- palavras curtas e transientes consonantais;
- pausa curta entre frases;
- silêncio vocal prolongado;
- vazamento ou ruído de fundo;
- trecho vocal baixo e trecho forte;
- entrada e retirada do sidechain, apenas para o LevelSync.

Use as mesmas fronteiras de tempo para todos os renders. Inclua pelo menos cinco segundos estáveis depois de cada mudança para separar transiente e regime estável.

## Configuração dos plugins

No LevelSync, faça Learn Balance no equilíbrio manual escolhido e registre Target Balance, Strength, Attack, Release, Deadband, Vocal Hold, Silence Return, Max Boost e Max Cut. No Vocal Rider, registre todos os controles usados. Iguale a faixa máxima de movimento sempre que os controles permitirem, sem afirmar que valores com nomes semelhantes têm a mesma resposta.

## Renderização

1. Renderize cada rota offline para WAV float32, com a mesma duração, sample rate e ponto de início.
2. Renderize também a voz sem rider e o instrumental de referência.
3. Nomeie os arquivos sem revelar o plugin para uma avaliação auditiva cega, mantendo uma tabela privada de correspondência.
4. Confirme alinhamento de amostras. O LevelSync declara latência zero; verifique se o host aplica alguma compensação ao outro plugin.

## Métricas

Calcule loudness com o mesmo detector e janelas em todos os arquivos. Em regiões com voz ativa e instrumental válido, compare:

- erro de balanço: alvo menos a diferença entre voz pós-processamento e instrumental;
- erro absoluto médio em regiões estáveis;
- ganho máximo e mínimo;
- movimento acumulado e pico de velocidade do ganho;
- tempo para entrar e permanecer dentro de ±0,5 dB do alvo após mudança sustentada;
- overshoot e quantidade de alternâncias ao redor do alvo;
- nível de ruído durante pausas vocais.

Relate regiões transitórias separadamente das estáveis. Uma média global pode esconder pumping, reação lenta ou amplificação de ruído.

## Avaliação auditiva

Faça comparação cega com loudness de saída igualado, sem clipping. Avalie inteligibilidade, naturalidade, preservação de dinâmica, pumping, reação a consoantes, respiração e ruído entre frases. Registre observações por região antes de revelar os plugins.

## Relatório

Inclua versão de cada plugin, nome e versão da DAW, hardware, sample rate, buffer, renderização online/offline, parâmetros, roteamento, arquivos analisados e scripts/ferramentas usados. Resultados só valem para o material e as configurações testadas. Não generalize uma vitória sem repetir o protocolo em materiais diversos.
