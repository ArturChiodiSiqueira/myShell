#include <stdio.h>      // Para entrada e saída (printf, fgets)
#include <stdlib.h>     // Para alocação de memória e exit (malloc, free, exit, getenv)
#include <string.h>     // Para manipulação de strings (strtok, strcmp, strcspn)
#include <unistd.h>     // Para chamadas de sistema POSIX (fork, exxecvp, chdir, getcwd, pipe, dup2)
#include <sys/wait.h>   // Para a chamada wait() e waitpid()
#include <sys/types.h>  // Necessário para open()
#include <sys/stat.h>   // Necessário para open()
#include <fcntl.h>      // Para open() e as flags (O_RDONLY, O_WRONLY, ...)
#include <signal.h>     // Para signal() e o handler de SIGCHLD

// Define as cores ANSI para o prompt
#define ANSI_COLOR_YELLOW  "\x1b[33m"  // Amarelo
#define ANSI_COLOR_RESET   "\x1b[0m"   // Reseta para a cor padrão

// Define limites do shell
#define MAX_LINE 80   // Tamanho máximo da linha de comando
#define MAX_ARGS 20   // Número máximo de argumentos

/**
 * @brief Handler para o sinal SIGCHLD.
 * Esta função é chamada automaticamente pelo kernel quando um processo filho termina.
 * Ela "limpa" os processos zumbis criados pela execução em background (&).
 */
void handle_sigchld(int sig) {
    // waitpid com WNOHANG não bloqueia (non-blocking).
    // O loop garante limpeza de todos os filhos que possam ter terminado.
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

/**
 * @brief Quebra a linha de comando em tokens (argumentos).
 * @param line A linha de comando lida do usuário.
 * @param args O array de strings (ponteiros) onde os argumentos serão armazenados.
 */
void parse_comando(char *line, char **args)
{
    int i = 0;
    
    // Remove o caractere de nova linha (\n) que o fgets pode deixar
    line[strcspn(line, "\n")] = 0; 

    // Usa strtok para dividir a string em tokens baseados em espaços
    char *token = strtok(line, " "); 
    while (token != NULL && i < MAX_ARGS - 1)
    {
        args[i] = token;
        i++;
        token = strtok(NULL, " ");
    }
    
    // O último elemento do array de argumentos deve ser NULL para o execvp
    args[i] = NULL; 
}

/**
 * @brief Executa um comando simples (sem pipes).
 * Lida com redirecionamento de I/O e execução em background.
 */
void executar_comando_simples(char **args, int background, char *arquivo_entrada, char *arquivo_saida, int modo_append) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("MyShell: Erro no fork");
        return;
    }

    if (pid == 0) {
        // CÓDIGO DO PROCESSO FILHO
        // 1 Redirecionamento de Entrada (<)
        if (arquivo_entrada != NULL) {
            // Abre o arquivo para leitura
            int fd_in = open(arquivo_entrada, O_RDONLY);
            if (fd_in < 0) {
                perror("MyShell: Erro ao abrir arquivo de entrada");
                exit(EXIT_FAILURE);
            }
            // dup2 religa o descritor de arquivo STDIN_FILENO (0) para apontar para fd_in
            dup2(fd_in, STDIN_FILENO); 
            close(fd_in);
        }

        // 2 Redirecionamento de Saída ( > ou >> )
        if (arquivo_saida != NULL) {
            // Definindo as flags de abertura do arquivo
            int flags = O_WRONLY | O_CREAT;
            if (modo_append) {
                flags |= O_APPEND; // Modo append (>>)
            } else {
                flags |= O_TRUNC;  // Modo sobrescrita (>)
            }
            // 0644 são as permissões de arquivo (rw-r--r--)
            int fd_out = open(arquivo_saida, flags, 0644);
            if (fd_out < 0) {
                perror("MyShell: Erro ao abrir arquivo de saída");
                exit(EXIT_FAILURE);
            }
            // dup2 religa o descritor de arquivo STDOUT_FILENO (1) para apontar para fd_out
            dup2(fd_out, STDOUT_FILENO); 
            close(fd_out);
        }

        // 3 Executa o comando
        // execvp procura o comando no PATH e substitui o processo filho
        if (execvp(args[0], args) == -1) {
            perror("MyShell");
            exit(EXIT_FAILURE);
        }
    } else {
        // CÓDIGO DO PROCESSO PAI
        if (!background) {
            // Se NÃO for background, espera o filho terminar
            waitpid(pid, NULL, 0);
        } else {
            // Se FOR background, imprime o PID do filho e continua
            printf("Comando [PID %d] iniciado em background.\n", pid);
        }
    }
}

/**
 * @brief Executa um comando com um pipe (cmd1 | cmd2).
 * 
 */
