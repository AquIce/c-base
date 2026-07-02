#include <base/foundation/macros.h>
#include <base/foundation/memory/memory.h>

#include <stdio.h>

int main(void) {
	MemorySource source = cmalloc_memory_source_create();

	int* value = (int*)memory_source_reserve(&source, sizeof(int), alignof(int), 0);
	if(!value) {
		return 1;
	}
	*value = 12;

	printf("Value: %d\n", *value);

	memory_source_release(&source, value, sizeof(int));
	cmalloc_memory_source_destroy(&source);

	return 0;
}
