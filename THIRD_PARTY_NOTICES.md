# Third-party notices

LevelSync depende de componentes de terceiros que preservam seus próprios direitos autorais e licenças.

## JUCE 9.0.5

Copyright © Raw Material Software Limited.

JUCE é disponibilizado sob licenciamento duplo: GNU Affero General Public License v3.0 e licença comercial JUCE. O build open source deste repositório usa JUCE sob AGPLv3.

- Licença incluída na distribuição JUCE: `JUCE/LICENSE.md`
- Inventário completo de dependências: `JUCE/JUCE.spdx.json`
- Site: <https://juce.com/>
- Licença JUCE 9: <https://juce.com/legal/juce-9-licence/>

O código-fonte do JUCE não é mantido neste repositório. Para reproduzir o build, use JUCE 9.0.5 e configure `JUCE_DIR` no CMake. Ao publicar binários sob AGPL, assegure acesso equivalente ao código-fonte correspondente do JUCE usado no build.

## Steinberg VST 3 SDK

O JUCE 9.0.5 inclui partes do VST 3 SDK usadas para gerar o formato VST3.

MIT License

Copyright © 2025 Steinberg Media Technologies GmbH

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

## Windows system libraries

O build usa bibliotecas fornecidas pelo Windows SDK e pelo runtime do Microsoft Visual C++. Esses componentes são distribuídos segundo os termos da Microsoft e não fazem parte do código-fonte do LevelSync.

## Inno Setup

O instalador VST3 para Windows é compilado com Inno Setup. O compilador não é incorporado ao repositório; apenas o instalador gerado é distribuído.

Copyright © 1997–2026 Jordan Russell e contribuidores do Inno Setup.

- Site e downloads: <https://jrsoftware.org/isinfo.php>
- Licença do Inno Setup 6: <https://jrsoftware.org/files/is6-license.txt>
- Licença do Inno Setup 7: <https://jrsoftware.org/files/is7-license.txt>

Quem reutilizar o workflow deve revisar os termos da versão do Inno Setup instalada, especialmente em uso comercial.

## Marcas

JUCE, VST, VST3 e as marcas das DAWs mencionadas pertencem a seus respectivos titulares. O uso dos nomes neste repositório serve apenas para identificar compatibilidade e não implica endosso.
