#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

#define HEAP_SIZE 1024

static char *heap = NULL;
static int currentHeapSize = 0;
static char isCallbackSet = 0;

typedef struct ListNode {
	void* next;
	int size;
} ListNode;

static ListNode* regionsList = NULL;

void* init_heap() {
	void* ptr = mmap(NULL, HEAP_SIZE, PROT_WRITE | PROT_READ | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if (ptr == MAP_FAILED) return NULL;

	regionsList = ptr;
	regionsList->next = NULL;
	regionsList->size = HEAP_SIZE - sizeof(ListNode);

	currentHeapSize = HEAP_SIZE;
	return ptr;
}

void finilize() {
	if (munmap(heap, HEAP_SIZE) == -1) {
		perror("Could not finilize()");
	}
	heap = NULL;
	currentHeapSize = 0;
}

void split(ListNode* curr, ListNode* prev, int size) {
	// Отрезаем от curr кусок незанятой памяти
	ListNode* newNode = (ListNode*)((int)curr + size + sizeof(ListNode));
	newNode->next = curr->next; //вставляем её справа от curr
	newNode->size = curr->size - size - sizeof(ListNode);

	if (prev == NULL) {
		puts("SHEM");
		// Первое выделение памяти, ставим ноду в начало
		regionsList = newNode;
		return;
	}

	prev->next = newNode;
}

int resize(int size) {
	// size % HEAP_SIZE == 0;
	size = (size + HEAP_SIZE - 1) & ~(HEAP_SIZE-1);

	void* ptr = mmap(heap, size, PROT_WRITE | PROT_READ | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if (ptr == MAP_FAILED) return -1;

	ListNode* node = ptr;
	node->size = size - sizeof(ListNode);
	node->next = regionsList;

	currentHeapSize += size;
	regionsList = node;
}

void* my_malloc(int size) {
	if (heap == NULL) {
		heap = init_heap();
		if (heap == NULL) {
			return NULL;
		}
	}

	if (!isCallbackSet) {
		atexit(finilize);
		isCallbackSet = 1;
	}

	// size % 8 == 0;
	size = (size + sizeof(void*) - 1) & ~(sizeof(void*)-1);

	ListNode* curr = regionsList;
	ListNode* prev = NULL;
	while (curr != NULL) {
		// Ищем регион подходящего размера
		if (curr->size < size) {
			prev = curr;
			curr = curr->next;
			continue;
		}
		// Нашли!!!
		
		// Если слишком большой, отрезаем пустой кусок
		if (curr->size - size > sizeof(ListNode)) {
			split(curr, prev, size);
		// Иначе целиком вырезаем выделенную память
		} else {
			if (prev == NULL) {
				regionsList = curr->next;
			} else {
				prev->next = curr->next;
			}
		}

		curr->next = NULL;
		// метаданные для free() слева от указателя
		return curr + 1;
	}

	// Не нашли подходящий кусок памяти,
	// Меняем размеры нод и пробуем снова
	int status = resize(size + sizeof(ListNode));
	if (status != -1) return my_malloc(size);

	// Не удалось поменять размер -> память не выделена
	return NULL;
}

void my_free(void *ptr) {
	// Ищем ноду, отвечающую за этот кусок
	ListNode* node = (ListNode*)ptr-1;

	// Если вся память занята, то
	// начнём перезаписывать с этого
	// освобождённого региона
	if (regionsList == NULL) {
		regionsList = node;
		return;
	}

	// Иначе вставляем пустой регион в список
	ListNode* curr = regionsList;
	ListNode* prev = NULL;
	while(curr != NULL) {
		// Ищем ноду, после которой
		// надо впихнуть очищенную
		if ((int)node > (int)curr) {
			prev = curr;
			curr = curr->next;
			continue;
		}

		if (prev != NULL) {
			prev->next = node;
		} else {
			regionsList = node;
		}
		node->next = curr;
		break;
	}

	// Не нашли такую, пихаем в конец
	if (node->next == NULL) {
		prev->next = node;
		return;
	}

	// Во-избежание фрагментации, мёржим
	// куски памяти правее ноды
	ListNode* toBeMerged = node->next;
	while(toBeMerged != NULL) {
		if ((int)toBeMerged != (int)node + node->size + sizeof(ListNode)) {
			break;
		}

		node->next = toBeMerged->next;
		node->size += toBeMerged->size + sizeof(ListNode);

		toBeMerged = toBeMerged->next;
	}
}