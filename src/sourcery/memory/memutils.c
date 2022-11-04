#include <sourcery/memory/memutils.h>
#include <assert.h>

void
memory_set(void* buffer, size_t buffer_size, uint8 value)
{

	/**
	 * This routine is incredibly slow and shouldn't be used in production. We can
	 * assert this behavior by ensure that any release builds will cause an absolute
	 * melt-down.
	 */

#if !defined(SOURCERY_DEBUG)
	assert(!"A faster implementation of memory_set should be created before building a release candidate.");
#endif

	// Yikes, this is slow, but this will suffice to get the job done.
	for (size_t i = 0; i < buffer_size; ++i)
		((uint8*)buffer)[i] = value;

	return;

}
