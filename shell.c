#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdbool.h>

#define LSH_RL_BUFSIZE 1024
#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"

void lsh_loop(void);
char* lsh_read_line(void);
char** lsh_split_line(char* line);
int lsh_launch(char** args);

// Function Declarations for builtin shell commands

int lsh_cd(char** args);
int lsh_help(char** args);
int lsh_exit(char** args);


int lsh_num_builtins();
int lsh_execute(char** args);



int main(char* args, char** argv) {

	lsh_loop();

	return EXIT_SUCCESS;
};

void lsh_loop(void) {
	char* line;
	char** args;
	int status;

	do {
		uid_t uid = getuid();
		struct passwd *pw = getpwuid(uid);

		printf("%s> ", pw->pw_name);
		line = lsh_read_line();
		args = lsh_split_line(line);
		status = lsh_execute(args);

		free(line);
		free(args);
	}
	while (status);
}

char* lsh_read_line(void) {
	int bufsize = LSH_RL_BUFSIZE;
	int position = 0;
	char* buffer = malloc(sizeof(char)*bufsize);
	int c;

	if (!buffer) {
		fprintf(stderr, "lsh: allocation error\n");
		exit(EXIT_FAILURE);
	}
	// Read a character
	while (true) {
		c = getchar();

		// if we hit EOF, replace it with a null character and return
		if (c == EOF || c == '\n') {
			buffer[position] = '\0';
			return buffer;
		} else {
			buffer[position] = c;
		}
		position++;
	}

	// if we have exceeded the buffer, reallocate.
	if (position >= bufsize) {
		bufsize += LSH_RL_BUFSIZE;
		buffer = realloc(buffer, bufsize);
		if (!buffer) {
			fprintf(stderr, "lsh: allocation error\n");
			exit(EXIT_FAILURE);
		}
	}
}

/* an alternate, newer, and easier method for the lsh_read_line is this:
 * 
 * char* lsh_read_line(void)
 * {
 *  	char* line = NULL;
 *  	ssize_t bufsize = 0;
 *
 *  	if (getline(&line, &bufsize, stdin) == -1){
 *  		if (feof(stdin))
 *  			exit(EXIT_SUCCESS);
 *  		else {
 *  			perror("readline");
 *  			exit(EXIT_FAILURE);
 *  		}
 *  	}
 *
 *  	return line;
 *  }
 *
 *  both of these functions do the same thing, but the one that I'm implementing
 *  is an archaic way of doing it. However, the way I'm reading the line is actually
 *  better in terms of learning because at the time of me writing this code, I am currently
 *  unfamiliar with the ins and outs of c code.
 */

char** lsh_split_line(char* line) {
	int bufsize = LSH_TOK_BUFSIZE, position = 0;
	char** tokens = malloc(bufsize*sizeof(char*));
	char* token;

	if(!tokens) {
		fprintf(stderr, "lsh: allocation error\n");
		exit(EXIT_FAILURE);
	}

	token = strtok(line, LSH_TOK_DELIM);
	while (token != NULL) {
		tokens[position] = token;
		position++;

		if (position >= bufsize) {
			bufsize += LSH_TOK_BUFSIZE;
			tokens = realloc(tokens, bufsize*sizeof(char*));
			if (!tokens) {
				fprintf(stderr, "lsh: allocation error\n");
				exit(EXIT_FAILURE);
			}
		}

		token = strtok(NULL, LSH_TOK_DELIM);
	}
	tokens[position] = NULL;
	return tokens;
}

int lsh_launch(char** args) {
	pid_t pid, wpid;
	int status;

	pid = fork();
	if (pid == 0) {
		// Child process
		if (execvp(args[0], args) == -1) {
			perror("lsh");
		}
		exit(EXIT_FAILURE);
	} else if (pid < 0) {
		// Error forking
		perror("lsh");
	} else {
		// Parent process
		do {
			wpid = waitpid(pid, &status, WUNTRACED);
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}

	return 1;
}

// List of builtin commands, followed by their corresponding functions.

char *builtin_str[] = {
	"cd",
	"help",
	"exit"
};

int (*builtin_func[]) (char **) = {
	&lsh_cd,
	&lsh_help,
	&lsh_exit
};

int lsh_num_builtins() {
	return sizeof(builtin_str)/sizeof(char *);
}


// Builtin function implementations.

int lsh_cd(char** args) {
	if (args[1] == NULL)
		fprintf(stderr, "lsh: expected argument to \"cd\"\n");
	else {
		if (chdir(args[1]) != 0)
			perror("lsh");
	}
	return 1;
}

int lsh_help(char** args) {
	int i;
	
	printf("Colin Largen's Linux Shell\n");
	printf("Type the program names and arguments, and hit enter.\n");
	printf("The following are built in:\n");

	for (i=0; i<lsh_num_builtins(); i++)
		printf(" %s\n", builtin_str[i]);
	
	printf("Use the man command for informatioin on other programs.\n");
	return 1;
}

int lsh_exit(char** args) {
	return 0;
}

int lsh_execute(char** args) {
	int i;

	if (args[0] == NULL) {
		// An empty command was entered
		return 1;
	}

	for (i=0; i<lsh_num_builtins(); i++) {
		if (strcmp(args[0], builtin_str[i]) == 0)
			return (*builtin_func[i])(args);
	}

	return lsh_launch(args);
}
