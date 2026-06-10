# Referencias tecnicas

Este documento registra as fontes analisadas e a decisao de uso para o projeto RetroTape-ESP32-CYD.

## Politica de uso de codigo

- Codigo de projetos sem licenca clara nao deve ser copiado.
- Codigo GPL-3.0 nao deve ser copiado sem aceitar que o firmware tambem seja GPL-3.0.
- Nesta fase, todas as referencias sao usadas apenas conceitualmente.
- Timings e formatos devem ser reimplementados a partir de especificacoes publicas.

## Repositorios estudados

### MaxDuino

- URL: https://github.com/rcmolina/MaxDuino
- Commit analisado: `12c2178`
- Descricao: firmware unificado para dispositivos TZXDuino/MaxDuino/CASDuino.
- Formatos citados pelo projeto: TZX, TAP, AY, UEF, TSZ, CAS, CDT, MZF, CAQ e outros.
- Licenca: nao foi encontrada uma licenca principal clara no repositorio.
- Uso permitido neste projeto: referencia conceitual.
- Nao copiar: implementacoes de parser, ISR, buffer, menu ou configuracao sem nova verificacao de licenca.

Arquivos analisados:

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
- `README.md`
- `FILE_TYPES.md`

### TZXDuino dev-hp

- URL: https://gitlab.com/dev-hp/TZXDuino
- Commit analisado: `f6aebbc`
- Descricao: variante/refatoracao de TZXDuino com documentacao clara sobre ISR, buffer e riscos de overrun.
- Licenca: nao foi encontrada uma licenca clara nos arquivos analisados.
- Uso permitido neste projeto: referencia conceitual.
- Nao copiar: codigo de `TZXProcessing.ino`, `Storage.ino` ou demais arquivos sem nova verificacao de licenca.

Arquivos analisados:

- `README.md`
- `TZXDuino.ino`
- `TZXProcessing.ino`
- `Storage.ino`
- `Storage.h`
- `Display.ino`
- `Buttons.ino`
- `TZXDuino.h`
- `userconfig.h`

### POWADCR

- URL: https://github.com/hash6iron/powadcr
- Commit analisado: `6f0599f`
- Descricao: gravador/reprodutor digital de cassette para maquinas de 8 bits, baseado em ESP32 Audio Kit.
- Licenca: GPL-3.0.
- Uso permitido neste projeto: referencia conceitual, salvo se o projeto decidir adotar GPL-3.0.
- Nao copiar: codigo de parser, audio, UI ou bibliotecas vendorizadas sem decisao explicita de licenca.

Arquivos analisados:

- `platformio.ini`
- `README.md`
- `LICENSE`
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

### SD_Tape_Player

- URL: https://github.com/GadgetReboot/SD_Tape_Player
- Commit analisado: `ed9a0ae`
- Descricao: PCB para player CASDuino/TZXDuino baseado em Arduino Nano.
- Licenca: nao foi encontrada uma licenca clara.
- Uso permitido neste projeto: referencia de hardware e conceito.

Arquivos analisados:

- `README.md`
- `KiCad/SD_Tape_Player.sch`
- `KiCad/SD_Tape_Player.kicad_pcb`
- `SD_Tape_Player-sch.pdf`

## Especificacoes e fontes publicas de formato

### ZX Spectrum TAP

- Sinclair Wiki, TAP format: https://sinclair.wiki.zxnet.co.uk/wiki/TAP_format
- Claus Jahn / World of Spectrum, TAP format: https://worldofspectrum.net/zx-modules/fileformats/tapformat.html

Resumo para implementacao:

- Arquivo TAP e uma sequencia de blocos.
- Cada bloco comeca com 2 bytes little-endian indicando o tamanho dos dados seguintes.
- Os dados geralmente incluem byte de flag no inicio e checksum XOR no final, mas o container TAP nao interpreta o conteudo.

### ESP32 e CYD

- ESP-IDF v4.2, timer driver: https://docs.espressif.com/projects/esp-idf/en/v4.2/esp32/api-reference/peripherals/timer.html
- CYD pin map e conector P4: https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/blob/main/PINS.md
- Esquematico original arquivado: https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/tree/main/OriginalDocumentation/5-Schematic
- SC8002B, amplificador de audio em ponte: https://www.alldatasheet.com/datasheet-pdf/pdf/1146797/FUMAN/SC8002B.html

Resumo para implementacao:

- O ESP32 possui quatro timers de hardware em dois grupos; o divisor 8 sobre o clock APB de 80 MHz produz ticks de 0,1 us.
- GPIO 26 e o DAC2 do ESP32 e esta ligado a entrada do amplificador da CYD.
- P4 e a saida do amplificador, nao um GPIO nem um terminal referenciado diretamente ao GND.

### ZX Spectrum TZX

- TZX format specification v1.20: https://worldofspectrum.net/TZXformat.html

Resumo para implementacao:

- TZX preserva timings e blocos complexos.
- Deve ficar para fase posterior.
- Primeiros blocos candidatos: ID10, ID11, ID12, ID13, ID14, ID15, ID20 e mensagens claras para blocos nao suportados.

### MSX CAS e TSX

- MSX Wiki, emulation related file formats: https://www.msx.org/wiki/Emulation_related_file_formats
- MSX2 Technical Handbook, cassette interface: https://konamiman.github.io/MSX2-Technical-Handbook/md/Chapter5a.html

Resumo para implementacao:

- CAS contem bytes decodificados em formato BIOS MSX.
- Sequencia de header conhecida: `1F A6 DE BA CC 13 7D 74`.
- CAS nao preserva timing completo, gaps ou diferenca precisa entre header longo e curto.
- Em 1200 baud, bit 0 usa 1 ciclo de 1200 Hz e bit 1 usa 2 ciclos de 2400 Hz.
- Cada byte e emitido com start bit 0, 8 bits LSB primeiro e dois stop bits 1.
- TSX estende TZX e adiciona bloco ID 4B para dados Kansas City Standard usados pelo MSX BIOS.

## Decisoes abertas

- Definir licenca do RetroTape-ESP32-CYD antes de qualquer copia de codigo GPL-3.0.
- Confirmar pinagem exata da CYD usada pelo usuario.
- Confirmar se a primeira saida de audio sera GPIO/PWM/DAC interno ou I2S PCM5102A.
- Definir se LVGL entra na primeira etapa ou se a primeira tela sera feita com LovyanGFX/TFT_eSPI.
