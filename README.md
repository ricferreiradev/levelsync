# LevelSync — etapa 4

Efeito VST3 para Windows x64, C++17, JUCE local e CMake. O passthrough original foi analisado e compilado em Release x64 antes das alterações. A identidade do plugin e os layouts foram preservados: voz mono/estéreo, sidechain estéreo ou desativado e saída com o mesmo layout da voz.

## Compilar e testar

Requisitos: CMake 3.22+, Visual Studio com ferramentas C++ e Windows SDK, JUCE em `C:\Dev\JUCE`.

```powershell
cd C:\projetos\LevelSync
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DBUILD_TESTING=ON
cmake --build build --config Release --target LevelSync_VST3 LevelSyncDSPTests LevelSyncProcessorTests LevelSyncOfflineMetrics --parallel 4
ctest --test-dir build -C Release --output-on-failure
```

Para Visual Studio 2022, use `Visual Studio 17 2022` e outra pasta de build, como `build-vs2022`. Para outro local do JUCE, acrescente `-DJUCE_DIR="C:/outro/caminho/JUCE"`. A integração permanece via `add_subdirectory()`, sem baixar o framework. `-DBUILD_TESTING=OFF` desativa os testes.

Bundle: `build/LevelSync_artefacts/Release/VST3/LevelSync.vst3`.
Binário interno: `Contents/x86_64-win/LevelSync.vst3`.

## Arquivos e arquitetura

Modificados: `CMakeLists.txt`, `Source/PluginProcessor.h/.cpp` e `README.md`.

Criados:

- `Source/DSP/LoudnessDetector.h`: K-weighting independente por canal, soma das energias, janela circular de 400 ms e suavização de energia com constante de 100 ms. Filtros de referência de 48 kHz remapeados por transformação bilinear para a taxa utilizada. Aquecimento de 400 ms, piso de confiabilidade de -70 dB K e presença de sinal com envelope de 10 ms.
- `Source/DSP/BalanceController.h`: cálculo puro de ganho, deadband contínuo, limites, Strength e validação.
- `Source/DSP/GainSmoother.h`: interpolação exponencial em dB por amostra. **Attack diminui o ganho; Release aumenta o ganho.** Os tempos são constantes de primeira ordem: 63,2% de um degrau no tempo indicado, aproximadamente 95% em três constantes de tempo.
- `Source/DSP/VocalActivityDetector.h`: energia média dos canais com envelope de 10 ms; threshold inicial -50 dBFS, histerese de 6 dB, atividade mínima de 80 ms e retenção de 250 ms. A retenção mantém a indicação entre frases; abaixo do limiar de saída não são autorizadas correções, mesmo durante a retenção.
- `Source/DSP/BalanceEngine.h`: composição do DSP, controle a cada 10 ms independente dos blocos, medição antes do ganho e mesmo ganho nos canais principais.
- `Source/DSP/ReferenceLearner.h`: máquina de estados do Learn Balance e histograma fixo para mediana ponderada pelo tempo válido.
- `Source/DSP/TelemetryQueue.h`: fila SPSC fixa; o produtor de áudio nunca bloqueia e descarta frames quando a GUI não consome.
- `Source/DSP/PerformanceMetrics.h`: acumulador offline de erro, extremos, movimento, velocidade e convergência.
- `Source/UI/LevelSyncLookAndFeel.h` e `Source/UI/MeterComponents.*`: tema vetorial, medidores e gráfico circular de 30 segundos.
- `Source/PluginEditor.h/.cpp`: interface redimensionável, Basic/Advanced, Learn, diagnóstico opcional e atualização a 25 Hz.
- `Tests/TestSupport.h`, `Tests/DSPTests.cpp`, `Tests/ProcessorTests.cpp`: regras matemáticas, sinais sintéticos, integração dos buses, estado e renderização da interface.

O DSP é C++17 independente de JUCE. Apenas `prepare()` aloca as janelas; o processamento não aloca, não usa mutex e não acessa ValueTrees. O processador usa ponteiros de parâmetros atômicos armazenados na construção e publica medidores atômicos para a interface. Os medidores são leituras independentes para exibição, sem garantia de fotografia transacional entre campos.

O instrumental é somente leitura e nunca é copiado para a saída. Medir a voz antes do ganho evita realimentação. Não há latência adicionada ao áudio; a reação tem o atraso natural da análise e da suavização.

## Fórmula

```text
erro = loudnessInstrumental + referenciaAtual - loudnessVoz
correcao = sign(erro) * max(abs(erro) - Deadband, 0)
ganhoAlvoDb = Strength/100 * clamp(correcao, -MaxCut, +MaxBoost)
ganhoAplicadoDb[n] = alvo + coeficiente * (ganhoAplicadoDb[n-1] - alvo)
vozSaida[n] = vozEntrada[n] * 10^(ganhoAplicadoDb[n]/20)
```

