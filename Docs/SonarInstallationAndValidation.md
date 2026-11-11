# Instalação e validação no Cakewalk Sonar

## Instalação

1. Feche instâncias do LevelSync carregadas no Sonar.
2. Copie o bundle completo `LevelSync.vst3` para `C:\Program Files\Common Files\VST3`. A operação pode exigir privilégios administrativos.
3. Abra o Sonar, execute nova varredura VST3 e confirme a versão 0.4.0.

## Roteamento

1. Insira LevelSync na faixa vocal. A entrada principal aceita mono ou estéreo e a saída conserva esse layout.
2. Crie um envio do bus ou faixa instrumental para a entrada auxiliar estéreo do LevelSync. Evite enviar a saída vocal de volta ao sidechain.
3. Confirme `Instrumental Loudness` válido e `Vocal Loudness` ativo. O plugin declara zero amostras de latência.

## Validação manual

1. Teste Enable e o bypass do host com ganho neutro, verificando passagem transparente e ausência de clicks.
2. Faça Learn Balance. Confirme retorno suave a 0 dB em Preparing, pausa do progresso sem voz/sidechain e conclusão após 5 s válidos.
3. Altere o instrumental de forma sustentada. Observe Current/Target Balance, ganho teórico/aplicado, gráfico e indicação de Max Boost/Cut.
4. Teste pausas curtas e longas, Vocal Hold e Silence Return. Verifique que ruído não é perseguido.
5. Automatize cada parâmetro, salve, feche e reabra o projeto. Confirme parâmetros e referência aprendida.
6. Repita em 44,1, 48 e 96 kHz, com buffers pequenos e grandes. Faça também renderização offline e compare o resultado ao playback em tempo real.
7. Feche a GUI durante playback e renderização; o áudio deve continuar normalmente.

## Estado da validação

Build, testes headless, VST3 manifest e renderização da interface são validados automaticamente. A execução dentro do Cakewalk Sonar, o scanner VST3, a exposição real do sidechain, automação do host e equivalência entre renderização online/offline permanecem testes manuais pendentes neste ambiente.
