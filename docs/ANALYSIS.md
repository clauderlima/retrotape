# Analise inicial e preparacao

Projeto: RetroTape-ESP32-CYD
Data da analise: 2026-06-07

## Objetivo desta fase

Esta fase estudou os projetos de referencia e definiu o que pode ser aproveitado conceitualmente para uma primeira versao limpa, modular e compilavel para a ESP32-2432S028 / CYD.

Os repositorios foram baixados apenas para estudo em `.analysis/reference-repos/`. Essa pasta fica ignorada pelo Git e nao deve virar parte do firmware.

## Repositorios analisados

### MaxDuino

Origem: https://github.com/rcmolina/MaxDuino
Commit analisado: `12c2178`
Licenca encontrada: nao ha licenca principal clara no topo do repositorio. Um arquivo isolado (`USBStorage.cpp`) menciona MIT, mas isso nao cobre o projeto inteiro.

Arquivos principais analisados:

- `MaxDuino/MaxDuino.ino`
- `MaxDuino/MaxProcessing.cpp`
- `MaxDuino/casProcessing.cpp`
- `MaxDuino/isr.cpp`
- `MaxDuino/buffer.cpp`
- `MaxDuino/buffer.h`
- `MaxDuino/TimerCounter.cpp`
- `MaxDuino/file_utils.cpp`
- `MaxDuino/CheckForExt.cpp`
- `MaxDuino/constants.h`
- `MaxDuino/processing_state.h`
- `platformio.ini`
- `README.md`
- `FILE_TYPES.md`

O que foi observado:

- O firmware e centrado em `setup()`/`loop()`, botoes fisicos, display pequeno, SD via SdFat e saida de audio por pino.
- A reproducao separa parcialmente duas responsabilidades: o loop principal interpreta arquivo e enche buffer, enquanto uma rotina de timer/ISR consome periodos e muda o nivel do pino de audio.
- `MaxProcessing.cpp` concentra o fluxo de TZX/TAP, incluindo blocos TZX comuns como ID10, ID11, ID12, ID13, ID14, ID15, ID20 e outros.
- `casProcessing.cpp` tem uma logica util para entender como CAS/MSX e Dragon convertem bytes em padroes de pulso.
- `buffer.cpp` usa double buffer com duas paginas, protegido por trechos curtos sem interrupcao.
- `TimerCounter.cpp` tenta abstrair timers para diferentes placas.
- A arquitetura e historica e eficiente para AVR, mas muito acoplada a globais, macros, display, botoes e estado de reproducao.

Decisao:

- Nao copiar codigo literal do MaxDuino.
- Reaproveitar apenas conceitos: tabela de timings, fluxo parser -> buffer -> saida, estados de bloco, tratamento de pausa, polaridade e deteccao de extensao.

### TZXDuino dev-hp

Origem: https://gitlab.com/dev-hp/TZXDuino
Commit analisado: `f6aebbc`
Licenca encontrada: nao foi encontrada licenca clara nos arquivos analisados.

Arquivos principais analisados:

- `README.md`
- `TZXDuino.ino`
- `TZXProcessing.ino`
- `Storage.ino`
- `Storage.h`
- `Display.ino`
- `Buttons.ino`
- `TZXDuino.h`
- `userconfig.h`

O que foi observado:

- O README explica com clareza a arquitetura ISR + `TZXLoop`.
- `TZXProcessing.ino` usa `waveBuffer[128]`, ponteiros `wavePos` e `fillPos`, e fatias de 64 posicoes para evitar conflito entre produtor e consumidor.
- A ISR apenas le o buffer e muda o nivel de audio; ela nao escreve de volta no buffer.
- O loop evita atualizacoes caras de display durante playback e alterna contador/progresso para reduzir risco de underrun.
- `Storage.ino` separa melhor o acesso a SD para ODROID-GO e Arduino, uma ideia util para nosso `SdCardService`.

Decisao:

- Nao copiar codigo literal.
- Usar como referencia conceitual forte para o desenho do `PulseGenerator` e da fila de pulsos.

### POWADCR

Origem: https://github.com/hash6iron/powadcr
Commit analisado: `6f0599f`
Licenca encontrada: GPL-3.0 explicita em `LICENSE` e nos cabecalhos de varios arquivos.

Arquivos principais analisados:

- `platformio.ini`
- `README.md`
- `src/config.h`
- `src/globales.h`
- `src/powadcr.cpp`
- `src/TAPprocessor.h`
- `src/TZXprocessor.h`
- `src/TSXprocessor.h`
- `src/PZXprocessor.h`
- `src/ZXProcessor.h`
- `src/SmartRadioBuffer.h`
- `src/PredictiveRadioBuffer.h`
- `HMI.h`

O que foi observado:

