# Instalação e validação em uma DAW

Este guia se aplica a DAWs Windows x64 que carreguem plugins VST3 e ofereçam roteamento para sidechain. A localização dos comandos varia entre hosts; consulte a documentação da sua DAW.

## Instalação

1. Feche instâncias do LevelSync carregadas na DAW.
2. Copie o bundle completo `LevelSync.vst3` para uma pasta VST3 escaneada pela DAW. No Windows, o local compartilhado padrão e recomendado é `C:\Program Files\Common Files\VST3`; se a DAW usar outro diretório configurado, copie o bundle para ele ou adicione o caminho padrão ao scanner. A operação pode exigir privilégios administrativos.
3. Abra a DAW, execute uma nova varredura VST3 e confirme que o LevelSync foi detectado.

## Roteamento

1. Insira o LevelSync na faixa vocal. A entrada principal aceita mono ou estéreo e a saída conserva esse layout.
2. Crie um envio do bus ou faixa instrumental para a entrada auxiliar estéreo do LevelSync. Evite enviar a saída vocal de volta ao sidechain.
3. Confirme `Instrumental Loudness` válido e `Vocal Loudness` ativo. O plugin declara zero amostras de latência.

Nem todas as DAWs apresentam ou roteiam buses auxiliares da mesma maneira. Uma DAW sem suporte ao sidechain VST3 não permitirá o funcionamento completo do balanceamento automático.

## Validação manual

1. Teste Enable e o bypass do host com ganho neutro, verificando passagem transparente e ausência de clicks.
2. Faça Learn Balance. Confirme retorno suave a 0 dB em Preparing, pausa do progresso sem voz/sidechain e conclusão após 5 s válidos.
3. Altere o instrumental de forma sustentada. Observe Current/Target Balance, ganho teórico/aplicado, gráfico e indicação de Max Boost/Cut.
4. Teste pausas curtas e longas, Music Silence Protection, Vocal Hold e os retornos a unity. Verifique que ruído não é perseguido.
5. Automatize cada parâmetro, salve, feche e reabra o projeto. Confirme parâmetros e referência aprendida.
6. Repita em 44,1, 48 e 96 kHz, com buffers pequenos e grandes. Faça também renderização offline e compare o resultado ao playback em tempo real.
7. Feche a GUI durante playback e renderização; o áudio deve continuar normalmente.

## Estado da validação

Build, testes headless, manifesto VST3 e renderização da interface são validados automaticamente. Descoberta pelo scanner, exposição real do sidechain, automação do host, restauração de sessão e equivalência entre renderização online/offline permanecem testes manuais específicos de cada DAW.

Compatibilidade com o padrão VST3 não garante comportamento idêntico em todas as DAWs. Registre nome e versão do host, formato do bus, sample rate, buffer e driver ao relatar resultados.
