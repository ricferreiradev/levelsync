# Relatório de validação — LevelSync 0.4.0

Data: 03/11/2026. Projeto validado: `C:\projetos\LevelSync`. JUCE local: 9.0.5 em `C:\Dev\JUCE`. Compilador: MSVC 19.52.37025.0, Release x64.

## Aprovados

- Configuração CMake com JUCE via `add_subdirectory()`.
- Build dos alvos `LevelSync_VST3`, `LevelSyncDSPTests`, `LevelSyncProcessorTests` e `LevelSyncOfflineMetrics`.
- Manifesto VST3 gerado para LevelSync 0.4.0 / VST SDK 3.8.0.
- CTest: 3 de 3 executáveis aprovados; 27 cenários nomeados, zero falhas, 12,47 s.
- DSP: 44,1/48/96 kHz, blocos variados, NaN/Inf, silêncio, sidechain ausente/desconectado, Learn, limites, hold, retorno neutro, execução prolongada e ausência de alocações no caminho preparado.
- Fila SPSC: capacidade fixa, FIFO, overflow não bloqueante e nenhuma alocação nas operações.
- Processador: layouts mono/estéreo, sidechain somente leitura, estado versionado, automação, Enable/bypass, GUI fechada, buffer vazio e abertura/redimensionamento/fechamento repetidos do editor.
- Interface renderizada em 1180×820 e inspecionada visualmente; nenhum controle cortado na captura padrão.

## Métricas sintéticas medidas

O cenário e as definições estão em `PerformanceMethodology.md`.

| Métrica | Resultado |
| --- | ---: |
| Mean Absolute Error estável | 0,0105566 dB |
| Maximum Gain | +6,0000 dB |
| Minimum Gain | −1,9485 dB |
| Gain Movement acumulado | 7,9485 dB |
| Peak Gain Speed | 7,47678 dB/s |
| Convergence Time | 2,4 s |
| Frames estáveis válidos | 150 |

Esses números pertencem ao sinal determinístico do teste e servem como referência de regressão. Não medem desempenho geral em fala e música reais.

## Reprovados

Nenhum teste automatizado na execução final.

## Não executados neste ambiente

- Descoberta e carregamento pelo scanner do Cakewalk Sonar.
- Exposição e roteamento real do sidechain no Sonar.
- Automação gravada e reproduzida pelo host.
- Comparação entre playback e renderização offline no host.
- Restauração depois de fechar e reabrir uma sessão real do Sonar.
- Comparação com Waves Vocal Rider em material musical.
- Avaliação auditiva, clipping intersample e uso de CPU em uma sessão real.

## Limitações conhecidas

- Loudness é uma estimativa K suavizada, não medidor LUFS normativo certificado.
- Detecção vocal por energia pode responder a vazamento, ruído e outros sons.
- Não há limiter ou proteção true-peak; boost requer headroom posterior.
- A automação é lida uma vez por bloco, sem precisão por amostra.
- O histórico visual existe enquanto o editor está aberto. Se a GUI não consumir a fila cheia, novos frames são descartados sem afetar o DSP.
- A telemetria e as métricas são observacionais; não participam do controle de áudio.
