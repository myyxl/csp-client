#include <stdlib.h>
#include <pthread.h>

#include <log/log.h>
#include <param/param_types.h>

void param_lock(param_index_t * mem)
{
	if (mem->lock_inited == 0)
		return;
	pthread_mutex_lock(mem->lock);
}

void param_unlock(param_index_t * mem)
{
	if (mem->lock_inited == 0)
		return;
	pthread_mutex_unlock(mem->lock);
}

void param_lock_init(param_index_t * mem)
{
	pthread_mutex_t *mutex;

	mem->lock_inited = 0;

	/* There is no way to free this pointer, so
	 * it will be leaked... */
	mutex = malloc(sizeof(*mutex));
	if (!mutex) {
		log_warning("failed to allocate param index mutex");
		return;
	}

	if (pthread_mutex_init(mutex, NULL) != 0) {
		log_warning("failed to init param index mutex");
		return;
	}

	mem->lock = mutex;
	mem->lock_inited = 1;
}
