/**
 * Copyright (C) Mellanox Technologies Ltd. 2001-2015.  ALL RIGHTS RESERVED.
 *
 * See file LICENSE for terms.
 */


#ifndef UCM_H_
#define UCM_H_

#include <ucs/sys/compiler.h>

BEGIN_C_DECLS

#include <ucs/sys/math.h>
#include <ucs/type/status.h>

#include <sys/types.h>
#include <stddef.h>


/**
 * @brief Memory event types
 */
typedef enum ucm_event_type {
    /* Native events */
    UCM_EVENT_MMAP        = UCS_BIT(0),
    UCM_EVENT_MUNMAP      = UCS_BIT(1),
    UCM_EVENT_MREMAP      = UCS_BIT(2),
    UCM_EVENT_SHMAT       = UCS_BIT(3),
    UCM_EVENT_SHMDT       = UCS_BIT(4),
    UCM_EVENT_SBRK        = UCS_BIT(5),

    /* Aggregate events */
    UCM_EVENT_VM_MAPPED   = UCS_BIT(16),
    UCM_EVENT_VM_UNMAPPED = UCS_BIT(17)
} ucm_event_type_t;


/**
 * @brief Memory event parameters and result.
 */
typedef union ucm_event {
    /*
     * UCM_EVENT_MMAP
     * mmap() is called.
     * callbacks: pre, post
     */
    struct {
        void           *result;
        void           *address;
        size_t         size;
        int            prot;
        int            flags;
        int            fd;
        off_t          offset;
    } mmap;

    /*
     * UCM_EVENT_MUNMAP
     * munmap() is called.
     */
    struct {
        int            result;
        void           *address;
        size_t         size;
    } munmap;

    /*
     * UCM_EVENT_MREMAP
     * mremap() is called.
     */
    struct {
        void           *result;
        void           *address;
        size_t         old_size;
        size_t         new_size;
        int            flags;
    } mremap;

    /*
     * UCM_EVENT_SHMAT
     * shmat() is called.
     */
    struct {
        void           *result;
        int            shmid;
        const void     *shmaddr;
        int            shmflg;
    } shmat;

    /*
     * UCM_EVENT_SHMDT
     * shmdt() is called.
     */
    struct {
        int            result;
        const void     *shmaddr;
    } shmdt;

    /*
     * UCM_EVENT_SBRK
     * sbrk() is called.
     */
    struct {
        void           *result;
        intptr_t       increment;
    } sbrk;

    /*
     * UCM_EVENT_VM_MAPPED, UCM_EVENT_VM_UNMAPPED
     *
     * This is a "read-only" event which is called whenever memory is mapped
     * or unmapped from process address space, in addition to the other events.
     * It can return only UCM_EVENT_STATUS_NEXT.
     *
     * For UCM_EVENT_VM_MAPPED, callbacks are post
     * For UCM_EVENT_VM_UNMAPPED, callbacks are pre
     */
    struct {
        void           *address;
        size_t         size;
    } vm_mapped, vm_unmapped;

} ucm_event_t;


/**
 * @brief Memory event callback.
 *
 *  This type describes a callback which handles memory events in the current process.
 *
 * @param [in]     event_type  Type of the event being fired. see @ref ucm_event_type_t.
 * @param [inout]  event       Event information. This structure can be updated by
 *                               this callback, as described below.
 * @param [in]     arg         User-defined argument as passed to @ref ucm_set_event_handler.
 *
 *
 *  Events are dispatched in order of callback priority (low to high).
 *
 * The fields of the relevant part of the union are initialized as follows:
 *  - "result" - to an invalid erroneous return value (depends on the specific event).
 *  - the rest - to the input parameters of the event.
 *
 *  The callback is allowed to modify the fields, and those modifications will
 * be passed to the next callback. Also, the callback is allowed to modify the
 * result, but **only if it's currently invalid**. A valid result indicates that
 * a previous callback already performed the requested memory operation, so a
 * callback should **refrain from actions with side-effects** in this case.
 *
 *  If the result is still invalid after all callbacks are called, the parameters,
 * possibly modified by the callbacks, will be passed to the original handler.
 *
 *
 * Important Note: The callback must not call any memory allocation routines, or
 *       anything which may trigger or wait for memory allocation, because it
 *       may lead to deadlock or infinite recursion.
 *
 * @todo describe use cases
 *
 */
typedef void (*ucm_event_callback_t)(ucm_event_type_t event_type,
                                     ucm_event_t *event, void *arg);


/**
 * @brief Install a handler for memory events.
 *
 * @param [in]  events     Bit-mask of events to handle.
 * @param [in]  priority   Priority value which defines the order in which event
 *                          callbacks are called.
 *                           <  0 - called before the original implementation,
 *                           >= 0 - called after the original implementation.
 * @param [in]  cb         Event-handling callback.
 * @param [in]  arg        User-defined argument for the callback.
 *
 * @return Status code.
 */
ucs_status_t ucm_set_event_handler(int events, int priority,
                                   ucm_event_callback_t cb, void *arg);


/**
 * @brief Remove a handler for memory events.
 *
 * @param [in]  events     Which events to remove. The handler is removed
 *                          completely when all its events are removed.
 * @param [in]  cb         Event-handling callback.
 * @param [in]  arg        User-defined argument for the callback.
 */
void ucm_unset_event_handler(int events, ucm_event_callback_t cb, void *arg);


/**
 * @brief Call the original implementation of @ref mmap without triggering events.
 */
void *ucm_orig_mmap(void *addr, size_t length, int prot, int flags, int fd,
                    off_t offset);


/**
 * @brief Call the original implementation of @ref munmap without triggering events.
 */
int ucm_orig_munmap(void *addr, size_t length);


/**
 * @brief Call the original implementation of @ref mremap without triggering events.
 */
void *ucm_orig_mremap(void *old_address, size_t old_size, size_t new_size,
                      int flags);


/**
 * @brief Call the original implementation of @ref shmat without triggering events.
 */
void *ucm_orig_shmat(int shmid, const void *shmaddr, int shmflg);


/**
 * @brief Call the original implementation of @ref shmdt without triggering events.
 */
int ucm_orig_shmdt(const void *shmaddr);


/**
 * @brief Call the original implementation of @ref sbrk without triggering events.
 */
void *ucm_orig_sbrk(intptr_t increment);


END_C_DECLS

#endif