O deadband contínuo e a histerese do detector vocal evitam alternâncias ao redor dos limiares. A relação final pode permanecer dentro dessa tolerância. Strength é aplicado depois da limitação. Durante pausas curtas, o alvo anterior é retido; após expirar a retenção vocal de 250 ms, o alvo volta a 0 dB. Sem referência válida ou com Enable desligado, o alvo também é neutro. Todas as transições usam o mesmo suavizador.

Um bus habilitado mas silencioso também não é uma referência válida. Não é possível distinguir desconexão de silêncio enviado pelo host. Retirar o bus auxiliar desativa a correção e inicia retorno suave a unidade; reconectar exige uma janela nova de 400 ms. Silêncio/invalidade em bus habilitado é detectado pelos envelopes e pela janela, com controle atualizado em até 10 ms. NaN/Inf da voz viram zero; invalidez impede controle por uma janela. O sidechain permanece intocado, inclusive com dados inválidos.

## Parâmetros automatizáveis

| Parâmetro | Faixa | Padrão |
| --- | --- | --- |
| Balance Offset | -12 a +12 dB | +3 dB |
| Attack | 50 a 1500 ms | 250 ms |
| Release | 100 a 3000 ms | 700 ms |
| Max Boost | 0 a 12 dB | 6 dB |
| Max Cut | 0 a 12 dB | 6 dB |
| Strength | 0 a 100% | 100% |
| Deadband | 0 a 2 dB | 0,5 dB |
| Vocal Threshold | -80 a -20 dBFS | -50 dBFS |
| Vocal Hold | 0 a 2000 ms | 250 ms |
| Silence Return | 100 a 5000 ms | 1500 ms |
| Enable | desligado/ligado | ligado |

Vocal Threshold permite ajustar a detecção à gravação. Histerese, atividade mínima e retenção são configuráveis na classe DSP. Todos os parâmetros da tabela são salvos/restaurados pelo APVTS.

## Learn Balance

Ao iniciar, a máquina passa por `Preparing` e força alvo de 0 dB usando Release. São exigidos 100 ms consecutivos dentro de ±0,01 dB antes de entrar em `Learning`; nenhuma medição de preparação é coletada. Em `Learning`, uma observação sincronizada de `vozDb - instrumentalDb` é considerada a cada tick de 10 ms apenas com voz e sidechain confiáveis. Silêncio e dados inválidos pausam o progresso, que representa 5 s de áudio válido e não tempo de parede.

As diferenças válidas entre −12 e +12 dB alimentam um histograma fixo de 0,1 dB. A referência é a mediana ponderada pela duração. Essa escolha rejeita valores extremos e mudanças breves sem calcular média aritmética de dB, ordenar vetores ou alocar na thread de áudio. A sessão falha após 30 s sem completar os 5 s válidos. `Cancelled` e `Failed` preservam a referência anterior. `Completed` retoma o controle suavemente e envia o resultado à thread de mensagens.

O resultado atualiza Balance Offset, que continua automatizável e pode ser ajustado manualmente após a calibração. O valor originalmente aprendido e a existência de calibração válida são mantidos separadamente. O estado serializado possui `stateVersion = 5` e inclui APVTS, referências aprendidas e flags de validade. A thread de áudio publica apenas dados atômicos; a atualização do parâmetro e a serialização acontecem fora de `processBlock()`.

## Music Silence Protection

A proteção de silêncio musical evita que uma correção anterior deixe a voz baixa durante paradas do instrumental. Ela mede energia estéreo não ponderada em dBFS, aplica hold e histerese e retorna suavemente o ganho vocal a 0 dB sem depender da entrada da voz. O Learn Balance pausa e o Perceptual Balance fica neutro enquanto a proteção está ativa. Threshold, hold, release, tempo de retorno e enable são automatizáveis e persistidos pelo APVTS. Consulte [Docs/MusicSilenceProtection.md](Docs/MusicSilenceProtection.md) para a escala e a máquina de estados.

## Interface e monitoramento

A interface 0.4.0 usa tema grafite, voz em ciano, instrumental em azul e correção em laranja. É redimensionável de 900×680 a 1600×1100 e desenhada com primitivas vetoriais do JUCE para DPI elevado. Basic reúne Target Balance, Strength e limites; Advanced reúne Attack, Release, Deadband, threshold, Vocal Hold e Silence Return.

O gráfico guarda 750 frames, equivalentes a 30 s em 25 Hz, e mostra instrumental, voz pré-gain, voz pós-gain e alvo derivado do instrumental. O processador envia frames triviais por uma fila SPSC fixa de 512 posições. Não há alocação, mutex, desenho ou chamada à GUI no áudio. Se a fila encher com o editor fechado, novos frames são descartados; o DSP segue normalmente e frames antigos são descartados ao reabrir o editor.

