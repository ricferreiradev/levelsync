# LevelSync

LevelSync é um plugin de balanceamento vocal para Windows que ajusta suavemente o nível da voz em relação a um sidechain instrumental. O projeto gera os formatos **VST3** e **Standalone** com C++17, JUCE e CMake.

> Estado do projeto: beta. Os testes automatizados cobrem o DSP, o processador e a interface, mas não substituem validação auditiva em uma DAW real.

## Recursos

- Entrada principal mono ou estéreo para voz.
- Sidechain estéreo dedicado para o instrumental.
- Controle automático com attack, release, deadband, strength e limites de ganho.
- Learn Balance com referência robusta e persistente.
- Perceptual Balance opcional com análise espectral em 24 bandas ERB.
- Music Silence Protection para devolver a voz a 0 dB durante paradas musicais.
- Interface redimensionável com histórico, medidores e diagnóstico.
- Processamento em tempo real sem mutex ou alocação após `prepareToPlay()`.
- VST3 e aplicativo Standalone para inspeção da interface sem abrir a DAW.

## Requisitos

- Windows 10 ou mais recente, x64.
- CMake 3.22 ou mais recente.
- Visual Studio 2022 ou mais recente com Desktop development with C++ e Windows SDK.
- JUCE 9.0.5. Por padrão, o projeto procura o framework em `C:\Dev\JUCE`.

O JUCE não é copiado para este repositório. Use a versão indicada para builds reproduzíveis e confira as obrigações em [Licenciamento](#licenciamento).

## Compilação

Depois de clonar ou baixar o código-fonte, abra PowerShell na raiz do projeto:

```powershell
cd C:\caminho\para\LevelSync

cmake -S . -B build -A x64 `
  -DJUCE_DIR="C:/Dev/JUCE" `
  -DBUILD_TESTING=ON

cmake --build build --config Release `
  --target LevelSync_VST3 LevelSync_Standalone `
           LevelSyncDSPTests LevelSyncProcessorTests LevelSyncOfflineMetrics `
  --parallel 4

ctest --test-dir build -C Release --output-on-failure
```

Artefatos gerados:

- VST3: `build/LevelSync_artefacts/Release/VST3/LevelSync.vst3`
- Standalone: `build/LevelSync_artefacts/Release/Standalone/LevelSync.exe`

## Pacotes de release

O repositório possui um workflow separado que publica releases ao receber uma tag no formato `vMAJOR.MINOR.PATCH`. A versão da tag deve ser igual à versão declarada em `CMakeLists.txt`.

Exemplo para a versão atual:

```powershell
git tag -a v0.5.0 -m "LevelSync 0.5.0"
git push origin v0.5.0
```

Depois do build e dos testes, a GitHub Release recebe:

- `LevelSync-0.5.0-Standalone-Windows-x64.zip`
- `LevelSync-0.5.0-VST3-Windows-x64.zip`
- `LevelSync-0.5.0-VST3-Setup-Windows-x64.exe`
- `SHA256SUMS.txt`

Os ZIPs incluem `LICENSE`, `NOTICE`, `README.md` e `THIRD_PARTY_NOTICES.md`. O ZIP VST3 preserva a pasta completa `LevelSync.vst3`; não extraia somente o binário interno.

Para gerar os ZIPs localmente depois do build:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File Packaging/CreateReleasePackages.ps1 `
  -BuildDirectory ./build -OutputDirectory ./release -Version 0.5.0
```

Para compilar o instalador, instale o Inno Setup e execute:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File Packaging/BuildInnoInstaller.ps1 `
  -BuildDirectory ./build -OutputDirectory ./release -Version 0.5.0 `
  -RepositoryUrl "https://github.com/SEU-USUARIO/LevelSync"
```

O instalador requer privilégios administrativos porque grava no diretório VST3 compartilhado do Windows. Builds locais e do CI não são assinados digitalmente por padrão; uma release pública pode exibir aviso do Microsoft Defender SmartScreen até que seja configurado um certificado de assinatura de código.

## Instalação em uma DAW

1. Feche instâncias existentes do LevelSync.
2. Copie a pasta inteira `LevelSync.vst3` para `C:\Program Files\Common Files\VST3`.
3. Execute uma nova varredura de plugins VST3 na DAW.
4. Insira o LevelSync na faixa de voz.
5. Envie o bus ou faixa instrumental para o sidechain estéreo do plugin.

O sidechain é somente leitura e nunca é misturado à saída vocal. Consulte [o guia completo de instalação e validação](Docs/DAWInstallationAndValidation.md).

## Uso básico

1. Reproduza voz e instrumental por alguns segundos.
2. Ajuste `Vocal Threshold` para reconhecer a voz sem reagir ao ruído de fundo.
3. Defina `Target Balance`, limites, attack e release.
4. Opcionalmente, ajuste manualmente uma relação musical adequada e use `Learn Balance`.
5. Mantenha Music Silence Protection habilitado para preservar o nível original da voz em paradas do instrumental.

As leituras exibidas são estimativas K-weighted de 400 ms. Elas não são medições LUFS integradas ou short-term normativas.

## Arquitetura

```text
Entrada vocal ──► Loudness/VAD ──► Balance Controller ──► Gain Smoother ──► Saída
                         ▲                  ▲
Sidechain musical ───────┴─► Silence + Spectral Analysis
```

- `Source/DSP`: componentes DSP independentes da interface.
- `Source/UI`: tema, medidores e gráficos.
- `Source/PluginProcessor.*`: buses, parâmetros, estado e ponte de telemetria.
- `Source/PluginEditor.*`: interface compartilhada por VST3 e Standalone.
- `Tests`: testes DSP, integração do processador, persistência e renderização.

## Documentação

- [Music Silence Protection](Docs/MusicSilenceProtection.md)
- [Instalação e validação em DAWs](Docs/DAWInstallationAndValidation.md)
- [Metodologia de desempenho](Docs/PerformanceMethodology.md)
- [Protocolo de comparação](Docs/ComparisonProtocol.md)
- [Relatório de validação](Docs/ValidationReport.md)
- [Checklist de release](Docs/ReleaseChecklist.md)
- [Empacotamento e publicação](Packaging/README.md)
- [Como contribuir](CONTRIBUTING.md)
- [Política de segurança](SECURITY.md)
- [Changelog](CHANGELOG.md)

## Limitações conhecidas

- Apenas Windows x64 está configurado no momento.
- O plugin não separa fontes; ruído e vazamento podem afetar detectores.
- Ganho positivo pode exceder 0 dBFS. Reserve headroom na cadeia.
- A automação é lida por bloco, não sample-accurate.
- O Standalone não possui sidechain instrumental separado; sem ele, o controle permanece neutro.
- Compatibilidade real deve ser validada nas versões de DAW e drivers usadas pelo usuário.

## Licenciamento

O código original do LevelSync é distribuído sob a **GNU Affero General Public License v3.0 ou posterior** (`AGPL-3.0-or-later`). Consulte [LICENSE](LICENSE).

O projeto usa JUCE 9.0.5, que é oferecido sob AGPLv3 ou licença comercial. Este repositório adota a opção AGPL. Se você quiser distribuir uma versão proprietária ou não cumprir a AGPL, deverá obter uma licença comercial adequada do JUCE e revisar a licença do restante do projeto.

Distribuições binárias devem disponibilizar o código-fonte correspondente, os scripts de build, esta licença e os avisos de terceiros pelo mesmo meio, conforme aplicável. Consulte [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

Copyright © 2026 LevelSync contributors.

O software é fornecido sem garantia. Este resumo não substitui os textos das licenças nem aconselhamento jurídico.
