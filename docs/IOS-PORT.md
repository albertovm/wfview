# iOS / iPadOS: status da portabilidade

Em 4 de novembro de 2026, a compilação Release para dispositivo terminou com
`BUILD SUCCEEDED`. O resultado é um executável Mach-O arm64, com destino mínimo
iOS 16.0 e famílias de dispositivo iPhone e iPad. Ainda não há validação de
execução em dispositivo nem assinatura para instalação.

Artefato local: `/Volumes/1TbSilver/wfview-ios/build/wfview-device/Release-iphoneos/wfview.app`
(aproximadamente 29 MiB, sem assinatura).

Para reproduzir a compilação sem assinatura:

```sh
sh scripts/build-ios.sh
```

O script gera o projeto antes de iniciar o Xcode, evitando que uma regeneração
dentro do build deixe configurações antigas carregadas. O destino iOS usa
Qt Multimedia com o plugin nativo Darwin, sem importar FFmpeg, e liga Opus
estaticamente. Os ajustes para Clang 21, USB opcional, cabeçalhos de áudio
desktop e abertura de logs estão nos fontes do repositório.

## Ambiente verificado

Qt 6.8.3 para macOS e iOS foi instalado em
`/Volumes/1TbSilver/wfview-ios/Qt/6.8.3`. Opus 1.5.2 foi compilado e
instalado para iOS arm64 em `install/iphoneos-arm64` nesse mesmo diretório
raiz. Os fontes de Eigen, QCustomPlot e r8brain estão extraídos em `src/`.

Use `sh scripts/configure-ios.sh` para reproduzir a configuração. O script
invoca o qmake de macOS com `target_qt.conf` do iOS, contornando as aspas
incorretas no wrapper distribuído pelo Qt quando o volume contém espaços.

A dependência Qt SerialPort foi removida do destino iOS. Os controles de serial
física e virtual ficam ocultos, configurações persistidas são normalizadas para
rede e os handlers seriais rejeitam acesso no iOS. Os destinos desktop mantêm
suas implementações. A compilação está concluída; a validação funcional completa
do aplicativo ainda está pendente.

## Hostname e DNS dinâmico

O campo de Radio Access aceita endereço IPv4 ou nome DNS, inclusive nomes de
DNS dinâmico. As conexões TCP resolvem o nome por meio do Qt; os caminhos UDP
Icom e Yaesu resolvem o nome durante cada nova conexão. Assim, depois de uma
desconexão/reconexão, uma alteração de IP publicada pelo DNS é utilizada sem
editar novamente as configurações.
O Qt também avisa que foi testado com SDK 18, enquanto o Xcode instalado fornece
SDK 27.1. A compatibilidade dessa combinação ainda não foi validada.

## Trabalho necessário antes de disponibilizar uma versão funcional

- Validar os caminhos de rede e Qt Multimedia para captura e reprodução
  no destino móvel; serial, pseudoterminais e USB desktop ficam desativados.
- Preparar e validar a compilação de simulador; a compilação concluída é
  exclusivamente para dispositivos arm64.
- Implementar permissões de microfone e rede local, configuração de sessão de
  áudio, interrupções, mudanças de rota e recuperação após suspensão.
- Garantir encerramento de transmissão ao perder a sessão e validar esse
  comportamento com rádio real antes de habilitar transmissão ao usuário.
- Empacotar definições de rádios, usar diretórios graváveis do sandbox e adaptar
  as janelas para toque, teclado virtual, orientação e tamanhos de iPhone/iPad.
- Configurar assinatura e executar testes em dispositivos. O pacote atual
  não é instalável sem certificado e perfil de provisionamento.

## Critérios de aceitação

Compilação e instalação em dispositivo e simulador; conexão e reconexão de rede;
recepção, espectro, waterfall, seleção de rádio e persistência das configurações;
captura e transmissão com controle seguro de PTT; negação de permissões;
interrupções de áudio, troca de saída, suspensão e retomada; ausência de regressões
nos destinos desktop. Não foi possível executar esses testes nesta etapa.

As alterações são arquivos versionáveis comuns; não houve publicação no GitHub.

## Instalação em outro volume

Para instalar as dependências no disco USB 1TbSilver, execute:

```sh
WFVIEW_QT_ROOT="/Volumes/1TbSilver/wfview-ios" sh scripts/install-ios-qt.sh
```

O script mantém SDKs, ambiente Python e arquivos de download no volume escolhido.
Pequenos arquivos de controle e sockets permanecem em `/private/tmp`, pois SMB
não suporta os sockets Unix usados pelo instalador.
Esse volume precisa estar montado durante instalação e compilação.
A instalação das dependências não significa que a portabilidade foi concluída.

## Compilação independente do codec

O codec Opus pode ser compilado enquanto o Qt é instalado:

```sh
WFVIEW_QT_ROOT="/Volumes/1TbSilver/wfview-ios" sh scripts/download-ios-deps.sh
WFVIEW_QT_ROOT="/Volumes/1TbSilver/wfview-ios" sh scripts/build-ios-opus.sh iphoneos
WFVIEW_QT_ROOT="/Volumes/1TbSilver/wfview-ios" sh scripts/build-ios-opus.sh iphonesimulator
WFVIEW_QT_ROOT="/Volumes/1TbSilver/wfview-ios" sh scripts/prepare-ios-headers.sh
```

As bibliotecas de dispositivo e simulador ficam separadas em `install/`,
por plataforma e arquitetura. O destino mínimo desse script é iOS 16.
A compilação isolada do codec não valida a aplicação inteira.

## Compilação para simulador

Os scripts aceitam um destino separado, sem sobrescrever a compilação de
aparelho físico:

```sh
sh scripts/build-ios-opus.sh iphonesimulator
WFVIEW_IOS_PLATFORM=iphonesimulator sh scripts/build-ios.sh
```

O Qt 6.8.3 pré-compilado deste ambiente usa `x86_64` para simulador e `arm64`
para aparelhos físicos. Por isso, o padrão dos scripts de simulador é `x86_64`.
É necessário um runtime universal com suporte a essa arquitetura (por exemplo,
iOS 26.0) e Rosetta em Macs Apple Silicon. O runtime iOS 27.1 arm64 instalado
não executa esse binário. `WFVIEW_SIM_ARCH` permite selecionar outra arquitetura
somente quando todas as bibliotecas Qt também foram compiladas para ela.

O projeto gerado fica em `build/wfview-simulator/wfview.xcodeproj`, dentro de
`WFVIEW_QT_ROOT`; o aplicativo fica em `Release-iphonesimulator/wfview.app`.
No Xcode 27, a interface dos simuladores é o Device Hub, disponível em
`Xcode > Open Developer Tool > Device Hub`.
