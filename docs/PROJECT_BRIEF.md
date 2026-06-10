# Briefing normalizado

## Nome

RetroTape-ESP32-CYD

## Objetivo

Criar um firmware para ESP32-2432S028, tambem conhecida como Cheap Yellow Display ou CYD, para funcionar como um emulador moderno de fita cassete para computadores antigos.

Alvos iniciais:

- TK90X / ZX Spectrum
- MSX
- WAV como teste direto de audio

## Fluxo esperado

1. Usuario liga a CYD.
2. Tela touch mostra a interface inicial.
3. Usuario escolhe TK90X/Spectrum, MSX ou WAV.
4. Firmware lista arquivos do cartao SD.
5. Usuario toca no arquivo desejado.
6. Firmware abre tela de player.
7. Usuario pressiona Play.
8. Firmware gera o sinal de audio correspondente na saida da ESP32.
9. Sinal e conectado na entrada EAR/CASSETTE do computador antigo.

## Escopo da primeira versao funcional

- Plataforma: ESP32-2432S028 / CYD.
- Framework: PlatformIO com Arduino Framework.
- Linguagem: C++17.
- UI: LVGL preferencialmente; se atrasar a base, usar LovyanGFX ou TFT_eSPI com uma abstracao simples.
- SD: FAT32, leitura de diretorios e arquivos.
- Formatos v1:
  - WAV
  - TAP para TK90X/ZX Spectrum
  - CAS para MSX, se viavel apos estabilizar audio
- Formatos v2:
  - TZX
  - TSX/TSZ

## Arquitetura desejada

```text
src/
  main.cpp
  config/
    pins.h
  app/
    AppController.h
    AppController.cpp
  storage/
    SdCardService.h
    SdCardService.cpp
  ui/
    ScreenHome.*
    ScreenFileBrowser.*
    ScreenPlayer.*
    ScreenSettings.*
  audio/
    AudioOutput.h
    DacOutput.*
    PwmOutput.*
    I2sOutput.*
  tape/
    TapePlayer.*
    TapeFormatDetector.*
    TapParser.*
    CasParser.*
    TzxParser.*
    TsxParser.*
    PulseGenerator.*
    TapeBlock.*
    TapeTiming.*
docs/
```

## Regra principal de arquitetura

UI, parser e audio nao devem ficar na mesma classe.

O caminho desejado e:

```text
arquivo no SD -> parser -> blocos/pulsos -> fila/buffer -> AudioOutput
```

## Prioridade imediata

O primeiro marco nao e reproduzir tudo. O primeiro marco e:

```text
firmware compila, inicia a CYD, mostra UI minima e lista arquivos do SD
```

Depois:

1. audio de teste;
2. WAV;
3. TAP;
4. CAS;
5. TZX/TSX.

