# Checklist de release

## Código e testes

- [ ] Atualizar versão em `CMakeLists.txt` e `CHANGELOG.md`.
- [ ] Configurar com JUCE 9.0.5 e Release x64.
- [ ] Compilar `LevelSync_VST3` e `LevelSync_Standalone`.
- [ ] Executar CTest com zero falhas.
- [ ] Abrir o Standalone e inspecionar a interface em 100% e escala DPI elevada.
- [ ] Validar VST3 no Cakewalk Sonar com sidechain real.
- [ ] Testar 44,1/48/96 kHz e buffers usados na validação manual.

## Distribuição AGPL

- [ ] Incluir `LICENSE`, `NOTICE` e `THIRD_PARTY_NOTICES.md`.
- [ ] Publicar o código-fonte exato correspondente ao binário.
- [ ] Identificar a versão exata do JUCE e disponibilizar acesso ao código correspondente.
- [ ] Incluir scripts CMake e informações suficientes para reproduzir o build.
- [ ] Não incluir arquivos proprietários, projetos de DAW ou áudio sem autorização.
- [ ] Publicar checksums dos artefatos.

## GitHub

- [ ] Criar uma tag assinada no formato `vX.Y.Z`.
- [ ] Anexar a pasta `LevelSync.vst3` compactada sem alterar sua estrutura.
- [ ] Anexar o Standalone separadamente.
- [ ] Copiar notas relevantes do changelog para a release.
- [ ] Informar claramente que testes automatizados não equivalem a validação em todas as DAWs.
