# Contribuindo com o LevelSync

Obrigado pelo interesse em melhorar o LevelSync.

## Antes de começar

- Leia o [README](README.md), a [licença](LICENSE) e os [avisos de terceiros](THIRD_PARTY_NOTICES.md).
- Ao enviar uma contribuição, você concorda que ela será distribuída sob `AGPL-3.0-or-later`.
- Não envie código, presets, áudio, imagens ou outros materiais sem permissão para redistribuí-los sob termos compatíveis.
- Mudanças que dependam de uma licença comercial do JUCE não devem ser misturadas ao caminho de build AGPL deste repositório.

## Fluxo recomendado

1. Abra uma issue descrevendo bugs ou mudanças grandes.
2. Crie um branch a partir da versão mais recente.
3. Faça alterações pequenas e focadas.
4. Adicione testes para mudanças no DSP, estado, parâmetros ou layouts.
5. Compile VST3 e Standalone em Release x64.
6. Execute toda a suíte com CTest.
7. Atualize documentação e `CHANGELOG.md` quando necessário.

## Verificação local

```powershell
cmake -S . -B build -A x64 -DJUCE_DIR="C:/Dev/JUCE" -DBUILD_TESTING=ON
cmake --build build --config Release --parallel 4
ctest --test-dir build -C Release --output-on-failure
```

## Regras para DSP em tempo real

- Não aloque memória nem use mutex na thread de áudio.
- Não acesse GUI, arquivos ou rede em `processBlock()`.
- Preserve o sidechain como somente leitura.
- Sanitize NaN e infinito sem propagar valores inválidos.
- Documente escalas: dBFS, estimativa K-weighted e ganho em dB não são intercambiáveis.
- Teste 44,1, 48 e 96 kHz e blocos pequenos, irregulares e grandes.

## Pull requests

Explique o problema, a solução, os testes executados e riscos conhecidos. Não declare validação em uma DAW que não tenha sido realmente realizada.
