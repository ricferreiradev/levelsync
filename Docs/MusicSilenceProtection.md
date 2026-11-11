# Music Silence Protection

Music Silence Protection impede que uma correção vocal antiga permaneça aplicada durante uma parada do instrumental.

## Medição

O detector usa energia não ponderada do sidechain estéreo, separada do medidor K-weighted usado pelo controlador de balanceamento. A escala é:

`dBFS = 10 * log10((L² + R²) / 2)`

Assim, o threshold rotulado em dBFS nunca é comparado diretamente ao valor K-weighted ou tratado como LUFS. Um envelope energético de 5 ms acompanha as transições. O estado muda para silêncio somente depois de `Silence Hold`; para voltar ao estado ativo, o sinal deve superar o threshold em 3 dB durante `Silence Release`.

## Comportamento

- Enquanto o silêncio ainda está no período de hold, o último target válido é preservado e nenhum novo target é calculado a partir do loudness decadente.
- Depois da confirmação, o target passa a 0 dB e chega a 99,9% de unity dentro de `Return to Unity`.
- O retorno independe da presença de voz e usa suavização por amostra.
- O Learn Balance pausa enquanto o instrumental não é válido, sem substituir a referência anterior.
- A correção perceptual é desativada e suavemente descarregada durante a proteção.
- Sidechain desconectado continua sendo tratado separadamente e não é apresentado como proteção ativa.

Valores padrão: threshold −55 dBFS, hold 200 ms, release 400 ms e retorno 300 ms.
