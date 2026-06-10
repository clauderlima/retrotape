# RetroTape-ESP32-CYD

Firmware para transformar a ESP32-2432S028 / Cheap Yellow Display em um player de fitas digitais para computadores antigos.

## Estado atual

Fase 7: servidor web de arquivos inicial.

Nesta fase ele:

- compila como projeto PlatformIO;
- inicializa Serial;
- monta uma estrutura modular;
- inicializa display TPM408-2.8 com perfil ILI9342 via LovyanGFX;
- inicializa touch XPT2046 via SPI por software;
- tenta montar o cartao SD;
- mostra uma tela HOME navegavel por toque;
- filtra arquivos por TK90X/ZX, MSX ou WAV;
- abre uma tela de navegador de arquivos do SD;
- abre uma tela PLAYER ao selecionar arquivo;
- mostra uma tela MENU simples;
- reproduz WAV PCM pelo DAC interno do ESP32 no GPIO 26;
- reproduz TAP ZX Spectrum/TK90X pelo DAC, comandado por timer de hardware no GPIO 26;
- reproduz CAS MSX BIOS em 1200 baud pelo GPIO 26;
- cria pastas `/msx`, `/tk90x` e `/wav` no cartao SD;
- permite configurar WiFi pelo Menu usando scan de redes e teclado na tela;
- inicia servidor web para upload e listagem de arquivos;
- mostra o IP do servidor no rodape da tela;
- mostra tempo decorrido/total no Player;
- permite interromper WAV/TAP/CAS com o botao Stop;
- deixa stubs para os demais formatos de fita.

## Placa alvo

- ESP32-2432S028 / ESP32-2432S028R / Cheap Yellow Display
- ESP32-WROOM, tela 320x240, touch resistivo XPT2046, microSD e speaker em GPIO 26 nas revisoes comuns

## Requisitos no computador

- Visual Studio Code
- Extensao PlatformIO IDE instalada no VSCode

O comando `pio` ainda nao esta disponivel no terminal deste ambiente. Isso nao impede o uso pelo VSCode: abra esta pasta no VSCode e use os botoes do PlatformIO.

## Como abrir no VSCode

1. Abra o VSCode.
2. Va em `File > Open Folder`.
3. Escolha a pasta `C:\Users\claud\OneDrive\Documentos\CasPlayer`.
4. Aguarde o PlatformIO detectar o arquivo `platformio.ini`.
5. No menu do PlatformIO, use `Build`.

## Estrutura inicial

```text
src/
  main.cpp
  app/
  audio/
  config/
  network/
  storage/
  tape/
  ui/
docs/
platformio.ini
```

## Servidor web

O firmware inicia um servidor HTTP na porta 80. O IP aparece no rodape como `Web x.x.x.x`.

Para usar na rede WiFi da casa:

1. Na tela inicial, toque em `Menu`.
2. Toque em `Config WiFi`.
3. Escolha a rede na lista.
4. Digite a senha no teclado da tela.
5. Toque em `Conectar`.

O ESP32 salva a rede e tenta reconectar sozinho nos proximos boots.

Se ainda nao houver rede configurada, o ESP32 cria uma rede propria:

```text
Rede: RetroTape
Senha: 12345678
IP: 192.168.4.1
```

A pagina web permite selecionar `MSX` ou `TK90X / ZX` e enviar arquivos. O firmware salva:

| Plataforma | Extensao | Pasta |
| --- | --- | --- |
| MSX | `.cas` | `/msx` |
| TK90X / ZX | `.tap` | `/tk90x` |

## Formatos planejados

| Formato | Status |
| --- | --- |
| WAV | reproduz PCM 8/16 bits mono/stereo |
| TAP ZX Spectrum/TK90X | DAC com DMA, nivel/timing/polaridade ajustaveis e log de blocos |
| CAS MSX | reproduz CAS BIOS em 1200 baud |
| TZX | stub futuro |
| TSX/TSZ | stub futuro |

## Proximo marco

Proximos ajustes:

- testar upload real pela rede;
- adicionar exclusao/renomeacao de arquivos pela pagina;
- preparar TSX/TZX ou melhorar buffer/timing conforme os testes reais.
