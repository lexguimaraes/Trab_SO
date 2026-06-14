# Explicação Do Simulador

Este documento explica o funcionamento do programa e a organização do código.
Ele complementa o `README.md`, que fica mais focado em build e uso rápido.

## O Que O Programa Simula

O simulador modela, de forma discreta, um sistema com:

- 4 CPUs simuladas;
- 4 discos simulados;
- 32 GiB de memória principal, representados como 32768 MiB;
- processos de tempo real, prioridade `0`;
- processos de usuário, prioridade `1`;
- escalonamento por múltiplas filas de feedback para processos de usuário;
- quantum de 2 unidades de tempo para processos de usuário;
- fases de execução `CPU1`, `IO` e `CPU2`;
- reserva exclusiva de discos solicitados pelos processos;
- alocação real de memória por blocos, com endereço base e tamanho;
- logs de criação, despacho, bloqueio, preempção, finalização e mapa de memória;
- relatório HTML opcional em `resultado.html`.

O tempo avança em unidades inteiras. Em cada unidade de tempo, o simulador admite
novos processos, inicia I/O em discos livres, despacha processos para CPUs livres,
imprime o estado atual e depois consome uma unidade de CPU/I/O dos processos em
execução.

## Build E Execução

Para compilar:

```sh
make
```

Para executar o exemplo em modo single-thread:

```sh
make run
```

Esse alvo usa `examples/processos.txt`, uma carga de validação com 50 processos.
Para rodar o exemplo pequeno:

```sh
make run_basic
```

Para executar o exemplo e gerar o relatório HTML:

```sh
make report
```

Para gerar o relatório do exemplo pequeno:

```sh
make report_basic
```

Ou diretamente:

```sh
./simulador examples/processos.txt
```

Ou diretamente com relatório:

```sh
./simulador examples/processos.txt --html resultado.html
```

## Comandos Make

```sh
make
```

Compila o projeto e gera o executável `simulador`. Também atualiza
`compile_commands.json` antes do build.

```sh
make simulador
```

Compila diretamente o alvo do binário. Na prática, hoje tem o mesmo efeito de
`make`.

```sh
make run
```

Compila o projeto, se necessário, e executa o simulador usando
`examples/processos.txt`, uma carga de validação com 50 processos.

```sh
make run_basic
```

Executa o caso pequeno em `examples/processos_basico.txt`.

```sh
make report
```

Compila o projeto, se necessário, executa o simulador usando
`examples/processos.txt` e gera o relatório `resultado.html`.

```sh
make report_basic
```

Gera o relatório HTML para o caso pequeno em `examples/processos_basico.txt`.

```sh
make compile_commands
```

Regenera apenas o `compile_commands.json`, usado pelo `clangd`/LSP para entender
as flags de compilação do projeto.

O arquivo gerado inclui entradas para:

- `src/*.c`;
- `include/*.h`, com `-x c-header`.

```sh
make clean
```

Remove o binário `simulador` e os arquivos objeto `src/*.o`.

## Entrada

O parser aceita o formato oficial descrito no enunciado:

```text
[id, cpu1, io, cpu2, memoria_mb]
```

Exemplo:

```text
[7, 4, 2, 4, 800]
```

Nesse formato:

- o processo é tratado como processo de usuário;
- a prioridade padrão é `1`;
- a chegada padrão é no tempo `0`;
- se `io > 0`, o processo usa 1 disco durante a fase de I/O.

Para testes mais completos, também são aceitos dois formatos estendidos:

```text
id prioridade cpu1 io cpu2 memoria_mb
id prioridade chegada cpu1 io cpu2 memoria_mb discos
```

Exemplos:

```text
21 0 3 0 0 512
42 1 5 8 3 6 1024 1
```

Prioridades:

- `0`: processo de tempo real;
- `1`: processo de usuário.

No formato estendido, o campo `discos` representa a quantidade de discos
reservados exclusivamente pelo processo durante toda a execução. Um processo com
I/O precisa solicitar ao menos 1 disco. A admissão de processos espera até haver
memória e discos suficientes.

Processos de tempo real são validados com limite máximo de 512 MiB de memória e
não podem solicitar I/O, segunda fase de CPU ou discos.