void executar_pipe(char **cmd1_args, char **cmd2_args) {
    int pipe_fd[2]; // [0] = ponta de leitura (read-end), [1] = ponta de escrita (write-end)
    pid_t pid1, pid2;

    if (pipe(pipe_fd) < 0) {
        perror("MyShell: Erro ao criar pipe");
        return;
    }

    // Primeiro Filho (Executa cmd1, escreve no pipe)
    pid1 = fork();
    if (pid1 < 0) {
        perror("MyShell: Erro no fork (pid1)");
        return;
    }

    if (pid1 == 0) {
        // primeiro filho (cmd1)
        close(pipe_fd[0]); // Fecha a ponta de leitura (não usada por ele)
        
        // Redireciona STDOUT (1) para a ponta de escrita do pipe
        dup2(pipe_fd[1], STDOUT_FILENO);
        
        close(pipe_fd[1]); // Fecha a ponta de escrita (agora duplicada)
        
        // Executa o primeiro comando
        if (execvp(cmd1_args[0], cmd1_args) == -1) {
            perror("MyShell (cmd1)");
            exit(EXIT_FAILURE);
        }
    }

    // Segundo Filho (Executa cmd2, lê do pipe)
    pid2 = fork();
    if (pid2 < 0) {
        perror("MyShell: Erro no fork (pid2)");
        return;
    }

    if (pid2 == 0) {
        // segundo filho (cmd2)
        close(pipe_fd[1]); // Fecha a ponta de escrita (não usada por ele)
        
        // Redireciona STDIN (0) para a ponta de leitura do pipe
        dup2(pipe_fd[0], STDIN_FILENO);
        
        close(pipe_fd[0]); // Fecha a ponta de leitura (agora duplicada)
        
        // Executa o segundo comando
        if (execvp(cmd2_args[0], cmd2_args) == -1) {
            perror("MyShell (cmd2)");
            exit(EXIT_FAILURE);
        }
    }

    // Processo Pai
    // O pai deve fechar AMBAS as pontas do pipe, senão os filhos podem travar
    close(pipe_fd[0]);
    close(pipe_fd[1]);

    // Espera ambos os filhos terminarem
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
}

/**
 * @brief Analisa os argumentos em busca de operadores (| < > >> &)
 * e chama a função de execução correta.
 */
void analisar_e_executar(char **args) {
    int background = 0;
    char *arquivo_saida = NULL;
    char *arquivo_entrada = NULL;
    int modo_append = 0;
    int indice_pipe = -1;

    // 1 Verificar se há um pipe
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "|") == 0) {
            indice_pipe = i;
            args[i] = NULL; // Divide o comando em dois neste ponto
            break; // Suporta apenas um pipe, como pedido
        }
    }

    if (indice_pipe != -1) {
        // Se encontrar um pipe, executa e retorna.
        // cmd1 é 'args', cmd2 é '&args[indice_pipe + 1]'
        executar_pipe(args, &args[indice_pipe + 1]);
        return;
    }

    // 2 Se não houver pipe, verifica redirecionamento e background
    // Novo array para compactar os argumentos (remover os operadores)
    char *args_simples[MAX_ARGS];
    int arg_index = 0;
    
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "<") == 0) {
            arquivo_entrada = args[i+1];
            i++; // Pula o nome do arquivo
        } else if (strcmp(args[i], ">") == 0) {
            arquivo_saida = args[i+1];
            modo_append = 0;
            i++; // Pula o nome do arquivo
        } else if (strcmp(args[i], ">>") == 0) {
            arquivo_saida = args[i+1];
            modo_append = 1;
            i++; // Pula o nome do arquivo
        } else if (strcmp(args[i], "&") == 0) {
            // O '&' deve ser o último token
            if (args[i+1] == NULL) {
                background = 1;
            }
            break; // '&' é sempre o fim
        } else {
            // É um argumento normal (ex: "ls", "-l")
            args_simples[arg_index++] = args[i];
        }
    }
    args_simples[arg_index] = NULL; // Termina array de args

    // 3 Executa o comando simples com todos os parâmetros encontrados
    executar_comando_simples(args_simples, background, arquivo_entrada, arquivo_saida, modo_append);
}

/**
 * @brief Função principal do MyShell
 */
int main() {
    char *args[MAX_ARGS]; // Array para armazenar os argumentos
    char line[MAX_LINE];  // Buffer para a linha de comando

    // Configura o handler para SIGCHLD (limpar zumbis)
    // Isso é essencial para comandos em background (&)
    signal(SIGCHLD, handle_sigchld);

    // Loop principal do shell
    while (1)
    {
        // Exibe o prompt customizável
        printf(ANSI_COLOR_YELLOW "MyShell> " ANSI_COLOR_RESET);
        fflush(stdout); 

        // Lê a linha de comando
        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\nEncerrando MyShell...\n");
            break; // Ctrl+D
        }

        // Analisa a linha em tokens
        parse_comando(line, args); 

        // Se nenhum comando foi digitado, continua o loop
        if (args[0] == NULL) {
            continue;
        }

        // Comandos Embutidos (Built-in)
        // Estes comandos rodam no próprio processo do shell

        // Verificação do comando exit
        if (strcmp(args[0], "exit") == 0) {
            printf("Encerrando MyShell...\n");
            break; // Sai do loop while(1)
        }
        
        // Verificação do comando cd
        if (strcmp(args[0], "cd") == 0) {
            if (args[1] == NULL) {
                // cd sozinho vai para o HOME
                if (chdir(getenv("HOME")) != 0) {
                    perror("MyShell: cd (HOME)");
                }
            } else {
                // cd [caminho]
                if (chdir(args[1]) != 0) {
                    perror("MyShell: cd");
                }
            }
            // cd embutido, então pula para a próxima iteração
            continue; 
        }

        // Execução de Comandos Externos
        // A lógica de fork/exec foi movida para as novas funções
        analisar_e_executar(args);
    }
    return 0;
}