- E a referencia mais proxima em hardware moderno, pois usa ESP32, PlatformIO, Arduino Framework, AudioKit, SD_MMC, audio digital e tela HMI.
- `platformio.ini` usa ESP32 a 240 MHz, PSRAM, particao customizada e bibliotecas de audio.
- `config.h` centraliza frequencias de amostragem, velocidade de SD, limites de blocos e parametros de audio.
- `globales.h` define descritores para TAP, TZX, PZX e CSW, alem de muitos estados globais.
- `TAPprocessor.h`, `TZXprocessor.h` e `TSXprocessor.h` analisam arquivos em uma etapa de descritores antes da reproducao.
- `ZXProcessor.h` gera pulsos em amostras PCM e escreve em stream de audio, com funcoes como `semiPulse`, `fullPulse`, `pilotTone`, `zeroTone`, `oneTone` e geracao de silencio.
- `powadcr.cpp` e poderoso, mas muito monolitico para o objetivo inicial deste projeto.

Decisao:

- Nao copiar codigo literal sem decisao explicita de licenca GPL-3.0 para o nosso projeto.
- Reaproveitar conceitos: descritores de blocos, geracao por amostras PCM/I2S, configuracoes de volume/polaridade, teste de audio, cuidado com nivel de sinal e estrutura de SD.
- Reimplementar uma versao menor e modular para CYD.

### SD_Tape_Player

Origem: https://github.com/GadgetReboot/SD_Tape_Player
Commit analisado: `ed9a0ae`
Licenca encontrada: nao foi encontrada licenca clara.

Arquivos principais analisados:

- `README.md`
- `KiCad/SD_Tape_Player.sch`
- `KiCad/SD_Tape_Player.kicad_pcb`
- `SD_Tape_Player-sch.pdf`

O que foi observado:

- E uma referencia de hardware para uma placa compatavel com CASDuino em Arduino Nano.
- O README informa que o PCB foi testado com CASDuino 1.24.
- E util para entender a ideia de conector de audio e controle remoto de cassette, mas nao traz firmware proprio.

Decisao:

- Usar apenas como referencia de hardware/conceito.

## O que sera reaproveitado

- Modelo geral parser -> fila/buffer -> gerador de audio.
- Separacao entre UI e audio para evitar jitter.
- Double buffer ou fila circular de periodos/pulsos.
- Conversao de T-states para tempo real no caso de Spectrum.
- Timings padrao do loader ROM do ZX Spectrum/TK90X.
- Ideia de descritores de blocos antes da reproducao.
- Tratamento explicito de pausa, stop, play, polaridade e progresso.
- Testes de audio com tons fixos e ajuste de volume.
- Cuidados com atualizacao de tela durante playback.

## O que sera reimplementado

- Todo o codigo-fonte do firmware novo.
- `TapParser`, `CasParser`, `TzxParser` e `TsxParser`.
- `PulseGenerator`.
- `TapePlayer`.
- `AudioOutput`, `DacOutput`, `PwmOutput` e `I2sOutput`.
- `SdCardService`.
- UI para CYD, preferencialmente LVGL.
- Configuracao de pinos para ESP32-2432S028.
- Testes de audio.

## Riscos tecnicos

- Pinagem da ESP32-2432S028 varia entre revisoes; display, touch, SD e audio precisam ser validados na placa real.
- Saida de audio onboard pode ter amplitude, ruido ou impedancia inadequados para EAR/CASSETTE.
- I2S com DAC externo PCM5102A deve ser tratado como caminho recomendado para qualidade e estabilidade.
- LVGL e playback simultaneo podem causar jitter se a UI atualizar demais.
- SD compartilhado com display/touch/SPI pode gerar latencia se a arquitetura bloquear a task de audio.
- TZX/TSX tem muitos blocos; v1 deve suportar apenas TAP, WAV e talvez CAS.
- CAS de MSX nao preserva todos os tempos do tape original; cabecalhos e pausas podem exigir heuristicas.
- Copiar codigo de projetos sem licenca clara ou GPL-3.0 pode contaminar a licenca do projeto.

## Estrategia de implementacao

1. Criar base PlatformIO minima.
2. Configurar pinos da CYD em `src/config/pins.h`.
3. Inicializar Serial, display, touch e SD.
4. Criar `AppController` com estados HOME, FILE_BROWSER, PLAYER, SETTINGS e ERROR.
5. Criar `SdCardService` com listagem e filtro por extensao.
6. Criar UI minima com home, navegador e player sem audio.
7. Criar `AudioOutput` e modo de teste com tons.
8. Implementar WAV simples.
9. Implementar TAP Spectrum/TK90X.
10. Implementar CAS MSX se os testes de audio estiverem estaveis.
11. Deixar TZX/TSX como stubs documentados.

## Plano de commits pequenos

1. `docs: add initial reference analysis`
2. `chore: ignore local analysis references`
3. `chore: create platformio project skeleton`
4. `docs: add hardware and architecture notes`
5. `feat: add CYD pin configuration`
6. `feat: initialize serial display touch and sd`
7. `feat: add app controller states`
8. `feat: add sd card file listing`
9. `feat: add player screen skeleton`
10. `feat: add audio output abstraction`
11. `feat: add audio test tones`
12. `feat: add wav playback`
13. `feat: add tap parser and zx timing generator`
14. `feat: add initial msx cas parser`