## Saída

A saída padrão no terminal é um log textual da simulação. Um trecho típico:

```text
-------------------------------------------------------------------------------
Ciclo 0: t=000 -> t=001
-------------------------------------------------------------------------------
  t=000 | estado    | P7   | NEW        -> READY      | fase=CPU1 restante=4
  t=000 | criacao   | P7   | base=512   tamanho=800   MiB discos=1 prioridade=USER
  t=000 | estado    | P7   | READY      -> RUNNING    | fase=CPU1 restante=4
  t=000 | despacho  | P7   | cpu2 fila=0 quantum=2
```

O simulador também imprime, a cada unidade de tempo:

- qual processo está em cada CPU;
- qual processo está em cada disco;
- o mapa atual da memória.

Exemplo de mapa de memória:

```text
Memoria | base=0     tam=512   P5   | base=512   tam=800   P7   | base=1312  tam=1200  P12  | base=2512  tam=30256 livre |
```

Cada bloco mostra:

- endereço base;
- tamanho em MiB;
- processo dono ou `livre`.

## Relatório HTML

Além do log no terminal, o simulador pode gerar um relatório HTML interativo:

```sh
./simulador examples/processos.txt --html resultado.html
```

O alvo equivalente do `Makefile` é:

```sh
make report
```

O arquivo `resultado.html` é autocontido: ele inclui HTML, CSS e JavaScript no
próprio arquivo, sem depender de servidor, framework web ou biblioteca externa.

O relatório mostra:

- cabeçalho com quantidade de processos, CPUs, discos, memória e quantum;
- controles para avançar/voltar ciclo, reproduzir automaticamente, alterar a
  velocidade, usar slider de ciclo e alternar entre um ciclo por vez ou todos;
- uma seção por ciclo da simulação;
- tabela de eventos por ciclo;
- snapshot das CPUs;
- snapshot dos discos;
- tamanho e PIDs das filas;
- barra visual da memória por blocos;
- tabela final com os processos e seus estados finais.

Esse relatório existe porque o terminal fica difícil de ler quando a simulação
tem muitos ciclos. O terminal continua útil para depuração rápida, enquanto o
HTML é melhor para apresentação e inspeção visual.

## Regras De Escalonamento

### Processos De Tempo Real

Processos com prioridade `0` entram na fila de tempo real.

Eles têm precedência sobre processos de usuário e são escalonados em ordem FCFS.
No modo atual, eles não usam quantum. Depois que entram em uma CPU, executam até
terminar a fase de CPU atual. Se um processo de tempo real chega enquanto todas
as CPUs estão ocupadas por usuários, um usuário é preemptado no limite do ciclo
para liberar CPU.

O usuário preemptado é escolhido pelo nível de feedback: entre os usuários em
execução, sai primeiro quem tiver o maior `feedback_level`, ou seja, a menor
prioridade entre usuários. Em caso de empate, o simulador escolhe a CPU de maior
índice para manter o comportamento determinístico.

### Processos De Usuário

Processos com prioridade `1` usam múltiplas filas de feedback:

- fila `0`: maior prioridade entre usuários;
- fila `1`: prioridade intermediária;
- fila `2`: menor prioridade entre usuários.

O quantum é 2. Quando um processo de usuário consome todo o quantum sem terminar
a fase de CPU, ele sofre preempção e desce uma fila, até o limite da fila `2`.

Quando um processo termina `CPU1`:

- se tiver `IO > 0`, vai para a fila de I/O;
- se não tiver I/O e tiver `CPU2 > 0`, volta para a fila de prontos;
- se não tiver mais trabalho, finaliza.

Quando termina I/O:

- se tiver `CPU2 > 0`, volta para a fila de prontos;
- caso contrário, finaliza.

## Memória

A memória é implementada em `src/memory.c`.

## Recursos De Disco

Discos são tratados em dois níveis:

- reserva: feita na admissão, usando o campo `discos` do descritor estendido;
- atividade de I/O: mostrada nos slots `d0` a `d3` enquanto o processo está na
  fase de I/O.

