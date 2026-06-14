# Simulador de Escalonamento

Trabalho prático de Sistemas Operacionais: simulador de processos com CPUs,
discos, memória principal e filas de prontos.

## Build

```sh
make
```

O build também regenera automaticamente `compile_commands.json` para
`clangd`/LSP. Para atualizar apenas esse arquivo:

```sh
make compile_commands
```

## Comandos Make

```sh
make
```

Compila o projeto e gera o executavel `simulador`. Tambem atualiza
`compile_commands.json` antes do build.

```sh
make simulador
```

Compila diretamente o alvo do binario. Na pratica, hoje tem o mesmo efeito de
`make`.

```sh
make run
```

Compila o projeto, se necessario, e executa o simulador usando
`examples/processos.txt`, que contem uma carga significativa de validacao.

```sh
make run_basic
```

Executa o caso pequeno em `examples/processos_basico.txt`.

```sh
make run_official
```

Executa uma carga grande com prioridade explícita em
`examples/processos_oficial.txt`.

```sh
make report
```

Compila o projeto, se necessario, executa o simulador e gera o
relatorio HTML interativo `resultado.html`.

```sh
make report_basic
```

Gera o relatorio HTML para o caso pequeno em `examples/processos_basico.txt`.

```sh
make report_official
```

Gera o relatorio HTML para a carga grande com prioridade explícita.

```sh
make compile_commands
```

Regenera apenas o `compile_commands.json`, usado pelo `clangd`/LSP para
entender flags como `-std=c11` e `-Iinclude`.

```sh
make clean
```

Remove o binario `simulador` e os arquivos objeto `src/*.o`.

## Execução

```sh
./simulador examples/processos.txt
```

Para gerar um relatorio HTML:

```sh
./simulador examples/processos.txt --html resultado.html
```

O relatório HTML é autocontido e permite navegar ciclo a ciclo, reproduzir a
simulação automaticamente, ajustar a velocidade e alternar entre um ciclo por
vez ou todos os ciclos.

## Entrada

O parser aceita o formato oficial descrito no enunciado:

```text
[id, cpu1, io, cpu2, memoria_mb]
```

Nesse formato, o processo entra como usuário, chega no tempo 0 e usa 1 disco
durante I/O quando `io > 0`.

Para testes mais completos, também são aceitos:

```text
id prioridade cpu1 io cpu2 memoria_mb
id prioridade chegada cpu1 io cpu2 memoria_mb discos
```

Prioridade `0` representa tempo real; prioridade `1` representa usuário.

O arquivo `examples/processos_oficial.txt` usa o primeiro formato estendido,
com prioridade explícita, chegada padrão em `0` e discos inferidos pelo valor de
`io`.

No formato estendido, `discos` representa recursos reservados pelo despachante
durante toda a vida do processo. Um processo com I/O deve solicitar ao menos 1
disco. Processos de tempo real usam apenas CPU e memoria, com limite de 512 MiB.

Chegada e admissão são momentos diferentes. A chegada indica quando o processo
passa a ser candidato; a admissão só ocorre quando há memória e discos
suficientes para reservar. Enquanto isso não acontece, o processo permanece em
`NEW` e aparece na fila `admissao` dos snapshots com o motivo da espera. No HTML,
`proximo-ciclo` indica que os recursos já foram liberados no fim do ciclo e a
admissão será reavaliada no início do próximo.

A admissão é ordenada por chegada e, em empate, por `id`, mas não é FIFO
bloqueante: se um processo pendente não couber nos recursos disponíveis, outro
processo posterior que couber pode ser admitido.

## Estado Atual

Implementado:

- 4 CPUs simuladas;
- 4 discos simulados;
- processos de tempo real em FCFS sem quantum;
- processos de usuário com feedback de 3 filas e quantum 2;
- preempção de usuário quando tempo real chega e todas as CPUs estão ocupadas
  por usuários;
- reserva exclusiva de discos durante a execução dos processos que os solicitaram;
- memória por blocos com first-fit e junção de blocos livres;
- logs de admissão, despacho, bloqueio, preempção e finalização.

## Threads

O enunciado pergunta como o simulador poderia se beneficiar de mais de uma
thread. A resposta está detalhada em [`EXPLICACAO.md`](EXPLICACAO.md), na seção
"Threads". Em resumo: cada CPU e cada disco poderiam virar uma thread
independente, executando em paralelo de verdade, com uma thread escalonadora
acordando os recursos ociosos via semáforos. Isso aproveitaria máquinas
multicore e modelaria o paralelismo real dos 4 CPUs e 4 discos, ao custo de
sincronizar as filas, a memória e o estado dos processos com mutexes. A versão
atual é single-thread e discreta por simplicidade e determinismo da saída.
