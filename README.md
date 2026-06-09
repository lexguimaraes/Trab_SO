# Simulador de Escalonamento

Trabalho prático de Sistemas Operacionais: simulador de processos com CPUs,
discos, memória principal, filas de prontos e modos de execução single-thread e
multi-thread.

## Build

```sh
make
```

## Execução

```sh
./simulador --modo single examples/processos.txt
```

O modo `multi` será implementado usando os mesmos modelos do modo `single`,
com threads reais representando componentes do sistema, não processos do
usuário.
