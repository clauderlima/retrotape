# Hardware

## Placa alvo

ESP32-2432S028 / ESP32-2432S028R, conhecida como Cheap Yellow Display ou CYD.

## Pinagem inicial assumida

Esta pinagem e comum nas placas CYD 2.8" resistivas, mas deve ser confirmada na placa real antes de ligarmos audio e perifericos definitivos.

### Display TFT TPM408-2.8

| Sinal | GPIO |
| --- | --- |
| MISO | 12 |
| MOSI | 13 |
| SCLK | 14 |
| CS | 15 |
| DC | 2 |
| RST | -1 |
| Backlight | 21 |

Na placa real testada, o vidro esta marcado como TPM408-2.8. Essa revisao exibiu imagem espelhada e area embaralhada com a configuracao ILI9341 generica. O firmware usa `Panel_ILI9342` da LovyanGFX, com memoria/painel em 320x240.

### Touch XPT2046

| Sinal | GPIO |
| --- | --- |
| IRQ | 36 |
| MOSI | 32 |
| MISO | 39 |
| SCLK | 25 |
| CS | 33 |

O touch esta configurado em SPI por software no firmware atual. Essa escolha evita conflito com o microSD, que usa o barramento VSPI de hardware.

Na placa TPM408-2.8 testada, o touch fica fisicamente em orientacao retrato em relacao ao display 320x240. O firmware usa `offset_rotation = 3` no XPT2046 para alinhar o toque com a tela em paisagem.

### microSD

| Sinal | GPIO |
| --- | --- |
| MISO | 19 |
| MOSI | 23 |
| SCLK | 18 |
| CS | 5 |

### Audio inicial

| Funcao | GPIO |
| --- | --- |
| DAC / audio inicial | 26 |

GPIO 26 e o DAC2 do ESP32 e alimenta a entrada do amplificador de audio da CYD. O firmware usa o DAC para WAV PCM, CAS MSX e TAP ZX/TK90X. No TAP, um timer de hardware de 10 MHz altera diretamente o valor do DAC em cada borda, com resolucao de 0,1 us e amplitude configuravel. Isso mantem a interface livre para o botao Stop sem depender de I2S/DMA.

O conector `SPEAK/P4` nao e o GPIO 26 direto: ele e a saida em ponte do amplificador SC8002B. Os dois terminais do P4 sao saidas ativas; nenhum deles deve ser curto-circuitado ao GND da placa.

Ligacao de teste usando o conector P4:

```text
um terminal P4 -> capacitor 1uF a 10uF -> resistor 4.7k a 10k -> TIP / EAR
GND da placa ----------------------------------------------------> sleeve / GND

outro terminal P4: deixar desconectado
```

Se usar capacitor eletrolitico, deixe o lado positivo voltado para o P4. Nao use o segundo terminal do P4 como terra. O resistor de 1k usado nos primeiros testes pode funcionar, mas 4.7k a 10k reduz a carga sobre o amplificador e e um ponto inicial mais seguro. Um potenciometro de 10k pode ser usado como atenuador.

Uma saida tomada antes do amplificador, diretamente no GPIO 26, seria eletricamente mais previsivel, mas esse pino nao esta disponivel nos conectores externos da CYD e exigiria modificacao fisica na placa.

Para TAP ZX/TK90X, use a mesma ligacao. O computador deve estar em `LOAD ""` antes de tocar o arquivo. O sinal TAP tem duty cycle de 50% e timings ROM padrao: piloto 2168 T-states, sincronismos 667/735 e bits 855/1710 T-states por semiciclo.

Para CAS MSX, use a mesma ligacao na entrada de cassette do MSX. O firmware gera CAS em 1200 baud nesta fase. O comando de carga depende do tipo de arquivo salvo na fita:

```text
CLOAD        ; BASIC
BLOAD"CAS:",R ; binario/maquina, quando aplicavel
```

## Saida de audio recomendada

Para carregar programas em TK90X/ZX Spectrum/MSX, a saida onboard pode ser suficiente para testes, mas pode ter ruido, distorcao ou nivel inadequado.

O caminho recomendado para estabilidade sera um DAC I2S externo, como PCM5102A, depois que o firmware base estiver funcionando.

## Cuidados

- Comecar com volume baixo e aumentar aos poucos.
- Evitar ligar saida amplificada diretamente em entradas sensiveis sem atenuacao.
- Validar a forma de onda em osciloscopio quando possivel.
- A entrada EAR/CASSETTE pode exigir ajuste de polaridade e amplitude.
- Algumas revisoes da CYD podem mudar pinos ou comportamento do backlight.

## Referencias de pinagem

- CYD community pin map: https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/blob/main/PINS.md
- CYD original schematic archive: https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/tree/main/OriginalDocumentation/5-Schematic
- ESP3D Sunton ESP32-2432S028R: https://esp3d.io/esp3d-tft/version_1x/hardware/esp32/sunton-28-2432/
- ESP-IDF timer driver: https://docs.espressif.com/projects/esp-idf/en/v4.2/esp32/api-reference/peripherals/timer.html