A reserva impede que mais de 4 discos sejam comprometidos ao mesmo tempo, mesmo
quando alguns processos reservados ainda não estão executando I/O naquele ciclo.

O gerenciador mantém uma lista dinâmica de blocos:

```c
typedef struct {
    int base;
    int size;
    int pid;
    int free;
} MemoryBlock;
```

A alocação usa first-fit:

1. percorre os blocos em ordem;
2. encontra o primeiro bloco livre com tamanho suficiente;
3. ocupa o bloco inteiro se o tamanho for exato;
4. ou divide o bloco em uma parte ocupada e outra livre.

Ao liberar memória, o gerenciador junta blocos livres vizinhos para reduzir
fragmentação externa.

## Organização Do Código

```text
.
├── Makefile
├── README.md
├── EXPLICACAO.md
├── examples/
│   ├── processos.txt
│   └── processos_basico.txt
├── include/
│   ├── memory.h
│   ├── process.h
│   ├── process_queue.h
│   └── simulator.h
└── src/
    ├── main.c
    ├── memory.c
    ├── process.c
    ├── process_queue.c
    └── simulator.c
```

### `src/main.c`

Responsável pela interface de linha de comando.

Ele valida o formato:

```sh
./simulador <arquivo> [--html resultado.html]
```

Lê o caminho de entrada e o caminho opcional do relatório HTML e chama
`simulator_run`.

### `include/simulator.h`

Define constantes globais do sistema:

```c
#define SYSTEM_CPU_COUNT 4
#define SYSTEM_DISK_COUNT 4
#define SYSTEM_MEMORY_MB (32 * 1024)
#define USER_QUANTUM 2
```

Também declara a função pública:

```c
int simulator_run(const char *input_path, const char *html_path);
```

### `include/process.h` E `src/process.c`

Definem o modelo de processo e o parser do arquivo de entrada.

O struct `Process` guarda tanto os dados originais do descritor quanto o estado
mutável da simulação:

- `id`;
- `priority`;
- `arrival_time`;
- tempos de `CPU1`, `IO` e `CPU2`;
- memória solicitada;
- discos solicitados e reservados;
- estado atual;
- fase atual;
- tempo restante da fase;
- quantum restante;
- nível atual na fila de feedback;
- endereço base na memória.

Estados possíveis:

```text
NEW
READY
RUNNING
BLOCKED_IO
FINISHED
```

Fases possíveis:

```text
CPU1
IO
CPU2
DONE
```

O parser também:

- ignora linhas vazias;
- ignora comentários iniciados com `#`;
- aceita colchetes e vírgulas no formato oficial;
- ordena os processos por tempo de chegada e, em empate, por `id`.

### `include/process_queue.h` E `src/process_queue.c`

Implementam uma fila simples de ponteiros para `Process`.

Principais operações:

```c
int queue_push(ProcessQueue *queue, Process *process);
Process *queue_pop(ProcessQueue *queue);
int queue_empty(const ProcessQueue *queue);
```

A fila é usada para:

- fila de tempo real;
- três filas de usuário;
- fila de processos aguardando I/O.

### `include/memory.h` E `src/memory.c`

Implementam o gerenciador de memória por blocos.

Principais funções:

```c
void memory_init(MemoryManager *memory, int total_size);
int memory_allocate(MemoryManager *memory, int pid, int size);
void memory_release(MemoryManager *memory, int pid);
void memory_print(const MemoryManager *memory);
```

### `src/simulator.c`

Contém o núcleo do modo single-thread.

A estrutura principal é `SingleThreadSystem`, que agrupa:

- fila de tempo real;
- três filas de usuário;
- fila de I/O;
- 4 slots de CPU;
- 4 slots de disco;
- gerenciador de memória;
- contador de discos reservados;
- ponteiro opcional para o relatório HTML;
- contador de processos finalizados.

O `Dispatcher` agrupa a lógica de despacho do simulador. Ele não é uma thread
separada; é uma estrutura que aponta para o estado do sistema e para a lista de
processos carregados. Suas funções concentram as decisões de admissão,
preempção por tempo real, início de I/O e despacho para CPU:

