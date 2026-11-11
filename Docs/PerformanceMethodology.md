# Metodologia de métricas offline

O executável `LevelSyncOfflineMetrics` processa sinais sintéticos determinísticos em 48 kHz. Uma voz senoidal de 997 Hz permanece ativa enquanto o instrumental sofre uma mudança sustentada no oitavo segundo. O motor recebe áudio amostra a amostra e produz um frame de análise a cada 40 ms, equivalente à telemetria de 25 Hz da interface.

As métricas usam somente frames com medição vocal válida, sidechain válido e atividade vocal. O erro é:

```text
balanceErrorDb = targetBalanceDb - ((voicePreDb + appliedGainDb) - musicDb)
```

- **Mean Absolute Error:** média de `abs(balanceErrorDb)` apenas na região estável, iniciada seis segundos após a mudança.
- **Maximum/Minimum Gain:** extremos do ganho efetivamente aplicado nos frames válidos após a mudança.
- **Gain Movement:** soma das diferenças absolutas entre ganhos consecutivos.
- **Peak Gain Speed:** maior diferença entre frames dividida por 0,04 s.
- **Convergence Time:** depois que a mudança faz o erro sair de ±0,5 dB, tempo até cinco frames consecutivos voltarem a essa faixa. Isso impede que amostras ainda dominadas pela janela anterior sejam contadas como convergência.

O teste imprime os valores medidos em cada execução e verifica apenas propriedades objetivas: finitude, número suficiente de frames, movimento não nulo, convergência observada e respeito aos limites de ±6 dB. Esses sinais sintéticos não representam toda a variação de fala e música; os números servem como regressão, não como alegação de desempenho em material real.
