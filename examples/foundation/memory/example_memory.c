#include <base/foundation/macros.h>
#include <base/foundation/memory/memory.h>

#include <stdio.h>

i32 main(void) {
	MemorySource source = cmalloc_memory_source_create();

	i32* value = MEMORY_SOURCE_RESERVE_T(i32, &source);
	if(!value) {
		return 1;
	}
	*value = 12;

	printf("Value: %d\n", *value);

	memory_source_release(&source, value, sizeof(i32));
	cmalloc_memory_source_destroy(&source);

	return 0;
}
