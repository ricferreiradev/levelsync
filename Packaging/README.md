# Empacotamento e publicação

## Artefatos

O pipeline produz três formas de distribuição para Windows x64:

| Arquivo | Conteúdo |
| --- | --- |
| `LevelSync-VERSION-Standalone-Windows-x64.zip` | Aplicativo Standalone e documentos legais |
| `LevelSync-VERSION-VST3-Windows-x64.zip` | Bundle completo `LevelSync.vst3` e documentos legais |
| `LevelSync-VERSION-VST3-Setup-Windows-x64.exe` | Instalador administrativo do VST3 |

O Inno Setup gera instaladores `.exe`; ele não gera pacotes Windows Installer `.msi`.

## ZIPs

Compile os alvos Release antes de executar:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File Packaging/CreateReleasePackages.ps1 `
  -BuildDirectory ./build `
  -OutputDirectory ./release `
  -Version 0.5.1
```

O script falha se os binários ou documentos legais estiverem ausentes. O VST3 permanece como um bundle, com `Contents/Resources` e `Contents/x86_64-win` dentro de `LevelSync.vst3`.

## Instalador Inno Setup

Instale o Inno Setup e execute:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File Packaging/BuildInnoInstaller.ps1 `
  -BuildDirectory ./build `
  -OutputDirectory ./release `
  -Version 0.5.1 `
  -RepositoryUrl "https://github.com/SEU-USUARIO/LevelSync"
```

O instalador:

- requer Windows x64 e privilégios administrativos;
- mostra a AGPL e os avisos de terceiros;
- instala o bundle em `{Common Files}\VST3\LevelSync.vst3`;
- adiciona uma entrada de desinstalação do Windows;
- remove os arquivos instalados quando desinstalado.

O instalador não é assinado automaticamente. Para distribuição pública profissional, configure assinatura de código em um ambiente protegido e nunca armazene a chave privada ou senha no repositório.

## GitHub Release

`.github/workflows/release.yml` executa somente para tags `vX.Y.Z`, valida a versão do CMake, compila, testa, gera os três pacotes, cria hashes SHA-256 e publica a release por meio do GitHub CLI.

O código-fonte da tag é disponibilizado automaticamente pelo GitHub. Como o build usa JUCE sob AGPLv3, a release também deve identificar e manter acessível o código correspondente do JUCE 9.0.5.
