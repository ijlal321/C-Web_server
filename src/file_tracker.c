

#include "file_tracker.h"

int file_tracker_init(struct FileTracker * file_tracker){
    // initliaze server lock
    if (pthread_mutex_init(&file_tracker->server.lock, NULL) != 0) {
        return -1;
    }

    // initialize file_tracker lock
    if (pthread_mutex_init(&file_tracker->lock, NULL) != 0) {
        pthread_mutex_destroy(&file_tracker->server.lock); // Question: Do i need to do this here ? A Prpoper cleanup is in necessary
        return -1;
    }
    return 0;

}


/**
 * @brief [Function/Variable Name]
 *
 * This function/variable performs its operation without acquiring any locks.
 * 
 * @note
 * - **No internal locking is performed.** The caller is responsible for ensuring proper synchronization and thread safety.
 * - Use with caution in multi-threaded environments to avoid race conditions.
 *
 * @pros
 * - Lightweight and fast, as it avoids the overhead of locking mechanisms.
 * - Suitable for scenarios where the caller already manages synchronization.
 *
 * @cons
 * - Unsafe to use concurrently without external locking.
 * - Potential for data races if used improperly.
 */
struct ClientFiles * search_client_in_accepted_clients(struct mg_connection *conn, struct FileTracker * file_tracker){
    return NULL;
}