O diagnóstico opcional mostra diferença, alvo, erro pós-correção, ganho-alvo/aplicado, atividade, sidechain e limitação. O medidor bipolar usa o ganho efetivo e marca Max Boost/Cut.

## Medição e limitações

K-weighting e soma de energia usam como referência a [ITU-R BS.1770-5, Anexo 1, tabelas 1 e 2](https://www.itu.int/dms_pubrec/itu-r/rec/bs/R-REC-BS.1770-5-202311-I!!PDF-E.pdf). A leitura é uma estimativa K de 400 ms suavizada, sem gating de programa: **não é LUFS integrado nem short-term LUFS normativo**, e não foi certificada como medidor normativo.

As energias L/R são somadas. Duplicar um sinal mono em dois canais aumenta a leitura em aproximadamente 3,01 dB. Ajuste Balance Offset conforme o roteamento e a intenção musical. A interface mostra a diferença entre as entradas antes da correção e o ganho aplicado separadamente.

A detecção por energia não distingue perfeitamente voz, instrumentos, ruído e vazamentos. O threshold exige ajuste em gravações ruidosas ou baixas. É preciso avaliar transientes, pausas e resposta com áudio real. Não há separação de fontes, limiter ou proteção de true peak. Ganho positivo pode exceder 0 dBFS: reserve headroom na cadeia.

Processamento float32, principal mono/estéreo e sidechain estéreo/desativado. Os testes cobrem 44,1/48/96 kHz e blocos de 1, 17, 64, 511 e 2048 amostras. Parâmetros são lidos por bloco; automação não é sample-accurate. Blocos vazios não avançam o DSP. O bypass suave depende de o host continuar chamando o plugin; a suspensão abrupta pelo host está fora do controle do processador.

## Testes automatizados

Resultado desta entrega (03/11/2026): build Release x64 concluído com MSVC 19.52.37025.0 e JUCE local 9.0.5. CTest: 3/3 executáveis e 27 cenários aprovados, zero falhas. O helper VST3 carregou o binário e gerou o manifesto 0.4.0. A interface 1180×820 foi renderizada e inspecionada. Não foi realizado teste no Cakewalk Sonar.

`LevelSync.DSP` também verifica preparação em ganho unitário, relação constante, mediana com outliers, pausas por voz/sidechain inválidos, cancelamento, timeout sem perda da referência, independência de bloco, transições contínuas e aprendizado da voz antes do ganho.

`LevelSync.Processor` verifica persistência versionada, conclusão do Learn, atualização e ajuste manual da referência, layouts, ausência de vazamento, preservação do sidechain, passthrough, Enable/Strength/bypass, saída finita e renderização da interface. Gera `build/LevelSync-editor.png`. Relatório: `build/Testing/Temporary/LastTest.log`.

`LevelSync.OfflineMetrics` executa 20 s sintéticos, aplica uma mudança sustentada no instrumental e imprime MAE estável, ganhos máximo/mínimo, movimento acumulado, pico de velocidade e convergência. A metodologia está em `Docs/PerformanceMethodology.md`; os números são regressão sintética, não resultado em música real.

Documentos adicionais: `Docs/ComparisonProtocol.md` descreve comparação controlada com Waves Vocal Rider; `Docs/SonarInstallationAndValidation.md` cobre instalação, roteamento e validação manual no Sonar.

Os testes não substituem execução na DAW. A compatibilidade efetiva no Cakewalk Sonar ainda precisa ser verificada nesse host.

## Testar no Cakewalk Sonar

1. Feche instâncias carregadas do plugin. Copie a pasta inteira `LevelSync.vst3` para `C:\Program Files\Common Files\VST3` (pode exigir administrador), ou configure o scanner para a pasta de saída do build. Faça nova varredura VST.
2. Insira o efeito na faixa de voz. Crie um envio da faixa/bus instrumental para a entrada de sidechain do LevelSync. Não envie a saída da voz de volta ao sidechain.
3. Reproduza ambos. Após pelo menos 400 ms, confira sidechain válido, atividade vocal e medições. Ajuste Vocal Threshold para ativar com voz e rejeitar ruído entre frases.
4. Ajuste manualmente voz e instrumental ao equilíbrio desejado e clique **Learn Balance**. Durante `Preparing`, confira o retorno suave do ganho a 0 dB. Reproduza ao menos 5 s de voz válida; pausas devem pausar a barra. Teste também Cancel.
5. Ao aparecer `Balance learned`, varie sustentadamente o instrumental e confira convergência do ganho da voz, sem alteração do instrumental. Ajuste Balance Offset para refinar manualmente a referência.
6. Retire o envio, silencie a voz e alterne Enable. Confira retenção curta entre frases, retorno suave a 0 dB, ausência de ruído amplificado e ausência de vazamento.
7. Salve e reabra o projeto, conferindo referência aprendida e ajuste manual. Repita nas taxas e buffers usados. Avalie transientes, pausas, automação e headroom com áudio real.
#   l e v e l s y n c  
 