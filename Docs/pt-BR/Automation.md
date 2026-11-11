# Automação na DAW: modos Write e Read

O LevelSync 0.5.2 expõe um parâmetro automatizável chamado `Rider Gain`. A faixa é de -12 a +12 dB e o valor padrão é 0 dB.

## Modos

### Automation Off

O controlador automático calcula e aplica o ganho internamente. Nenhum ponto de Rider Gain é enviado ao host. Este modo preserva o comportamento das versões anteriores do LevelSync.

### Automation Write

O LevelSync continua calculando e aplicando o ganho internamente enquanto publica a correção aplicada no parâmetro `Rider Gain` pela API de automação do host.

- As atualizações são limitadas à taxa de controle e telemetria de 25 Hz.
- Um novo ponto é enviado somente quando o ganho difere pelo menos 0,05 dB do último valor publicado.
- As notificações e os gestos de automação são executados na thread de mensagens, nunca dentro do callback de áudio em tempo real.
- O valor de `Rider Gain` devolvido pelo host não é aplicado à saída durante Write, evitando ganho duplicado.

Para gravar:

1. Exiba na DAW a pista de automação `Rider Gain` do plugin.
2. Coloque essa pista ou o modo de automação do plugin em Write, Touch ou Latch, conforme o host.
3. Selecione **Automation Write** no cabeçalho do LevelSync.
4. Reproduza do início ao fim o trecho desejado.
5. Pare a reprodução e altere a pista da DAW para Read.
6. Selecione **Automation Read** no LevelSync.

## Automation Read

No modo Read, o motor automático continua medindo o áudio para os medidores e diagnósticos, mas o ganho calculado por ele não é usado na saída. A voz segue exclusivamente o valor `Rider Gain` controlado pela DAW.

Cada mudança recebida por bloco é suavizada durante 20 ms para evitar clicks. A curva pode ser editada, copiada, simplificada ou apagada com as ferramentas normais de automação da DAW.

## Comportamento importante

- Automatize `Rider Gain`, e não o fader de volume da faixa, quando quiser manter a curva associada ao LevelSync.
- Não deixe a pista da DAW em Write depois da gravação, pois ela poderá substituir edições manuais.
- Use Automation Read para reproduzir uma curva editada. Se o LevelSync permanecer em Write, ele publicará novos valores.
- A automação de parâmetros é lida uma vez por bloco e não possui precisão por amostra.
- O Standalone apresenta os controles, mas não possui uma DAW para gravar automação.
- O comportamento varia entre hosts. A gravação iniciada pelo plugin deve ser validada em cada DAW antes do uso em produção.

## Segurança e alternativa

Se o host não gravar mudanças de parâmetro iniciadas pelo plugin, mantenha Automation Off e use o processamento interno do LevelSync, ou desenhe manualmente `Rider Gain` em Automation Read. O algoritmo de balanceamento existente não foi alterado no modo Off.
