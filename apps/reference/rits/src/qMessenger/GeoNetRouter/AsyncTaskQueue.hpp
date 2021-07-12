/*
 *  Copyright (c) 2020, The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file       AsyncTaskQueue.hpp
 * @brief      Implements a queue that will hold on futures for async tasks.
 *             This allows async tasks to be created from within class methods
 *             and prevents the task from blocking at the end of a function a
 *             class method. This is necessary if the future is not returned.
 */

#ifndef ASYNCTASKQUEUE_HPP
#define ASYNCTASKQUEUE_HPP

#include <deque>
#include <future>
#include <memory>
#include <mutex>

template<typename T>
class AsyncTaskQueue {
public:

    /**
     * This function performs two functions - it first purges completed tasks from the front
     * of the queue, then it adds the new task to the end of the queue. We do this to simplify
     * its use. Clients are not required to call purgeCompleted themselves.
     *
     * @param [in] f - future associated with an async task
     */
    void add(std::shared_future<void> &f) {
        purgeCompleted();
        tasksQueue_.push_back(f);
    }

protected:

    /**
     * Removes completed tasks from the front of the task queue. For performance reasons, it
     * will not remove any completed tasks if there exits an uncompleted task ahead of it in
     * the task queue
     */
    void purgeCompleted() {

        std::lock_guard<std::mutex> lock(tasksMutex_);

        // Set timeout time to now so that we timeout immediately. Unfortunately,
        // futures don't have any methods to immediately find out if it's ready.
        // We always have to supply some timeout.
        std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
        auto itr = std::begin(tasksQueue_);

        // Iterate from head of queue and remove if the task is complete
        while (itr != std::end(tasksQueue_)) {
            bool doRemove = false;
            if (itr->valid()) {
                // If the task has already completed, we can remove it.
                if (std::future_status::ready == itr->wait_until(now)) {
                    doRemove = true;
                }
                else {
                }
            }
            else {
                // If the task is invalid, we'll just assume it's also complete
                // and remove it as well.
                doRemove = true;
            }

            // Remove the task if complete. If not, we'll just exit the loop.
            // Of course, we can just keep on looping and find all tasks that have
            // completed, but then we would be deleting from the middle of a queue
            // and we don't want to do that because it's a more expensive operation
            // than removing from the beginning of the queue.
            if (doRemove) {
                itr = tasksQueue_.erase(itr);
            }
            else {
                break;
            }
        }
    }

    std::mutex tasksMutex_;                        // mutex protecting queue
    std::deque<std::shared_future<T>> tasksQueue_; // queue of futures for async tasks
};
#endif // #ifndef ASYNCTASKQUEUE_HPP
