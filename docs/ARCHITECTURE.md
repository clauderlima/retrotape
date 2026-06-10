# Arquitetura

## Objetivo da arquitetura

Separar interface, armazenamento, parser de fita e saida de audio. Isso evita uma solucao monolitica e reduz o risco de travar a reproducao quando a tela for atualizada.

## Fluxo desejado

```text
SD card -> parser -> blocos de fita -> pulsos -> AudioOutput
                              ^
                              |
                              UI consulta estado/progresso

Browser web -> FileWebServer -> SD card
```

## Camadas

### `src/main.cpp`

Ponto de entrada Arduino. Cria os servicos principais e chama `AppController`.

### `src/app`

Controla estado global da aplicacao.

Estados planejados:

- `Home`
- `FileBrowser`
- `Player`
- `Settings`
- `Error`

Na fase 3, o controlador tambem recebe acoes de toque da UI, muda de tela, filtra arquivos por modo e guarda o arquivo selecionado para a tela `Player`.

### `src/storage`

Responsavel por SD, diretorios, arquivos e filtros por extensao.

Classe inicial:

- `SdCardService`

### `src/ui`

Interface com o usuario.

Na fase 3, `UiService` desenha as telas principais e mantem zonas de toque para transformar coordenadas em acoes. A UI nao decide regras de negocio; ela apenas devolve comandos como abrir modo, voltar, selecionar linha e trocar pagina.

### `src/audio`

Abstracao da saida de audio.

Classe inicial:

- `AudioOutput`: interface
- `NoopAudioOutput`: implementacao temporaria sem audio real
- `DacAudioOutput`: WAV/CAS pelo DAC e TAP pelo DAC temporizado no GPIO 26

Implementacoes planejadas:

- `PwmOutput`
- `DacOutput`
- `I2sOutput`

Na fase 4, `DacAudioOutput` reproduz WAV PCM 8/16 bits, mono ou stereo pelo loop principal. A UI consulta progresso, mostra tempo decorrido/total e pode interromper a reproducao com `Stop`.

Na fase 5, a mesma saida tambem gera pulsos TAP ZX Spectrum/TK90X. O arquivo TAP e lido bloco a bloco do SD, usando prefixo little-endian de tamanho, pilot tone, sync e pulsos de bits com timings ROM padrao.

Depois dos testes no TK90X, a saida TAP passou a usar o DAC do GPIO 26 comandado pelo timer de hardware do ESP32. O timer trabalha a 10 MHz, com resolucao de 0,1 us, e a ISR apenas troca entre dois niveis do DAC e agenda o proximo pulso. O loop principal le um bloco TAP por vez, atende o touch e carrega o bloco seguinte durante a pausa. As atualizacoes visuais de progresso ficam suspensas durante TAP para evitar jitter. O monitor serial informa tipo e tamanho do bloco, flag, checksum, pilot tone, sync, bits, pausa e atrasos observados no timer.

Essa separacao torna o `Stop` imediato: ele pausa o timer antes de fechar o arquivo e liberar o bloco. Nao existe tarefa de audio esperando por DMA ou escrita no SD.

Na fase 6, `DacAudioOutput` tambem reproduz CAS MSX em 1200 baud. O arquivo CAS e lido como bytes BIOS MSX; marcadores `1F A6 DE BA CC 13 7D 74` geram cabecalhos longos ou curtos por heuristica, e os bytes sao emitidos com start bit, 8 bits LSB primeiro e dois stop bits.

### `src/network`

WiFi e servidor web de arquivos.

Classes iniciais:

- `WifiService`: escaneia redes, conecta em modo station, salva credenciais em `Preferences` e cria AP de fallback quando nao ha rede configurada.
- `FileWebServer`: inicia HTTP na porta 80, mostra pagina de upload/listagem e grava arquivos no SD.

Na fase 7, o servidor aceita `.cas` para `/msx` e `.tap` para `/tk90x`. O WiFi pode ser configurado pela tela `Menu > Config WiFi`, com lista de redes e teclado de senha. O controlador chama o servidor no loop apenas quando o audio nao esta tocando, para reduzir risco de falhas de timing durante reproducao.

### `src/tape`

Formatos, parser e player.

Classes iniciais:

- `TapeFormatDetector`
- `TapePlayer`
- `PulseGenerator`
- `TapParser`
- `CasParser`
- `TzxParser`
- `TsxParser`

Os parsers formais ainda sao stubs, mas o player de audio ja possui suporte inicial a TAP padrao e CAS MSX. TZX, TSX e TSZ ficam para fases posteriores.

## Decisao importante

A UI nao deve gerar audio diretamente. A UI pede a acao para o controlador, o controlador muda o estado, e o player/audio executa a reproducao.

## Concorrencia planejada

O TAP usa timer de hardware e estado nao bloqueante. Para os demais formatos, a arquitetura ainda pode evoluir para:

- task de UI;
- task de leitura/parser;
- task, timer ou periferico dedicado para audio;
- fila de pulsos ou samples entre parser e saida.