- `dispatcher_admit_processes`: admite processos que já chegaram, reservando
  memória e discos antes de colocá-los na fila de prontos;
- `dispatcher_preempt_for_real_time`: libera CPU para tempo real, preemptando o
  usuário em execução com menor prioridade de feedback;
- `dispatcher_dispatch_cpus`: move processos prontos para CPUs livres;
- `dispatcher_start_io`: move processos aguardando I/O para discos livres.

O loop principal fica em `run_single`:

```c
while (system.finished_count < (int)processes->count) {
    html_cycle_begin(system.report, time);
    dispatcher_admit_processes(&dispatcher, time);
    dispatcher_start_io(&dispatcher, time);
    dispatcher_preempt_for_real_time(&dispatcher, time);
    dispatcher_dispatch_cpus(&dispatcher, time);
    print_resources(&system, time);
    print_queues(&system);
    memory_print(&system.memory);
    tick_cpus(&dispatcher, time);
    tick_disks(&dispatcher, time);
    html_snapshot(system.report, &system, time + 1);
    html_cycle_end(system.report);
    time++;
}
```

Em termos de simulação, cada iteração representa uma unidade de tempo.

O relatório HTML é implementado no próprio `src/simulator.c`, por funções
auxiliares como:

- `html_write_header`;
- `html_cycle_begin`;
- `html_event`;
- `html_snapshot`;
- `html_cycle_end`;
- `html_write_footer`.

Essas funções recebem um `HtmlReport`, que contém o `FILE *` aberto para o
arquivo de saída. Quando `html_path == NULL`, os ponteiros de relatório ficam
nulos e a simulação executa apenas com saída no terminal.

## Fluxo De Um Processo

O fluxo geral é:

```text
NEW
  |
  | chegada + memória e discos disponíveis
  v
READY
  |
  | CPU livre + escalonador escolhe o processo
  v
RUNNING
  |
  | termina CPU1 e tem I/O
  v
BLOCKED_IO
  |
  | disco termina I/O
  v
READY
  |
  | CPU livre
  v
RUNNING
  |
  | termina CPU2 ou não tem mais fases
  v
FINISHED
```

Processos de usuário podem voltar de `RUNNING` para `READY` quando o quantum
acaba.

## Threads

O enunciado faz uma pergunta dissertativa:

> "Em relação ao simulador, como este poderia ser beneficiado com a
> implementação do simulador com mais de uma thread?"

O simulador desta entrega é single-thread e avança o tempo de forma discreta: a
cada unidade de tempo o laço principal admite processos, inicia I/O, despacha
CPUs e consome uma unidade de cada recurso ocupado. Tudo isso é serializado em
um único fluxo de execução.

O simulador poderia se beneficiar de múltiplas threads das seguintes formas:

- **Paralelismo real dos recursos.** Cada uma das 4 CPUs e cada um dos 4 discos
  poderia ser uma thread independente, executando de verdade ao mesmo tempo, em
  vez de o laço único processar um recurso após o outro a cada tique. Em uma
  máquina multicore isso aproveitaria os núcleos físicos e modelaria com mais
  fidelidade o fato de o sistema ter 4 CPUs e 4 discos operando em paralelo.
- **Dispatcher reativo.** Uma thread escalonadora poderia dormir enquanto não há
  trabalho e ser acordada por semáforos ou variáveis de condição quando um
  processo chega à fila de prontos ou quando um recurso fica livre, eliminando a
  varredura ativa feita a cada unidade de tempo.
- **I/O verdadeiramente bloqueante.** O bloqueio em disco deixaria de ser
  simulado por contagem de tiques e passaria a ser um bloqueio real da thread,
  liberando naturalmente a CPU para outro processo.

O custo é a necessidade de sincronização: as filas de prontos, a fila de I/O, o
gerenciador de memória e o estado de cada processo passam a ser recursos
compartilhados e precisam ser protegidos por **mutexes** para evitar condições de
corrida. A saída textual também precisaria ser serializada para não embaralhar
as mensagens das várias threads. Por isso a versão atual ficou single-thread:
ela é mais simples de raciocinar e produz uma saída determinística, o que
facilita a validação e a correção do trabalho.
