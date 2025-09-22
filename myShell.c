#include <stdio.h>    //Para entrada e saída (printf, fgets)
#include <stdlib.h>   //Para alocação de memória e exit (malloc, free, exit, getenv)
#include <string.h>   //Para manipulação de strings (strtok, strcmp, strcspn)
#include <unistd.h>   //Para chamadas de sistema POSIX (fork, execvp, chdir, getcwd)
#include <sys/wait.h> //Para a chamada wait()

#define ANSI_COLOR_YELLOW   "\x1b[33m" //Para deixar em amarelo
#define ANSI_COLOR_RESET   "\x1b[0m"   //Para voltar ao normal

#define MAX_LINE 80   //Tamanho máximo da linha de comando
#define MAX_ARGS 20   //Número máximo de argumentos

/**
 * Função para analisar a linha de comando em tokens (argumentos).
 * @param line A linha de comando lida do usuário.
 * @param args O array de strings onde os argumentos serão armazenados.
 */
void parse_comando(char *line, char **args)
{
    int i = 0;

    line[strcspn(line, "\n")] = 0; //Remove o caractere de nova linha (\n) que o fgets pode deixar

    char *token = strtok(line, " "); //Usa strtok para dividir a string em tokens baseados em espaços
    while (token != NULL && i < MAX_ARGS - 1)
    {
        args[i] = token;
        i++;
        token = strtok(NULL, " ");
    }

    args[i] = NULL; //O último elemento do array de argumentos deve ser NULL para o execvp
}

int main() {
    char *args[MAX_ARGS]; //Array para armazenar os argumentos do comando
    char line[MAX_LINE];  //Buffer para a linha de comando inserida
    int status;           //Status de saída do processo filho

    while (1)
    {
        //Exibe o prompt customizável usando as cores definidas
        printf(ANSI_COLOR_YELLOW "MyShell> " ANSI_COLOR_RESET);
        fflush(stdout); //Garante que o prompt seja exibido

        //Le a linha de comando
        if (fgets(line, sizeof(line), stdin) == NULL) //Se fgets retornar NULL (ex: Ctrl+D), encerra o shell
            break;

        parse_comando(line, args); //Analisa (parse) a linha de comando

        //Se nenhum comando foi digitado, continua o loop
        if (args[0] == NULL)
            continue;

        //Comandos Embutidos
        if (strcmp(args[0], "exit") == 0) //Verificação do comando "exit"
        {
            printf("Encerrando MyShell...\n");
            break; //Sai do loop while(1) e termina o programa
        }
        
        //Verificação do comando "cd"
        if (strcmp(args[0], "cd") == 0) //Se for "cd" sem argumentos, muda para o diretório HOME
        {
            if (args[1] == NULL) {
                if (chdir(getenv("HOME")) != 0)
                    perror("MyShell: cd (HOME)");
            } else {
                if (chdir(args[1]) != 0) //Muda para o diretório especificado
                    perror("MyShell: cd");
            }
            continue; //Como "cd" é embutido, voltamos ao início do loop
        }

        //Execução de Programas Externos
        pid_t pid = fork();

        if (pid < 0)
            perror("MyShell: Erro no fork"); //Erro ao criar o processo filho
        
        else if (pid == 0)
        {
            //Código do PROCESSO FILHO
            if (execvp(args[0], args) == -1)
            {
                perror("MyShell");
                exit(EXIT_FAILURE); //Se execvp falhar, o processo filho deve terminar para não continuar executando o código do shell.
            }

        } else {
            //Código do PROCESSO PAI
            wait(&status); //O pai espera o processo filho terminar
        }
    }
    return 0;
}