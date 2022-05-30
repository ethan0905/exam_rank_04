/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   microshell.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: esafar <marvin@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/05/30 16:39:25 by esafar            #+#    #+#             */
/*   Updated: 2022/05/30 18:13:58 by esafar           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#define STDIN 0
#define STDOUT 1
#define STDERR 2

#define TYPE_END 3
#define TYPE_PIPE 4
#define TYPE_BREAK 5

typedef struct s_test
{
	char **argv;
	int size;
	int type;
	int fd[2];
	struct s_test *prev;
	struct s_test *next;
}		t_test;

int ft_strlen(char *str)
{
	int i = 0;

	while (str[i])
		i++;
	return (i);
}

char	*ft_strdup(char *src)
{
	size_t	i;
	char *dest;

	i = 0;
	dest = (char *)malloc(sizeof(char) * (ft_strlen(src) + 1));
	if (!dest)
		return (NULL);
	while (src[i] != '\0')
	{
		dest[i] = src[i];
		i++;
	}
	dest[i] = '\0';
	return (dest);
}

/*--------EXIT ERROR---------*/

void	exit_fatal(void)
{
	write(STDERR, "error: fatal\n", ft_strlen("error: fatal\n"));
	exit(EXIT_FAILURE);
}

void	exit_execve(char *str)
{
	write(STDERR, "error: cannot execute ", ft_strlen("error: cannot execute "));
	write(STDERR, str, ft_strlen(str));
	write(STDERR, "\n", 1);
	exit(EXIT_FAILURE);
}

int	exit_cd_1()
{
	write(STDERR, "error: cd: bad arguments\n", ft_strlen("error: cd: bad arguments\n"));
	return(EXIT_FAILURE);
}

int	exit_cd_2(char *str)
{
	write(STDERR, "error: cd: cannot change directory to ", ft_strlen("error: cd: cannot change directory to "));
	write(STDERR, str, ft_strlen(str));
	write(STDERR, "\n", 1);
	return (EXIT_FAILURE);
}

/*---------PARSING----------*/

int size_argv(char **argv)
{
	int i = 0;

	while (argv[i] && strcmp(argv[i], "|") != 0 && strcmp(argv[i], ";") != 0)
		i++;
	return (i);
}

int check_end(char *av)
{
	if (!av)
		return (TYPE_END);
	if (strcmp(av, "|") == 0)
		return (TYPE_PIPE);
	if (strcmp(av, ";") == 0)
		return (TYPE_BREAK);
	return (0);
}

void	ft_lstadd_back(t_test **test, t_test *new)
{
	t_test *tmp;

	if (!(*test))
		*test = new;
	else
	{
		tmp = *test;
		while (tmp->next)
			tmp = tmp->next;
		tmp->next = new;
		new->prev = tmp;
	}
}

int parser_argv(t_test **test, char **av)
{
	int size = size_argv(av);
	t_test *new;

	if (!(new = (t_test *)malloc(sizeof(t_test))))
		exit_fatal();
	if (!(new->argv = (char **)malloc(sizeof(char *) * (size+1))))
		exit_fatal();
	new->size = size;
	new->next = NULL;
	new->prev= NULL;
	new->argv[size] = NULL;
	while (--size >= 0)
		new->argv[size] = ft_strdup(av[size]);
	new->type = check_end(av[new->size]);
	ft_lstadd_back(test, new);
	return (new->size);
}

/*---------EXEC---------*/

void	exec_cmd(t_test *tmp, char **env)
{
	pid_t pid;
	int status;
	int pipe_open;

	pipe_open = 0;
	if (tmp->type == TYPE_PIPE || (tmp->prev && tmp->prev->type == TYPE_PIPE))
	{
		pipe_open = 1;
		if (pipe(tmp->fd))
			exit_fatal();
	}
	pid = fork();
	if (pid < 0)
		exit_fatal();
	else if (pid == 0) // child process
	{
		if (tmp->type == TYPE_PIPE && dup2(tmp->fd[STDOUT], STDOUT) < 0)
			exit_fatal();
		if (tmp->prev && tmp->prev->type == TYPE_PIPE && dup2(tmp->prev->fd[STDIN], STDIN) < 0)
			exit_fatal();
		if ((execve(tmp->argv[0], tmp->argv, env)) < 0)
			exit_execve(tmp->argv[0]);
		exit(EXIT_SUCCESS);
	}
	else //parent process
	{
		waitpid(pid, &status, 0);
		if (pipe_open)
		{
			close(tmp->fd[STDOUT]);
			if (!tmp->next || tmp->type == TYPE_BREAK)
				close(tmp->fd[STDIN]);
		}
		if (tmp->prev && tmp->prev->type == TYPE_PIPE)
			close(tmp->prev->fd[STDIN]);
	}
}

void	exec_cmds(t_test *test, char **env)
{
	t_test *tmp;

	tmp = test;
	while (tmp)
	{
		if (strcmp("cd", tmp->argv[0]) == 0)
		{
			if (tmp->size < 2)
				exit_cd_1();
			else if (chdir(tmp->argv[1]))
				exit_cd_2(tmp->argv[1]);
		}
		else
			exec_cmd(tmp, env);
		tmp = tmp->next;
	}
}

/*-----------------------*/

void	free_all(t_test *test)
{
	t_test *tmp;
	int i;

	while (test)
	{
		tmp = test->next;
		i = 0;
		while (i < test->size)
			free(test->argv[i++]);
		free(test->argv);
		free(test);
		test = tmp;
	}
	test = NULL;
}

int main(int ac, char **av, char **env)
{
	t_test *test = NULL;
	int i;

	i = 1;
	if (ac > 1)
	{
		while (av[i])
		{
			if (strcmp(av[i], ";") == 0)
			{
				i++;
				continue ;
			}
			i += parser_argv(&test, &av[i]);
			if (!av[i])
				break ;
			else
				i++;
		}
		if (test)
			exec_cmds(test, env);
		free_all(test);
	}
}
