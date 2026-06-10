# Roadmap inicial

## Marco 0: Analise e preparacao

Status: iniciado.

- Estudar referencias.
- Registrar decisoes de licenca.
- Definir arquitetura alvo.
- Evitar copia literal de codigo de terceiros.

## Marco 1: Base compilavel

- Criar projeto PlatformIO.
- Configurar ambiente ESP32.
- Criar estrutura de pastas.
- Adicionar `main.cpp` minimo.
- Inicializar Serial.

## Marco 2: Hardware CYD

Status: concluido na placa TPM408-2.8 com perfil ILI9342.

- Mapear pinos em `src/config/pins.h`.
- Inicializar display.
- Inicializar touch.
- Inicializar SD.
- Validar cartao FAT32.

## Marco 3: UI minima

Status: implementado inicialmente.

- Tela HOME.
- Tela FILE_BROWSER.
- Tela PLAYER sem audio.
- Tela SETTINGS simples.
- Navegacao por toque.

## Marco 4: Audio base

Status: concluido como base funcional; TAP usa timer de hardware e DAC com amplitude configuravel no GPIO 26.

- Criar `AudioOutput`.
- Implementar primeira saida simples.
- Adicionar tons de teste:
  - 1 kHz
  - 1200 Hz
  - 2400 Hz
- Adicionar volume e polaridade.

## Marco 5: WAV

Status: implementado.

- Ler WAV PCM do SD.
- Reproduzir via saida selecionada.
- Validar nivel de sinal.

## Marco 6: TAP Spectrum/TK90X

Status: implementado inicialmente com TAP padrao.

- Implementar parser TAP.
- Implementar timings ROM padrao.
- Gerar pulsos em tempo real.
- Testar arquivos simples.

## Marco 7: CAS MSX

Status: iniciado com CAS BIOS em 1200 baud.

- Implementar parser CAS.
- Implementar geracao Kansas City/MSX BIOS.
- Testar CLOAD/BLOAD com arquivos pequenos.

## Marco 8: Servidor web de arquivos

Status: implementado inicialmente.

- Configurar WiFi pela tela `Menu > Config WiFi`.
- Escanear redes disponiveis.
- Selecionar rede e informar senha por teclado na tela.
- Salvar credenciais na memoria do ESP32.
- Mostrar IP no rodape.
- Criar servidor HTTP na porta 80.
- Enviar `.cas` para `/msx`.
- Enviar `.tap` para `/tk90x`.
- Listar arquivos existentes na pagina web.

## Marco 9: TZX/TSX

- Criar stubs claros.
- Implementar blocos comuns primeiro.
- Mensagem clara para bloco nao suportado.
