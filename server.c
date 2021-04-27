#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
- program takes one argument, an integer identifying which port it will use to listen on (between 5000 and 65536)
- opens and binds a listening socket on the specified port
- waits for incoming connection requests
	- For each connection:
		- create a thread to handle communication with that client
		- thread terminates once the connection is closed
- maintains the key-value data in a data structure shared by all threads
*/

typedef struct Node
{
	char* key;
	char* value;
	struct Node *next;
}Node;

Node* head = NULL;

Node* node(char* key, char* value)
{
	Node* new_node = (Node*) malloc(sizeof(Node));
	char* new_key = malloc(sizeof(char) * (strlen(key) + 1));
	char* new_value = malloc(sizeof(char) * (strlen(value) + 1));
	strcpy(new_key, key);
	strcpy(new_value, value);
	new_node->key = new_key;
	new_node->value = new_value;
	new_node->next = NULL;
	return new_node;
}

char* get(char* key)
{
	Node* curr_node = head;
	while(curr_node != NULL)
	{
		if (strcmp(curr_node->key, key) == 0)
		{
			return curr_node->value;
		}
		curr_node = curr_node->next;
	}
	return NULL;
}

void set(char* key, char* value)
{
	if (head == NULL)
	{
		head = node(key, value);
		return;
	}
	Node* curr_node = head;
	int compare;
	int counter = 0;
	while (curr_node != NULL)
	{
		counter++;
		compare = strcmp(key, curr_node->key);
		if (compare == 0)
		{
			curr_node->value = realloc(curr_node->value, (sizeof(char) * (strlen(value) + 1)));
			strcpy(curr_node->value, value);
			return;
		}
		else if (compare > 0)
		{	
			if (curr_node->next == NULL)
			{
				curr_node->next = node(key, value);
				return;
			}
			else if (strcmp(key, curr_node->next->key) < 0)
			{
				// key fits before next node
				Node* new_node = node(key, value);
				new_node->next = curr_node->next;
				curr_node->next = new_node;
				return;
			}
		}
		else
		{	
			if (counter == 1)
			{
				// key fits before head
				Node* new_node = node(key, value);
				new_node->next = curr_node;
				head = new_node;
				return;
			}
			// key should have gone before curr_node, error
			printf("error");
		}
		curr_node = curr_node->next;
	}
	// if we reach here, error
	return;
}

char* del(char* key)
{
	Node* curr_node = head;
	int counter = 0;
	while (curr_node->next != NULL)
	{
		counter++;
		Node* next_node = curr_node->next;
		if ((strcmp(curr_node->key, key) == 0) && (counter == 1))
		{
			// delete head
			char *to_return = malloc(sizeof(char) * (strlen(head->value) + 1));
			strcpy(to_return, head->value);
			free(head);
			head = next_node;
			return to_return;
		}
		int compare = strcmp(next_node->key, key);
		if (compare > 0)
		{
			// we've passed where it would've been, can conlcude a match isn't in the key list
			return NULL;
		}
		else if (compare == 0)
		{
			char *to_return = malloc(sizeof(char) * (strlen(next_node->value) + 1));
			strcpy(to_return, next_node->value);
			curr_node->next = next_node->next;
			free(next_node);
			return to_return;
		}
		curr_node = next_node;
	}
	// check if list is of size 1 and head matches
	if (counter == 0 && (strcmp(curr_node->key, key) == 0))
	{
		Node* new_head = NULL;
		char *to_return = malloc(sizeof(char) * (strlen(head->value) + 1));
		strcpy(to_return, head->value);
		free(head);
		head = new_head;
		return to_return;
	}
	// key not in list
	return NULL;
}

void print_list()
{
	Node *curr_node = head;
	while (curr_node != NULL)
	{
		printf("%s: %s\n", curr_node->key, curr_node->value);
		curr_node = curr_node->next;
	}
}

int main(int argc, char **argv)
{
	
	return 0;
}

