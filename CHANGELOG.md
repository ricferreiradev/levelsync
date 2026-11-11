# Changelog

Todas as alterações relevantes serão documentadas neste arquivo. O formato segue [Keep a Changelog](https://keepachangelog.com/pt-BR/1.1.0/) e o projeto pretende seguir [Versionamento Semântico](https://semver.org/lang/pt-BR/).

## [Unreleased]

## [0.5.1] - 2026-11-11

### Changed

- Interface compactada para ocupar menos espaço na tela, preservando todos os controles.
- Painéis de monitoramento, Learn Balance e controles reorganizados para melhorar o aproveitamento vertical.

### Fixed

- Espaçamento e recorte dos controles inferiores de Music Silence Protection.

### Added

- Estrutura de documentação e governança para publicação no GitHub.
- Licença AGPL-3.0-or-later e avisos de terceiros.
- Empacotamento ZIP para Standalone e VST3.
- Instalador VST3 para Windows com Inno Setup.
- Publicação automática de releases a partir de tags versionadas.

## [0.5.0] - 2026-11-11

### Added

- Perceptual Balance com análise espectral em 24 bandas ERB.
- Music Silence Protection configurável.
- Formato Standalone além do VST3.
- Interface profissional, diagnóstico e telemetria.
- Learn Balance com referência espectral persistente.

### Fixed

- Retorno da voz para ganho unitário durante paradas prolongadas do instrumental.
- Ordem de desenho do painel Diagnostics.

## [0.1.0] - 2026-11-03

### Added

- Estrutura inicial do plugin VST3 com JUCE, CMake e passthrough.
